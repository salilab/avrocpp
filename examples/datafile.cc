/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fstream>

#include "cpx.hh"
#include "avro/Encoder.hh"
#include "avro/Decoder.hh"
#include "avro/ValidSchema.hh"
#include "avro/Compiler.hh"
#include "avro/DataFile.hh"

internal_avro::ValidSchema loadSchema(const char* filename) {
  std::ifstream ifs(filename);
  internal_avro::ValidSchema result;
  internal_avro::compileJsonSchema(ifs, result);
  return result;
}

int main() {
  internal_avro::ValidSchema cpxSchema = loadSchema("cpx.json");

  {
    internal_avro::DataFileWriter<c::cpx> dfw("test.bin", cpxSchema);
    c::cpx c1;
    for (int i = 0; i < 100; i++) {
      c1.re = i * 100;
      c1.im = i + 100;
      dfw.write(c1);
    }
    dfw.close();
  }

  {
    internal_avro::DataFileReader<c::cpx> dfr("test.bin", cpxSchema);
    c::cpx c2;
    while (dfr.read(c2)) {
      std::cout << '(' << c2.re << ", " << c2.im << ')' << std::endl;
    }
  }
  return 0;
}
