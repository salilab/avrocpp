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

#define __STDC_LIMIT_MACROS

#include "Decoder.hh"
#include "Zigzag.hh"
#include "Exception.hh"

#include <boost/array.hpp>
#include <boost/make_shared.hpp>

namespace internal_avro {

using boost::make_shared;

DecoderPtr binaryDecoder() {
  return make_shared<GenericDecoder<BinaryDecoder> >();
}

bool BinaryDecoder::decodeBool() {
  uint8_t v = in_.read();
  if (v == 0) {
    return false;
  } else if (v == 1) {
    return true;
  }
  throw Exception("Invalid value for bool");
}

int32_t BinaryDecoder::decodeInt() {
  int64_t val = doDecodeLong();
  if (val < INT32_MIN || val > INT32_MAX) {
    throw Exception(boost::format("Value out of range for Avro int: %1%") %
                    val);
  }
  return static_cast<int32_t>(val);
}

void BinaryDecoder::decodeBytes(std::vector<uint8_t>& value) {
  size_t len = decodeInt();
  value.resize(len);
  if (len > 0) {
    in_.readBytes(&value[0], len);
  }
}

void BinaryDecoder::decodeFixed(size_t n, std::vector<uint8_t>& value) {
  value.resize(n);
  if (n > 0) {
    in_.readBytes(&value[0], n);
  }
}

size_t BinaryDecoder::doDecodeItemCount() {
  int64_t result = doDecodeLong();
  if (result < 0) {
    doDecodeLong();
    return static_cast<size_t>(-result);
  }
  return static_cast<size_t>(result);
}

size_t BinaryDecoder::skipArray() {
  for (;;) {
    int64_t r = doDecodeLong();
    if (r < 0) {
      size_t n = static_cast<size_t>(doDecodeLong());
      in_.skipBytes(n);
    } else {
      return static_cast<size_t>(r);
    }
  }
}

int64_t BinaryDecoder::doDecodeLong() {
  uint64_t encoded = 0;
  int shift = 0;
  uint8_t u;
  do {
    if (shift >= 64) {
      throw Exception("Invalid Avro varint");
    }
    u = in_.read();
    encoded |= static_cast<uint64_t>(u & 0x7f) << shift;
    shift += 7;
  } while (u & 0x80);

  return decodeZigzag64(encoded);
}

}  // namespace internal_avro
