# SDSL v3 - Succinct Data Structure Library

[![CI][1]][2]

[1]: https://img.shields.io/github/actions/workflow/status/xxsds/sdsl-lite/ci_linux.yml?branch=master&style=flat&logo=github&label=CI "Open GitHub actions page"
[2]: https://github.com/xxsds/sdsl-lite/actions?query=branch%3Amaster+event%3Apush

## Main differences to [v2](https://github.com/simongog/sdsl-lite)

* header-only library
* support for serialisation via [cereal](https://github.com/USCiLab/cereal)
* compatible with C++17, C++20, and C++23

## Supported compilers

Other compiler may work, but are not tested within the continuous integration. In general, the latest minor release of
each listed major compiler version is supported.

* GCC 11, 12, 13
* clang 14, 15, 16

Tests are run with C++20 and C++23.

## Dependencies

As SDSL v3 is header-only, dependencies marked as `required` only apply to building tests/examples.

* required: [CMake >= 3.2](https://github.com/Kitware/CMake)
* required: [googletest 1.13.0](https://github.com/google/googletest/releases/tag/release-1.13.0)
* optional: [cereal 1.3.2](https://github.com/USCiLab/cereal)

cereal can be activated by passing `-DSDSL_CEREAL=1` to CMake.
