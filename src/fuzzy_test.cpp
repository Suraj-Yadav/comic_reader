#include "fuzzy.hpp"

#include <gtest/gtest.h>

TEST(fuzzy, less) { EXPECT_FALSE(fuzzy::less(1.0000, 1.0001)); }

TEST(fuzzy, greater) { EXPECT_FALSE(fuzzy::greater(1.0001, 1.0000)); }

TEST(fuzzy, equal) { EXPECT_TRUE(fuzzy::equal(1.0000000, 1.0000001)); }

TEST(fuzzy, less_equal) { EXPECT_TRUE(fuzzy::less_equal(1.0000, 1.0001)); }

TEST(fuzzy, greater_equal) {
	EXPECT_TRUE(fuzzy::greater_equal(1.0001, 1.0000));
}
