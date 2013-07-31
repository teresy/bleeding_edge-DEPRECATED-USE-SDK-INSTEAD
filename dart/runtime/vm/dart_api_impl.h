// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef VM_DART_API_IMPL_H_
#define VM_DART_API_IMPL_H_

#include "vm/allocation.h"
#include "vm/native_arguments.h"
#include "vm/object.h"

namespace dart {

class ApiLocalScope;
class ApiState;
class FinalizablePersistentHandle;
class LocalHandle;
class PersistentHandle;

const char* CanonicalFunction(const char* func);

#define CURRENT_FUNC CanonicalFunction(__FUNCTION__)

// Checks that the current isolate is not NULL.
#define CHECK_ISOLATE(isolate)                                                 \
  do {                                                                         \
    if ((isolate) == NULL) {                                                   \
      FATAL1("%s expects there to be a current isolate. Did you "              \
             "forget to call Dart_CreateIsolate or Dart_EnterIsolate?",        \
             CURRENT_FUNC);                                                    \
    }                                                                          \
  } while (0)

// Checks that the current isolate is NULL.
#define CHECK_NO_ISOLATE(isolate)                                              \
  do {                                                                         \
    if ((isolate) != NULL) {                                                   \
      FATAL1("%s expects there to be no current isolate. Did you "             \
             "forget to call Dart_ExitIsolate?", CURRENT_FUNC);                \
    }                                                                          \
  } while (0)

// Checks that the current isolate is not NULL and that it has an API scope.
#define CHECK_ISOLATE_SCOPE(isolate)                                           \
  do {                                                                         \
    Isolate* tmp = (isolate);                                                  \
    CHECK_ISOLATE(tmp);                                                        \
    ApiState* state = tmp->api_state();                                        \
    ASSERT(state);                                                             \
    if (state->top_scope() == NULL) {                                          \
      FATAL1("%s expects to find a current scope. Did you forget to call "     \
           "Dart_EnterScope?", CURRENT_FUNC);                                  \
    }                                                                          \
  } while (0)

#define DARTSCOPE(isolate)                                                     \
  Isolate* __temp_isolate__ = (isolate);                                       \
  CHECK_ISOLATE_SCOPE(__temp_isolate__);                                       \
  HANDLESCOPE(__temp_isolate__);


#define RETURN_TYPE_ERROR(isolate, dart_handle, type)                          \
  do {                                                                         \
    const Object& tmp =                                                        \
        Object::Handle(isolate, Api::UnwrapHandle((dart_handle)));             \
    if (tmp.IsNull()) {                                                        \
      return Api::NewError("%s expects argument '%s' to be non-null.",         \
                           CURRENT_FUNC, #dart_handle);                        \
    } else if (tmp.IsError()) {                                                \
      return dart_handle;                                                      \
    } else {                                                                   \
      return Api::NewError("%s expects argument '%s' to be of type %s.",       \
                           CURRENT_FUNC, #dart_handle, #type);                 \
    }                                                                          \
  } while (0)


#define RETURN_NULL_ERROR(parameter)                                           \
  return Api::NewError("%s expects argument '%s' to be non-null.",             \
                       CURRENT_FUNC, #parameter);


#define CHECK_LENGTH(length, max_elements)                                     \
  do {                                                                         \
    intptr_t len = (length);                                                   \
    intptr_t max = (max_elements);                                             \
    if (len < 0 || len > max) {                                                \
      return Api::NewError(                                                    \
          "%s expects argument '%s' to be in the range [0..%"Pd"].",           \
          CURRENT_FUNC, #length, max);                                         \
    }                                                                          \
  } while (0)


class Api : AllStatic {
 public:
  // Creates a new local handle.
  static Dart_Handle NewHandle(Isolate* isolate, RawObject* raw);

  // Unwraps the raw object from the handle.
  static RawObject* UnwrapHandle(Dart_Handle object);

  // Unwraps a raw Type from the handle.  The handle will be null if
  // the object was not of the requested Type.
#define DECLARE_UNWRAP(Type)                                                   \
  static const Type& Unwrap##Type##Handle(Isolate* isolate,                    \
                                          Dart_Handle object);
  CLASS_LIST_FOR_HANDLES(DECLARE_UNWRAP)
#undef DECLARE_UNWRAP

  // Validates and converts the passed in handle as a persistent handle.
  static PersistentHandle* UnwrapAsPersistentHandle(
      Dart_PersistentHandle object);

  // Validates and converts the passed in handle as a weak persistent handle.
  static FinalizablePersistentHandle* UnwrapAsWeakPersistentHandle(
      Dart_WeakPersistentHandle object);

  // Validates and converts the passed in handle as a prologue weak
  // persistent handle.
  static FinalizablePersistentHandle* UnwrapAsPrologueWeakPersistentHandle(
      Dart_WeakPersistentHandle object);

  // Returns an Error handle if isolate is in an inconsistent state.
  // Returns a Success handle when no error condition exists.
  static Dart_Handle CheckIsolateState(Isolate *isolate);

  // Casts the internal Isolate* type to the external Dart_Isolate type.
  static Dart_Isolate CastIsolate(Isolate* isolate);

  // Gets the handle used to designate successful return.
  static Dart_Handle Success() { return Api::True(); }

  // Sets up the acquired error object after initializing an Isolate. This
  // object is pre-created because we will not be able to allocate this
  // object when the error actually occurs. When the error occurs there will
  // be outstanding acquires to internal data pointers making it unsafe to
  // allocate objects on the dart heap.
  static void SetupAcquiredError(Isolate* isolate);

  // Gets the handle which holds the pre-created acquired error object.
  static Dart_Handle AcquiredError(Isolate* isolate);

  // Returns true if the handle holds a Smi.
  static bool IsSmi(Dart_Handle handle) {
    // TODO(turnidge): Assumes RawObject* is at offset zero.  Fix.
    RawObject* raw = *(reinterpret_cast<RawObject**>(handle));
    return !raw->IsHeapObject();
  }

  // Returns true if the handle holds a Dart Instance.
  static bool IsInstance(Dart_Handle handle) {
    return (ClassId(handle) >= kInstanceCid);
  }

  // Returns the value of a Smi.
  static intptr_t SmiValue(Dart_Handle handle) {
    // TODO(turnidge): Assumes RawObject* is at offset zero.  Fix.
    uword value = *(reinterpret_cast<uword*>(handle));
    return Smi::ValueFromRaw(value);
  }

  static intptr_t ClassId(Dart_Handle handle) {
    // TODO(turnidge): Assumes RawObject* is at offset zero.  Fix.
    RawObject* raw = *(reinterpret_cast<RawObject**>(handle));
    if (!raw->IsHeapObject()) {
      return kSmiCid;
    }
    return raw->GetClassId();
  }

  // Generates a handle used to designate an error return.
  static Dart_Handle NewError(const char* format, ...) PRINTF_ATTRIBUTE(1, 2);

  // Gets a handle to Null.
  static Dart_Handle Null();

  // Gets a handle to True.
  static Dart_Handle True();

  // Gets a handle to False
  static Dart_Handle False();

  // Retrieves the top ApiLocalScope.
  static ApiLocalScope* TopScope(Isolate* isolate);

  // Performs one-time initialization needed by the API.
  static void InitOnce();

  // Allocates handles for objects in the VM isolate.
  static void InitHandles();

  // Helper function to get the peer value of an external string object.
  static bool ExternalStringGetPeerHelper(Dart_Handle object, void** peer);

  // Helper function to set the return value of native functions.
  static void SetReturnValue(NativeArguments* args, Dart_Handle retval) {
    NoGCScope no_gc_scope;
    args->SetReturnUnsafe(UnwrapHandle(retval));
  }
  static void SetSmiReturnValue(NativeArguments* args, intptr_t retval) {
    NoGCScope no_gc_scope;
    args->SetReturnUnsafe(Smi::New(retval));
  }
  static void SetIntegerReturnValue(NativeArguments* args, intptr_t retval) {
    NoGCScope no_gc_scope;
    args->SetReturnUnsafe(Integer::New(retval));
  }
  static void SetDoubleReturnValue(NativeArguments* args, double retval) {
    NoGCScope no_gc_scope;
    args->SetReturnUnsafe(Double::New(retval));
  }

 private:
  // Thread local key used by the API. Currently holds the current
  // ApiNativeScope if any.
  static ThreadLocalKey api_native_key_;
  static Dart_Handle true_handle_;
  static Dart_Handle false_handle_;
  static Dart_Handle null_handle_;

  friend class ApiNativeScope;
};

class IsolateSaver {
 public:
  explicit IsolateSaver(Isolate* current_isolate)
      : saved_isolate_(current_isolate) {
  }
  ~IsolateSaver() {
    Isolate::SetCurrent(saved_isolate_);
  }
 private:
  Isolate* saved_isolate_;

  DISALLOW_COPY_AND_ASSIGN(IsolateSaver);
};

// Start a scope in which no Dart API call backs are allowed.
#define START_NO_CALLBACK_SCOPE(isolate)                                       \
  isolate->IncrementNoCallbackScopeDepth()

// End a no Dart API call backs Scope.
#define END_NO_CALLBACK_SCOPE(isolate)                                         \
  isolate->DecrementNoCallbackScopeDepth()

#define CHECK_CALLBACK_STATE(isolate)                                          \
  if (isolate->no_callback_scope_depth() != 0) {                               \
    return reinterpret_cast<Dart_Handle>(Api::AcquiredError(isolate));         \
  }                                                                            \

#define ASSERT_CALLBACK_STATE(isolate)                                         \
  ASSERT(isolate->no_callback_scope_depth() == 0)

}  // namespace dart.

#endif  // VM_DART_API_IMPL_H_
