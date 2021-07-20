IREE can now execute TFLite models through the use of TOSA, an open standard of common tensor operations, and a part of MLIR core. TOSA’s high-level representation of tensor operations provides a common front-end for ingesting models from different front-ends. In this case we ingest a TFLite flatbuffer, compile to TOSA IR which IREE can compile to its variety of backends.

Executing TFLite on IREE provides an alternative execution environment to the TFLite runtime that can leverage IREE’s advantages, specifically multi-cpu and GPU architecture support with a lightweight execution environment. Having this compile-once-run-everywhere approach guarantees more consistent performance on a variety of clients without depending on client-side updates.


Today, we have validated floating point support for a variety of models, including mobilenet (v1, v2, and v3) and mobilebert. More work is in progress to support fully quantized models, and TFLite’s hybrid quantization, along with dynamic shape support.

Examples of using TFLite with IREE are available in Python and Java.  We have a colab notebook that shows how to use IREE’s python bindings and TFLite compiler tools to compile and run a pre-trained TFLite model from a flatbuffer.  We are also working on an Android Java app that was forked from an existing TFLite demo app, and we swapped out the TFLite library for our own AAR.