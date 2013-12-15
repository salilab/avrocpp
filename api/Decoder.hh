/*
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

#ifndef avro_Decoder_hh__
#define avro_Decoder_hh__

#include "Config.hh"
#include <stdint.h>
#include <string>
#include <vector>

#include "ValidSchema.hh"
#include "Stream.hh"

#include <boost/shared_ptr.hpp>

/// \file
///
/// Low level support for decoding avro values.
/// This class has two types of funtions.  One type of functions support
/// decoding of leaf values (for example, decodeLong and
/// decodeString). These functions have analogs in Encoder.
///
/// The other type of functions support decoding of maps and arrays.
/// These functions are arrayStart, startItem, and arrayEnd
/// (and similar functions for maps).

namespace internal_avro {

/**
 * Decoder is an interface implemented by every decoder capable
 * of decoding Avro data.
 */
class AVRO_DECL Decoder {
 public:
  virtual ~Decoder() {};
  /// All future decoding will come from is, which should be valid
  /// until replaced by another call to init() or this Decoder is
  /// destructed.
  virtual void init(InputStream& is) = 0;

  /// Decodes a null from the current stream.
  virtual void decodeNull() = 0;

  /// Decodes a bool from the current stream
  virtual bool decodeBool() = 0;

  /// Decodes a 32-bit int from the current stream.
  virtual int32_t decodeInt() = 0;

  /// Decodes a 64-bit signed int from the current stream.
  virtual int64_t decodeLong() = 0;

  /// Decodes a single-precision floating point number from current stream.
  virtual float decodeFloat() = 0;

  /// Decodes a double-precision floating point number from current stream.
  virtual double decodeDouble() = 0;

  /// Decodes a UTF-8 string from the current stream.
  std::string decodeString() {
    std::string result;
    decodeString(result);
    return result;
  }

  /**
   * Decodes a UTF-8 string from the stream and assigns it to value.
   */
  virtual void decodeString(std::string& value) = 0;

  /// Skips a string on the current stream.
  virtual void skipString() = 0;

  /// Decodes arbitray binary data from the current stream.
  std::vector<uint8_t> decodeBytes() {
    std::vector<uint8_t> result;
    decodeBytes(result);
    return result;
  }

  /// Decodes arbitray binary data from the current stream and puts it
  /// in value.
  virtual void decodeBytes(std::vector<uint8_t>& value) = 0;

  /// Skips bytes on the current stream.
  virtual void skipBytes() = 0;

  /**
   * Decodes fixed length binary from the current stream.
   * \param[in] n The size (byte count) of the fixed being read.
   * \return The fixed data that has been read. The size of the returned
   * vector is guaranteed to be equal to \p n.
   */
  std::vector<uint8_t> decodeFixed(size_t n) {
    std::vector<uint8_t> result;
    decodeFixed(n, result);
    return result;
  }

  /**
   * Decodes a fixed from the current stream.
   * \param[in] n The size (byte count) of the fixed being read.
   * \param[out] value The value that receives the fixed. The vector will
   * be size-adjusted based on the fixed's size.
   */
  virtual void decodeFixed(size_t n, std::vector<uint8_t>& value) = 0;

  /// Skips fixed length binary on the current stream.
  virtual void skipFixed(size_t n) = 0;

  /// Decodes enum from the current stream.
  virtual size_t decodeEnum() = 0;

  /// Start decoding an array. Returns the number of entries in first chunk.
  virtual size_t arrayStart() = 0;

  /// Returns the number of entries in next chunk. 0 if last.
  virtual size_t arrayNext() = 0;

  /// Tries to skip an array. If it can, it returns 0. Otherwise
  /// it returns the number of elements to be skipped. The client
  /// should skip the individual items. In such cases, skipArray
  /// is identical to arrayStart.
  virtual size_t skipArray() = 0;

  /// Start decoding a map. Returns the number of entries in first chunk.
  virtual size_t mapStart() = 0;

  /// Returns the number of entries in next chunk. 0 if last.
  virtual size_t mapNext() = 0;

  /// Tries to skip a map. If it can, it returns 0. Otherwise
  /// it returns the number of elements to be skipped. The client
  /// should skip the individual items. In such cases, skipMap
  /// is identical to mapStart.
  virtual size_t skipMap() = 0;

  /// Decodes a branch of a union. The actual value is to follow.
  virtual size_t decodeUnionIndex() = 0;
};

template <class Impl>
class GenericDecoder: public Decoder {
  Impl impl_;

 public:
  virtual ~GenericDecoder() {};

  virtual void init(InputStream& is) { impl_.init(is); }

  virtual void decodeNull() { impl_.decodeNull(); }

  virtual bool decodeBool()  { return impl_.decodeBool();}

  virtual int32_t decodeInt()  { return impl_.decodeInt();}

  virtual int64_t decodeLong()  { return impl_.decodeLong();}

  virtual float decodeFloat()  { return impl_.decodeFloat();}

  virtual double decodeDouble() { return impl_.decodeDouble(); }

