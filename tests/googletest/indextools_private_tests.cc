#include <gtest/gtest.h>

#include "indextools_private.hpp"

// Demonstrate some basic assertions.
TEST(IndexToolsStaticTest, BasicAssertions) {
  IndexID index_id = MainIndexIndex::indexID;

  // Expect two strings not to be equal.
  EXPECT_STRNE("hello", "world");
  // Expect equality.
  EXPECT_EQ(7 * 6, 42);
}
