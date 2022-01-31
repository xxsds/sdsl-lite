#include <gtest/gtest.h>// IWYU pragma: keep
// IWYU pragma: no_include "gtest/gtest_pred_impl.h"
// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
// IWYU pragma: no_include <gtest/gtest-typed-test.h>

@SDSL_INCLUDE_ALL@

namespace
{
// The fixture for testing the compilation of all header files.
class CompileTest : public ::testing::Test { };

//! Test constructors
TEST_F(CompileTest, Compile) { }

}  // namespace

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
