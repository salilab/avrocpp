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

#include <boost/test/included/unit_test_framework.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include <sstream>

#include "DataFile.hh"
#include "Generic.hh"
#include "Stream.hh"
#include "Compiler.hh"

using std::auto_ptr;
using std::string;
using std::pair;
using std::vector;
using std::map;
using std::istringstream;
using std::ostringstream;

using boost::array;
using boost::shared_ptr;
using boost::unit_test::test_suite;

using internal_avro::ValidSchema;
using internal_avro::GenericDatum;
using internal_avro::GenericRecord;

const int count = 1000;

template <typename T>
struct Complex {
  T re;
  T im;
  Complex() : re(0), im(0) {}
  Complex(T r, T i) : re(r), im(i) {}
};

struct Integer {
  int64_t re;
  Integer() : re(0) {}
  Integer(int64_t r) : re(r) {}
};

typedef Complex<int64_t> ComplexInteger;
typedef Complex<double> ComplexDouble;

struct Double {
  double re;
  Double() : re(0) {}
  Double(double r) : re(r) {}
};

namespace internal_avro {

template <typename T>
struct codec_traits<Complex<T> > {
  static void encode(Encoder& e, const Complex<T>& c) {
    internal_avro::encode(e, c.re);
    internal_avro::encode(e, c.im);
  }

  static void decode(Decoder& d, Complex<T>& c) {
    internal_avro::decode(d, c.re);
    internal_avro::decode(d, c.im);
  }
};

template <>
struct codec_traits<Integer> {
  static void decode(Decoder& d, Integer& c) { internal_avro::decode(d, c.re); }
};

template <>
struct codec_traits<Double> {
  static void decode(Decoder& d, Double& c) { internal_avro::decode(d, c.re); }
};
}

static ValidSchema makeValidSchema(const char* schema) {
  istringstream iss(schema);
  ValidSchema vs;
  compileJsonSchema(iss, vs);
  return ValidSchema(vs);
}

static const char sch[] =
    "{\"type\": \"record\","
    "\"name\":\"ComplexInteger\", \"fields\": ["
    "{\"name\":\"re\", \"type\":\"long\"},"
    "{\"name\":\"im\", \"type\":\"long\"}"
    "]}";
static const char isch[] =
    "{\"type\": \"record\","
    "\"name\":\"ComplexInteger\", \"fields\": ["
    "{\"name\":\"re\", \"type\":\"long\"}"
    "]}";
static const char dsch[] =
    "{\"type\": \"record\","
    "\"name\":\"ComplexDouble\", \"fields\": ["
    "{\"name\":\"re\", \"type\":\"double\"},"
    "{\"name\":\"im\", \"type\":\"double\"}"
    "]}";
static const char dblsch[] =
    "{\"type\": \"record\","
    "\"name\":\"ComplexDouble\", \"fields\": ["
    "{\"name\":\"re\", \"type\":\"double\"}"
    "]}";

string toString(const ValidSchema& s) {
  ostringstream oss;
  s.toJson(oss);
  return oss.str();
}

class DataFileTest {
  const char* filename;
  const ValidSchema writerSchema;
  const ValidSchema readerSchema;

 public:
  DataFileTest(const char* f, const char* wsch, const char* rsch)
      : filename(f),
        writerSchema(makeValidSchema(wsch)),
        readerSchema(makeValidSchema(rsch)) {}

  typedef pair<ValidSchema, GenericDatum> Pair;

  void testCleanup() { BOOST_CHECK(boost::filesystem::remove(filename)); }

  void testWrite() {
    internal_avro::DataFileWriter<ComplexInteger> df(filename, writerSchema,
                                                     100);
    int64_t re = 3;
    int64_t im = 5;
    for (int i = 0; i < count; ++i, re *= im, im += 3) {
      ComplexInteger c(re, im);
      df.write(c);
    }
    df.close();
  }

  void testWriteGeneric() {
    internal_avro::DataFileWriter<Pair> df(filename, writerSchema, 100);
    int64_t re = 3;
    int64_t im = 5;
    Pair p(writerSchema, GenericDatum());

    GenericDatum& c = p.second;
    c = GenericDatum(writerSchema.root());
    GenericRecord& r = c.value<GenericRecord>();

    for (int i = 0; i < count; ++i, re *= im, im += 3) {
      r.fieldAt(0) = re;
      r.fieldAt(1) = im;
      df.write(p);
    }
    df.close();
  }

