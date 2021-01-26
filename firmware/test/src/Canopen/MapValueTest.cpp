#include "CanopenTestFixture.hpp"

TEST_F(CanopenTest, mapValue)
{
// something is going wrong in the compiler and there are
// macro "EXPECT_EQ" passed 3 arguments, but takes just 2 errors when there are no ( ) around
// Canopen::mapValue<float, int32_t>(0.0f, 1.0f, 0, 1000, 0.5f)

// input positive
EXPECT_EQ((Canopen::mapValue<float, int32_t>(0.0f, 1.0f, 0, 1000, 0.5f)), 500);
EXPECT_EQ((Canopen::mapValue<float, int32_t>(0.0f, 1.0f, 0, 1000, 0.25f)), 250);
EXPECT_EQ((Canopen::mapValue<float, int32_t>(0.0f, 1.0f, 0, 1000, 0.75f)), 750);
EXPECT_EQ((Canopen::mapValue<float, int32_t>(0.0f, 1.0f, 0, 1000, 1.0f)), 1000);
EXPECT_EQ((Canopen::mapValue<float, int32_t>(0.0f, 1.0f, 0, 1000, 0.0f)), 0);

EXPECT_EQ((Canopen::mapValue<float, int32_t>(0.0f, 1.0f, -1000, 1000, 0.5f)), 0);
EXPECT_EQ((Canopen::mapValue<float, int32_t>(0.0f, 1.0f, -1000, 1000, 0.25f)), -500);
EXPECT_EQ((Canopen::mapValue<float, int32_t>(0.0f, 1.0f, -1000, 1000, 0.75f)), 500);
EXPECT_EQ((Canopen::mapValue<float, int32_t>(0.0f, 1.0f, -1000, 1000, 1.0f)), 1000);
EXPECT_EQ((Canopen::mapValue<float, int32_t>(0.0f, 1.0f, -1000, 1000, 0.0f)), -1000);

// input negative
EXPECT_EQ((Canopen::mapValue<float, int32_t>(-1.0f, 1.0f, 0, 1000, 0.0f)), 500);
EXPECT_EQ((Canopen::mapValue<float, int32_t>(-1.0f, 1.0f, 0, 1000, 1.0f)), 1000);
EXPECT_EQ((Canopen::mapValue<float, int32_t>(-1.0f, 1.0f, 0, 1000, -1.0f)), 0);
EXPECT_EQ((Canopen::mapValue<float, int32_t>(-1.0f, 1.0f, 0, 1000, -0.5f)), 250);
EXPECT_EQ((Canopen::mapValue<float, int32_t>(-1.0f, 1.0f, 0, 1000, 0.5f)), 750);

EXPECT_EQ((Canopen::mapValue<float, int32_t>(-1.0f, 1.0f, -1000, 1000, 0.0f)), 0);
EXPECT_EQ((Canopen::mapValue<float, int32_t>(-1.0f, 1.0f, -1000, 1000, 1.0f)), 1000);
EXPECT_EQ((Canopen::mapValue<float, int32_t>(-1.0f, 1.0f, -1000, 1000, -1.0f)), -1000);
EXPECT_EQ((Canopen::mapValue<float, int32_t>(-1.0f, 1.0f, -1000, 1000, -0.5f)), -500);
EXPECT_EQ((Canopen::mapValue<float, int32_t>(-1.0f, 1.0f, -1000, 1000, 0.5f)), 500);

// clipping
EXPECT_EQ((Canopen::mapValue<float, int32_t>(0.0f, 1.0f, 0, 1000, -1.5f)), 0);
EXPECT_EQ((Canopen::mapValue<float, int32_t>(0.0f, 1.0f, 0, 1000, 1.5f)), 1000);

EXPECT_EQ((Canopen::mapValue<float, int32_t>(0.0f, 1.0f, -1000, 1000, -1.5f)), -1000);
EXPECT_EQ((Canopen::mapValue<float, int32_t>(0.0f, 1.0f, -1000, 1000, 1.5f)), 1000);

EXPECT_EQ((Canopen::mapValue<float, int32_t>(-1.0f, 1.0f, 0, 1000, -1.5f)), 0);
EXPECT_EQ((Canopen::mapValue<float, int32_t>(-1.0f, 1.0f, 0, 1000, 1.5f)), 1000);

EXPECT_EQ((Canopen::mapValue<float, int32_t>(-1.0f, 1.0f, -1000, 1000, -1.5f)), -1000);
EXPECT_EQ((Canopen::mapValue<float, int32_t>(-1.0f, 1.0f, -1000, 1000, 1.5f)), 1000);

// everything in reverse for various testcases
// positive
EXPECT_EQ((Canopen::mapValue<int32_t, float>(0, 1000, 0.0f, 1.0f, 500)), 0.5f);
EXPECT_EQ((Canopen::mapValue<int32_t, float>(0, 1000, 0.0f, 1.0f, 250)), 0.25f);
EXPECT_EQ((Canopen::mapValue<int32_t, float>(0, 1000, 0.0f, 1.0f, 750)), 0.75f);
EXPECT_EQ((Canopen::mapValue<int32_t, float>(0, 1000, 0.0f, 1.0f, 1000)), 1.0f);
EXPECT_EQ((Canopen::mapValue<int32_t, float>(0, 1000, 0.0f, 1.0f, 0)), 0.0f);

EXPECT_EQ((Canopen::mapValue<int32_t, float>(0, 1000, -1.0f, 1.0f, 500)), 0.0f);
EXPECT_EQ((Canopen::mapValue<int32_t, float>(0, 1000, -1.0f, 1.0f, 250)), -0.5f);
EXPECT_EQ((Canopen::mapValue<int32_t, float>(0, 1000, -1.0f, 1.0f, 750)), 0.5f);
EXPECT_EQ((Canopen::mapValue<int32_t, float>(0, 1000, -1.0f, 1.0f, 1000)), 1.0f);
EXPECT_EQ((Canopen::mapValue<int32_t, float>(0, 1000, -1.0f, 1.0f, 0)), -1.0f);

// negative
EXPECT_EQ((Canopen::mapValue<int32_t, float>(-1000, 1000, 0.0f, 1.0f, -500)), 0.25f);
EXPECT_EQ((Canopen::mapValue<int32_t, float>(-1000, 1000, 0.0f, 1.0f, 1000)), 1.0f);
EXPECT_EQ((Canopen::mapValue<int32_t, float>(-1000, 1000, 0.0f, 1.0f, 0)), 0.5f);
EXPECT_EQ((Canopen::mapValue<int32_t, float>(-1000, 1000, 0.0f, 1.0f, 250)), 0.625f);
EXPECT_EQ((Canopen::mapValue<int32_t, float>(-1000, 1000, 0.0f, 1.0f, 750)), 0.875f);

EXPECT_EQ((Canopen::mapValue<int32_t, float>(-1000, 1000, -1.0f, 1.0f, 500)), 0.5f);
EXPECT_EQ((Canopen::mapValue<int32_t, float>(-1000, 1000, -1.0f, 1.0f, 1000)), 1.0f);
EXPECT_EQ((Canopen::mapValue<int32_t, float>(-1000, 1000, -1.0f, 1.0f, 0)), 0.0f);
EXPECT_EQ((Canopen::mapValue<int32_t, float>(-1000, 1000, -1.0f, 1.0f, -250)), -0.25f);
EXPECT_EQ((Canopen::mapValue<int32_t, float>(-1000, 1000, -1.0f, 1.0f, 750)), 0.75f);

// clipping
EXPECT_EQ((Canopen::mapValue<int32_t, float>(0, 1000, 0.0f, 1.0f, -1500)), 0.0f);
EXPECT_EQ((Canopen::mapValue<int32_t, float>(0, 1000, 0.0f, 1.0f, 1500)), 1.0f);

EXPECT_EQ((Canopen::mapValue<int32_t, float>(0, 1000, -1.0f, 1.0f, -1500)), -1.0f);
EXPECT_EQ((Canopen::mapValue<int32_t, float>(0, 1000, -1.0f, 1.0f, 1500)), 1.0f);

EXPECT_EQ((Canopen::mapValue<int32_t, float>(-1000, 1000, 0.0f, 1.0f, -1500)), 0.0f);
EXPECT_EQ((Canopen::mapValue<int32_t, float>(-1000, 1000, 0.0f, 1.0f, 1500)), 1.0f);

EXPECT_EQ((Canopen::mapValue<int32_t, float>(-1000, 1000, -1.0f, 1.0f, -1500)), -1.0f);
EXPECT_EQ((Canopen::mapValue<int32_t, float>(-1000, 1000, -1.0f, 1.0f, 1500)), 1.0f);
}