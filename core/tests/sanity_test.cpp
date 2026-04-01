#include <gtest/gtest.h>

TEST(Sanity, CompilerWorks) { EXPECT_EQ(1 + 1, 2); }

TEST(Sanity, StdStringWorks) {
  std::string s = "ibridger";
  EXPECT_EQ(s.size(), 8u);
}
