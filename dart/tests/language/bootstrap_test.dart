// Copyright (c) 2013, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.
// Dart test for testing binary operations.
// VMOptions=--verify_implements  --check-function-fingerprints

class BootstrapTest {

  static testMain() {
    var obj = new Object();

    return obj;
  }

}

main() {
  BootstrapTest.testMain();
}
