// this is the caretc a compiler for the C^
#include <gtest/gtest.h>
#include "caretc.hpp"
TEST(CaretC, VersionString) {
    EXPECT_STREQ(CARETC_VERSION, "0.1.0");
}