  void testWriteDouble() {
    internal_avro::DataFileWriter<ComplexDouble> df(filename, writerSchema,
                                                    100);
    double re = 3.0;
    double im = 5.0;
    for (int i = 0; i < count; ++i, re += im - 0.7, im += 3.1) {
      ComplexDouble c(re, im);
      df.write(c);
    }
    df.close();
  }

  void testTruncate() {
    testWriteDouble();
    uintmax_t size = boost::filesystem::file_size(filename);
    {
      internal_avro::DataFileWriter<Pair> df(filename, writerSchema, 100);
      df.close();
    }
    uintmax_t new_size = boost::filesystem::file_size(filename);
    BOOST_CHECK(size > new_size);
  }

  void testReadFull() {
    internal_avro::DataFileReader<ComplexInteger> df(filename, writerSchema);
    int i = 0;
    ComplexInteger ci;
    int64_t re = 3;
    int64_t im = 5;
    while (df.read(ci)) {
      BOOST_CHECK_EQUAL(ci.re, re);
      BOOST_CHECK_EQUAL(ci.im, im);
      re *= im;
      im += 3;
      ++i;
    }
    BOOST_CHECK_EQUAL(i, count);
  }

  void testReadProjection() {
    internal_avro::DataFileReader<Integer> df(filename, readerSchema);
    int i = 0;
    Integer integer;
    int64_t re = 3;
    int64_t im = 5;
    while (df.read(integer)) {
      BOOST_CHECK_EQUAL(integer.re, re);
      re *= im;
      im += 3;
      ++i;
    }
    BOOST_CHECK_EQUAL(i, count);
  }

  void testReaderGeneric() {
    internal_avro::DataFileReader<Pair> df(filename, writerSchema);
    int i = 0;
    Pair p(writerSchema, GenericDatum());
    int64_t re = 3;
    int64_t im = 5;

    const GenericDatum& ci = p.second;
    while (df.read(p)) {
      BOOST_REQUIRE_EQUAL(ci.type(), internal_avro::AVRO_RECORD);
      const GenericRecord& r = ci.value<GenericRecord>();
      const size_t n = 2;
      BOOST_REQUIRE_EQUAL(r.fieldCount(), n);
      const GenericDatum& f0 = r.fieldAt(0);
      BOOST_REQUIRE_EQUAL(f0.type(), internal_avro::AVRO_LONG);
      BOOST_CHECK_EQUAL(f0.value<int64_t>(), re);

      const GenericDatum& f1 = r.fieldAt(1);
      BOOST_REQUIRE_EQUAL(f1.type(), internal_avro::AVRO_LONG);
      BOOST_CHECK_EQUAL(f1.value<int64_t>(), im);
      re *= im;
      im += 3;
      ++i;
    }
    BOOST_CHECK_EQUAL(i, count);
  }

  void testReaderGenericProjection() {
    internal_avro::DataFileReader<Pair> df(filename, readerSchema);
    int i = 0;
    Pair p(readerSchema, GenericDatum());
    int64_t re = 3;
    int64_t im = 5;

    const GenericDatum& ci = p.second;
    while (df.read(p)) {
      BOOST_REQUIRE_EQUAL(ci.type(), internal_avro::AVRO_RECORD);
      const GenericRecord& r = ci.value<GenericRecord>();
      const size_t n = 1;
      BOOST_REQUIRE_EQUAL(r.fieldCount(), n);
      const GenericDatum& f0 = r.fieldAt(0);
      BOOST_REQUIRE_EQUAL(f0.type(), internal_avro::AVRO_LONG);
      BOOST_CHECK_EQUAL(f0.value<int64_t>(), re);

      re *= im;
      im += 3;
      ++i;
    }
    BOOST_CHECK_EQUAL(i, count);
  }

  void testReadDouble() {
    internal_avro::DataFileReader<ComplexDouble> df(filename, writerSchema);
    int i = 0;
    ComplexDouble ci;
    double re = 3.0;
    double im = 5.0;
    while (df.read(ci)) {
      BOOST_CHECK_CLOSE(ci.re, re, 0.0001);
      BOOST_CHECK_CLOSE(ci.im, im, 0.0001);
      re += (im - 0.7);
      im += 3.1;
      ++i;
    }
    BOOST_CHECK_EQUAL(i, count);
  }

