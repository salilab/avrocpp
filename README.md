avrocpp
=======

Patched fork of the Apache AvroC++ project. The purposes of this fork are:
- to provide a version of avro that can be bundled internally to a project without risk of conflicting with an external build
- add seek support for avro data containers
- add support for writing avro data containers to arbitrary streams
- fix various problems with avro exports
- accelerate Avro by allowing one to remove the virtual function calles in the encode/decode data flows

Branches names are the version of avrocpp that is being forked. See the log for the branch for a more complete list of patches.

Errata:
- current resolving decoding of object containers is broken
- the code structure of encoders and decoders are is a bit of a mess