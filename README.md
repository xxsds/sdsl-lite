# SDSL v3 - Succinct Data Structure Library

[![linux status][1]][2]
[![macos status][3]][4]

[1]: https://img.shields.io/github/workflow/status/xxsds/sdsl-lite/SDSL%20CI%20on%20Linux/master?style=flat&logo=github&label=Linux%20CI "Open GitHub actions page"
[2]: https://github.com/xxsds/sdsl-lite/actions?query=branch%3Amaster
[3]: https://img.shields.io/github/workflow/status/xxsds/sdsl-lite/SDSL%20CI%20on%20macOS/master?style=flat&logo=github&label=macOS%20CI "Open GitHub actions page"
[4]: https://github.com/xxsds/sdsl-lite/actions?query=branch%3Amaster

## Main differences to [v2](https://github.com/simongog/sdsl-lite)

* header-only library
* support for serialisation via [cereal](https://github.com/USCiLab/cereal)
* compatible with C++17 and C++20

## Supported compilers

Other compiler may work, but are not tested within the continuous integration. In general, the latest minor release of each
listed major compiler version is supported.

* GCC 8, 9, 10, 11
* clang 9, 10, 11, 12

## Dependencies

As SDSL v3 is header-only, dependencies marked as `required` only apply to building tests/examples.

* required: [CMake >= 3.2](https://github.com/Kitware/CMake)
* required: [googletest 1.11.0](https://github.com/google/googletest/releases/tag/release-1.11.0)
* optional: [cereal 1.3.0](https://github.com/USCiLab/cereal)

cereal can be activated by passing `-DSDSL_CEREAL=1` to CMake.
