// Copyright (c) 2012, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "vm/globals.h"
#if defined(TARGET_ARCH_ARM)

#include "vm/assembler.h"
#include "vm/cpu.h"
#include "vm/cpuinfo.h"
#include "vm/heap.h"
#include "vm/isolate.h"
#include "vm/object.h"
#include "vm/simulator.h"

#if defined(HOST_ARCH_ARM)
#include <sys/syscall.h>  /* NOLINT */
#include <unistd.h>  /* NOLINT */
#endif

// ARM version differences.
// We support three major 32-bit ARM ISA versions: ARMv5TE, ARMv6 and variants,
// and ARMv7 and variants. For each of these we detect the presence of vfp,
// neon, and integer division instructions. Considering ARMv5TE as the baseline,
// later versions add the following features/instructions that we use:
//
// ARMv6:
// - PC read offset in store instructions is 8 rather than 12, matching the
//   offset in read instructions,
// - strex, ldrex, and clrex load/store/clear exclusive instructions,
// - umaal multiplication instruction,
// ARMv7:
// - movw, movt 16-bit immediate load instructions,
// - mls multiplication instruction,
// - vmovs, vmovd floating point immediate load instructions.
//
// If an aarch64 CPU is detected, we generate ARMv7 code.
//
// If an instruction is missing on ARMv5TE or ARMv6, we emulate it, if possible.
// Where we are missing vfp, we do not unbox doubles, or generate intrinsics for
// floating point operations. Where we are missing neon, we do not unbox SIMD
// values, or inline operations on SIMD values. Where we are missing integer
// division, we do not inline division operations, and we do not generate
// intrinsics that do division. See the feature tests in flow_graph_optimizer.cc
// for details.
//
// Alignment:
//
// Before ARMv6, that is only for ARMv5TE, unaligned accesses will cause a
// crash. This includes the ldrd and strd instructions, which must use addresses
// that are 8-byte aligned. Since we don't always guarantee that for our uses
// of ldrd and strd, these instructions are emulated with two load or store
// instructions on ARMv5TE. On ARMv6 and on, we assume that the kernel is
// set up to fixup unaligned accesses. This can be verified by checking
// /proc/cpu/alignment on modern Linux systems.