  /**
   * Constructs the DataFileReader in two steps.
   */
  void testReadDoubleTwoStep() {
    auto_ptr<internal_avro::DataFileReaderBase> base(
        new internal_avro::DataFileReaderBase(filename));
    internal_avro::DataFileReader<ComplexDouble> df(base);
    BOOST_CHECK_EQUAL(toString(writerSchema), toString(df.readerSchema()));
    BOOST_CHECK_EQUAL(toString(writerSchema), toString(df.dataSchema()));
    int i = 0;
    ComplexDouble ci;
    double re = 3.0;
    double im = 5.0;
    while (df.read(ci)) {
      BOOST_CHECK_CLOSE(ci.re, re, 0.0001);
      BOOST_CHECK_CLOSE(ci.im, im, 0.0001);
      re += (im - 0.7);
      im += 3.1;
      ++i;
    }
    BOOST_CHECK_EQUAL(i, count);
  }

  /**
   * Constructs the DataFileReader in two steps using a different
   * reader schema.
   */
  void testReadDoubleTwoStepProject() {
    auto_ptr<internal_avro::DataFileReaderBase> base(
        new internal_avro::DataFileReaderBase(filename));
    internal_avro::DataFileReader<Double> df(base, readerSchema);

    BOOST_CHECK_EQUAL(toString(readerSchema), toString(df.readerSchema()));
    BOOST_CHECK_EQUAL(toString(writerSchema), toString(df.dataSchema()));
    int i = 0;
    Double ci;
    double re = 3.0;
    double im = 5.0;
    while (df.read(ci)) {
      BOOST_CHECK_CLOSE(ci.re, re, 0.0001);
      re += (im - 0.7);
      im += 3.1;
      ++i;
    }
    BOOST_CHECK_EQUAL(i, count);
  }
};

void addReaderTests(test_suite* ts, const shared_ptr<DataFileTest>& t) {
  ts->add(BOOST_CLASS_TEST_CASE(&DataFileTest::testReadFull, t));
  ts->add(BOOST_CLASS_TEST_CASE(&DataFileTest::testReadProjection, t));
  ts->add(BOOST_CLASS_TEST_CASE(&DataFileTest::testReaderGeneric, t));
  ts->add(BOOST_CLASS_TEST_CASE(&DataFileTest::testReaderGenericProjection, t));
  ts->add(BOOST_CLASS_TEST_CASE(&DataFileTest::testCleanup, t));
}

test_suite* init_unit_test_suite(int argc, char* argv[]) {
  test_suite* ts = BOOST_TEST_SUITE("DataFile tests");
  shared_ptr<DataFileTest> t1(new DataFileTest("test1.df", sch, isch));
  ts->add(BOOST_CLASS_TEST_CASE(&DataFileTest::testWrite, t1));
  addReaderTests(ts, t1);

  shared_ptr<DataFileTest> t2(new DataFileTest("test2.df", sch, isch));
  ts->add(BOOST_CLASS_TEST_CASE(&DataFileTest::testWriteGeneric, t2));
  addReaderTests(ts, t2);

  shared_ptr<DataFileTest> t3(new DataFileTest("test3.df", dsch, dblsch));
  ts->add(BOOST_CLASS_TEST_CASE(&DataFileTest::testWriteDouble, t3));
  ts->add(BOOST_CLASS_TEST_CASE(&DataFileTest::testReadDouble, t3));
  ts->add(BOOST_CLASS_TEST_CASE(&DataFileTest::testReadDoubleTwoStep, t3));
  ts->add(
      BOOST_CLASS_TEST_CASE(&DataFileTest::testReadDoubleTwoStepProject, t3));
  ts->add(BOOST_CLASS_TEST_CASE(&DataFileTest::testCleanup, t3));

  shared_ptr<DataFileTest> t4(new DataFileTest("test4.df", dsch, dblsch));
  ts->add(BOOST_CLASS_TEST_CASE(&DataFileTest::testTruncate, t4));
  ts->add(BOOST_CLASS_TEST_CASE(&DataFileTest::testCleanup, t4));

  return ts;
}
