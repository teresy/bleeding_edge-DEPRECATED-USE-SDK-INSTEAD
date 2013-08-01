// Copyright (c) 2013, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

import "dart:mirrors";

import "package:expect/expect.dart";

typedef void FooFunction(int a, double b);

bar(int a) {}

main() {
  TypedefMirror tm = reflectClass(FooFunction);
  FunctionTypeMirror ftm = tm.referent;
  Expect.equals(const Symbol("void"), ftm.returnType.simpleName);
  Expect.equals(const Symbol("int"), ftm.parameters[0].type.simpleName);
  Expect.equals(const Symbol("double"), ftm.parameters[1].type.simpleName);
  ClosureMirror cm = reflect(bar);
  ftm = cm.type;
  Expect.equals(const Symbol("dynamic"), ftm.returnType.simpleName);
  Expect.equals(const Symbol("int"), ftm.parameters[0].type.simpleName);
}