namespace dart {

// TODO(zra): Add a target for ARMv6.
#if defined(TARGET_ARCH_ARM_5TE)
DEFINE_FLAG(bool, use_vfp, false, "Use vfp instructions if supported");
DEFINE_FLAG(bool, use_neon, false, "Use neon instructions if supported");
DEFINE_FLAG(bool, use_integer_division, false,
            "Use integer division instruction if supported");
#else
DEFINE_FLAG(bool, use_vfp, true, "Use vfp instructions if supported");
DEFINE_FLAG(bool, use_neon, true, "Use neon instructions if supported");
DEFINE_FLAG(bool, use_integer_division, true,
            "Use integer division instruction if supported");
#endif

#if !defined(HOST_ARCH_ARM)
#if defined(TARGET_ARCH_ARM_5TE)
DEFINE_FLAG(bool, sim_use_hardfp, false, "Use the softfp ABI.");
#else
DEFINE_FLAG(bool, sim_use_hardfp, true, "Use the softfp ABI.");
#endif
#endif

void CPU::FlushICache(uword start, uword size) {
#if defined(HOST_ARCH_ARM)
  // Nothing to do. Flushing no instructions.
  if (size == 0) {
    return;
  }

  // ARM recommends using the gcc intrinsic __clear_cache on Linux, and the
  // library call cacheflush from unistd.h on Android:
  // blogs.arm.com/software-enablement/141-caches-and-self-modifying-code/
  #if defined(__linux__) && !defined(ANDROID)
    extern void __clear_cache(char*, char*);
    char* beg = reinterpret_cast<char*>(start);
    char* end = reinterpret_cast<char*>(start + size);
    ::__clear_cache(beg, end);
  #elif defined(ANDROID)
    cacheflush(start, start + size, 0);
  #else
    #error FlushICache only tested/supported on Linux and Android
  #endif

#endif
}


const char* CPU::Id() {
  return
#if !defined(HOST_ARCH_ARM)
  "sim"
#endif  // !defined(HOST_ARCH_ARM)
  "arm";
}


bool HostCPUFeatures::integer_division_supported_ = false;
bool HostCPUFeatures::vfp_supported_ = false;
bool HostCPUFeatures::neon_supported_ = false;
bool HostCPUFeatures::hardfp_supported_ = false;
const char* HostCPUFeatures::hardware_ = NULL;
ARMVersion HostCPUFeatures::arm_version_ = ARMvUnknown;
intptr_t HostCPUFeatures::store_pc_read_offset_ = 8;
#if defined(DEBUG)
bool HostCPUFeatures::initialized_ = false;
#endif


#if defined(HOST_ARCH_ARM)
void HostCPUFeatures::InitOnce() {
  bool is_arm64 = false;
  CpuInfo::InitOnce();
  hardware_ = CpuInfo::GetCpuModel();

  // Check for ARMv5TE, ARMv6, ARMv7, or aarch64.
  // It can be in either the Processor or Model information fields.
  if (CpuInfo::FieldContains(kCpuInfoProcessor, "aarch64") ||
      CpuInfo::FieldContains(kCpuInfoModel, "aarch64")) {
    // pretend that this arm64 cpu is really an ARMv7
    arm_version_ = ARMv7;
    is_arm64 = true;
  } else if (CpuInfo::FieldContains(kCpuInfoProcessor, "ARM926EJ-S") ||
             CpuInfo::FieldContains(kCpuInfoModel, "ARM926EJ-S")) {
    // Lego Mindstorm EV3.
    arm_version_ = ARMv5TE;
    // On ARMv5, the PC read offset in an STR or STM instruction is either 8 or
    // 12 bytes depending on the implementation. On the Mindstorm EV3 it is 12
    // bytes.
    store_pc_read_offset_ = 12;
  } else if (CpuInfo::FieldContains(kCpuInfoProcessor, "Feroceon 88FR131") ||
             CpuInfo::FieldContains(kCpuInfoModel, "Feroceon 88FR131")) {
    // This is for the DGBox. For the time-being, assume it is similar to the
    // Lego Mindstorm.
    arm_version_ = ARMv5TE;
    store_pc_read_offset_ = 12;
  } else if (CpuInfo::FieldContains(kCpuInfoProcessor, "ARMv6") ||
             CpuInfo::FieldContains(kCpuInfoModel, "ARMv6")) {
    // Raspberry Pi, etc.
    arm_version_ = ARMv6;
  } else {
    ASSERT(CpuInfo::FieldContains(kCpuInfoProcessor, "ARMv7") ||
           CpuInfo::FieldContains(kCpuInfoModel, "ARMv7"));
    arm_version_ = ARMv7;
  }

  // Has floating point unit.
  vfp_supported_ =
      (CpuInfo::FieldContains(kCpuInfoFeatures, "vfp") || is_arm64) &&
      FLAG_use_vfp;

  // Has integer division.
  bool is_krait = CpuInfo::FieldContains(kCpuInfoHardware, "QCT APQ8064");
  if (is_krait) {
    // Special case for Qualcomm Krait CPUs in Nexus 4 and 7.
    integer_division_supported_ = FLAG_use_integer_division;
  } else {
    integer_division_supported_ =
        (CpuInfo::FieldContains(kCpuInfoFeatures, "idiva") || is_arm64) &&
        FLAG_use_integer_division;
  }
  neon_supported_ =
      (CpuInfo::FieldContains(kCpuInfoFeatures, "neon") || is_arm64) &&
      FLAG_use_vfp && FLAG_use_neon;

  // Use the cross-compiler's predefined macros to determine whether we should
  // use the hard or soft float ABI.
#if defined(__ARM_PCS_VFP)
  hardfp_supported_ = true;
#else
  hardfp_supported_ = false;
#endif

#if defined(DEBUG)
  initialized_ = true;
#endif
}


void HostCPUFeatures::Cleanup() {
  DEBUG_ASSERT(initialized_);
#if defined(DEBUG)
  initialized_ = false;
#endif
  ASSERT(hardware_ != NULL);
  free(const_cast<char*>(hardware_));
  hardware_ = NULL;
  CpuInfo::Cleanup();
}

#else

void HostCPUFeatures::InitOnce() {
  CpuInfo::InitOnce();
  hardware_ = CpuInfo::GetCpuModel();

#if defined(TARGET_ARCH_ARM_5TE)
  arm_version_ = ARMv5TE;
#else
  arm_version_ = ARMv7;
#endif

  integer_division_supported_ = FLAG_use_integer_division;
  vfp_supported_ = FLAG_use_vfp;
  neon_supported_ = FLAG_use_vfp && FLAG_use_neon;
  hardfp_supported_ = FLAG_sim_use_hardfp;
#if defined(DEBUG)
  initialized_ = true;
#endif
}


void HostCPUFeatures::Cleanup() {
  DEBUG_ASSERT(initialized_);
#if defined(DEBUG)
  initialized_ = false;
#endif
  ASSERT(hardware_ != NULL);
  free(const_cast<char*>(hardware_));
  hardware_ = NULL;
  CpuInfo::Cleanup();
}
#endif  // defined(HOST_ARCH_ARM)

}  // namespace dart

#endif  // defined TARGET_ARCH_ARM
