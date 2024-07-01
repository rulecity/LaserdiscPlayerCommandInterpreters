#ifndef LDP_IN_STDAFX_H
#define LDP_IN_STDAFX_H

// Compiling tests may go faster if we use pre-compiled headers for these massive google test includes
#include <gmock/gmock.h>
#include <gtest/gtest.h>

// google mock stuff here
#include "mocks.h"
using ::testing::AtLeast;
using ::testing::Return;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Pointee;
using ::testing::SetArgumentPointee;
using ::testing::Mock;
using ::testing::Throw;
using ::testing::InSequence;

// Test compatibility macros

#define TEST_CHECK(a)   EXPECT_TRUE(a)

#define TEST_REQUIRE(a) ASSERT_TRUE(a)

#define TEST_CHECK_EQUAL(a,b)   EXPECT_EQ(a,b)

#define TEST_REQUIRE_EQUAL(a,b) ASSERT_EQ(a,b)

#define TEST_CHECK_NOT_EQUAL(a,b)   EXPECT_NE(a,b)

#define TEST_REQUIRE_NOT_EQUAL(a,b) ASSERT_NE(a,b)

#define TEST_CHECK_THROW(a) { bool bCaught = false; try { a; } catch (...) { bCaught = true; } EXPECT_TRUE(bCaught); }

// normal test case (all tests will run)
// TEST is from google test
#define TEST_CASE(name) TEST(LdpInTest, name)

// exclusive test flag ignored for now
#define TEST_CASE_EX(name) TEST_CASE(name)

#endif //LDP_IN_STDAFX_H
