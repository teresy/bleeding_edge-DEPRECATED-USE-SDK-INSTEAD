// Copyright (c) 2013, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

import "package:expect/expect.dart";
import 'memory_compiler.dart' show compilerFor;
import '../../../sdk/lib/_internal/compiler/implementation/apiimpl.dart' show
    Compiler;
import
    '../../../sdk/lib/_internal/compiler/implementation/tree/tree.dart'
show
    Node;
import
    '../../../sdk/lib/_internal/compiler/implementation/dart_backend/dart_backend.dart';
import
    '../../../sdk/lib/_internal/compiler/implementation/mirror_renamer/mirror_renamer.dart';
import
    '../../../sdk/lib/_internal/compiler/implementation/scanner/scannerlib.dart'
show
    SourceString;

main() {
  testWithMirrorHelperLibrary(minify: true);
  testWithMirrorHelperLibrary(minify: false);
  testWithoutMirrorHelperLibrary(minify: true);
  testWithoutMirrorHelperLibrary(minify: false);
}

Compiler runCompiler({useMirrorHelperLibrary: false, minify: false}) {
  List<String> options = ['--output-type=dart'];
  if (minify) {
    options.add('--minify');
  }
  Compiler compiler = compilerFor(MEMORY_SOURCE_FILES, options: options);
  DartBackend backend = compiler.backend;
  backend.useMirrorHelperLibrary = useMirrorHelperLibrary;
  compiler.runCompiler(Uri.parse('memory:main.dart'));
  return compiler;
}

void testWithMirrorHelperLibrary({bool minify}) {
  Compiler compiler = runCompiler(useMirrorHelperLibrary: true, minify: minify);

  DartBackend backend = compiler.backend;
  MirrorRenamer mirrorRenamer = backend.mirrorRenamer;
  Map<Node, String> renames = backend.renames;
  Map<String, SourceString> symbols = mirrorRenamer.symbols;

  Expect.isFalse(null == backend.mirrorHelperLibrary);
  Expect.isFalse(null == backend.mirrorHelperGetNameFunction);
  Expect.isTrue(symbols.containsValue(
      const SourceString(MirrorRenamer.MIRROR_HELPER_GET_NAME_FUNCTION)));

  for (Node n in renames.keys) {
    if (symbols.containsKey(renames[n])) {
      if(n.toString() == 'getName') {
        Expect.equals(
            const SourceString(MirrorRenamer.MIRROR_HELPER_GET_NAME_FUNCTION),
            symbols[renames[n]]);
      } else {
        Expect.equals(n.toString(), symbols[renames[n]].stringValue);
      }
    }
  }

  String output = compiler.assembledCode;
  String getNameMatch = MirrorRenamer.MIRROR_HELPER_GET_NAME_FUNCTION;
  Iterable i = getNameMatch.allMatches(output);


  if (minify) {
    Expect.equals(1, i.length);
  } else {
    // Appears twice in code (defined & called) and twice in renames map.
    Expect.equals(4, i.length);
  }

  String mapMatch = 'const<String,SourceString>';
  i = mapMatch.allMatches(output);
  Expect.equals(1, i.length);
}

void testWithoutMirrorHelperLibrary({bool minify}) {
  Compiler compiler =
      runCompiler(useMirrorHelperLibrary: false, minify: minify);
  DartBackend backend = compiler.backend;

  Expect.equals(null, backend.mirrorHelperLibrary);
  Expect.equals(null, backend.mirrorHelperGetNameFunction);
  Expect.equals(null, backend.mirrorRenamer);
}

const MEMORY_SOURCE_FILES = const <String, String> {
  'main.dart': """
import 'dart:mirrors';

class Foo {
  noSuchMethod(Invocation invocation) {
    MirrorSystem.getName(invocation.memberName);
  }
}

void main() {
  new Foo().fisk();
}
"""};