  virtual void decodeString(std::string& value) { impl_.decodeString(value); }

  virtual void skipString() { impl_.skipString(); }

  virtual void decodeBytes(std::vector<uint8_t>& value) {
    impl_.decodeBytes(value);
  }

  virtual void skipBytes() { impl_.skipBytes(); }

  virtual void decodeFixed(size_t n, std::vector<uint8_t>& value) {
    impl_.decodeFixed(n, value);
  }

  virtual void skipFixed(size_t n) {  impl_.skipFixed(n);}

  virtual size_t decodeEnum() { return impl_.decodeEnum();}

  virtual size_t arrayStart() { return impl_.arrayStart();}

  virtual size_t arrayNext() { return impl_.arrayNext();}

  virtual size_t skipArray() { return impl_.skipArray();}

  virtual size_t mapStart() { return impl_.mapStart();}

  virtual size_t mapNext() { return impl_.mapNext();}

  virtual size_t skipMap() { return impl_.skipMap();}

  virtual size_t decodeUnionIndex() { return impl_.decodeUnionIndex(); }
};


/**
 * Shared pointer to Decoder.
 */
typedef boost::shared_ptr<Decoder> DecoderPtr;

/**
 * ResolvingDecoder is derived from \ref Decoder, with an additional
 * function to obtain the field ordering of fiedls within a record.
 */
class AVRO_DECL ResolvingDecoder : public Decoder {
 public:
  /// Returns the order of fields for records.
  /// The order of fields could be different from the order of their
  /// order in the schema because the writer's field order could
  /// be different. In order to avoid buffering and later use,
  /// we return the values in the writer's field order.
  virtual const std::vector<size_t>& fieldOrder() = 0;
};

/**
 * Shared pointer to ResolvingDecoder.
 */
typedef boost::shared_ptr<ResolvingDecoder> ResolvingDecoderPtr;

class BinaryDecoder {
  StreamReader in_;
  const uint8_t* next_;
  const uint8_t* end_;

 public:
  void init(InputStream& is) { in_.reset(is); }
  void decodeNull() {}
  bool decodeBool();
  int32_t decodeInt() {
    int64_t val = doDecodeLong();
    if (val < INT32_MIN || val > INT32_MAX) {
      throw Exception(boost::format("Value out of range for Avro int: %1%") %
                      val);
    }
    return static_cast<int32_t>(val);
  }
  int64_t decodeLong() { return doDecodeLong(); }
  float decodeFloat() {
    float result;
    in_.readBytes(reinterpret_cast<uint8_t*>(&result), sizeof(float));
    return result;
  }
  double decodeDouble() {
    double result;
    in_.readBytes(reinterpret_cast<uint8_t*>(&result), sizeof(double));
    return result;
  }
  void decodeString(std::string& value) {
    size_t len = decodeInt();
    value.resize(len);
    if (len > 0) {
      in_.readBytes(reinterpret_cast<uint8_t*>(&value[0]), len);
    }
  }
  void skipString()  {
    size_t len = decodeInt();
    in_.skipBytes(len);
  }
  void decodeBytes(std::vector<uint8_t>& value);
  void skipBytes() {
    size_t len = decodeInt();
    in_.skipBytes(len);
  }
  void decodeFixed(size_t n, std::vector<uint8_t>& value);
  void skipFixed(size_t n) { in_.skipBytes(n); }
  size_t decodeEnum() { return static_cast<size_t>(doDecodeLong()); }
  size_t arrayStart() { return doDecodeItemCount(); }
  size_t arrayNext() { return static_cast<size_t>(doDecodeLong()); }
  size_t skipArray();
  size_t mapStart() { return doDecodeItemCount(); }
  size_t mapNext() { return doDecodeItemCount(); }
  size_t skipMap() { return skipArray(); }
  size_t decodeUnionIndex() { return static_cast<size_t>(doDecodeLong()); }

  int64_t doDecodeLong();
  size_t doDecodeItemCount();
  void more();
};

/**
 *  Returns an decoder that can decode binary Avro standard.
 */
AVRO_DECL DecoderPtr binaryDecoder();

/**
 *  Returns an decoder that validates sequence of calls to an underlying
 *  Decoder against the given schema.
 */
AVRO_DECL DecoderPtr
    validatingDecoder(const ValidSchema& schema, const DecoderPtr& base);

/**
 *  Returns an decoder that can decode Avro standard for JSON.
 */
AVRO_DECL DecoderPtr jsonDecoder(const ValidSchema& schema);

/**
 *  Returns a decoder that decodes avro data from base written according to
 *  writerSchema and resolves against readerSchema.
 *  The client uses the decoder as if the data were written using readerSchema.
 *  // FIXME: Handle out of order fields.
 */
AVRO_DECL ResolvingDecoderPtr resolvingDecoder(const ValidSchema& writer,
                                               const ValidSchema& reader,
                                               const DecoderPtr& base);

}  // namespace internal_avro

#endif
