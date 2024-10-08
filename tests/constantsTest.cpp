/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/

#include <gtest/gtest.h>

#include "src/constants.h"

TEST(CameraMapperTest, ReturnsNonEmptyMapper)
{
    auto &mapper = getCameraMapper();

    EXPECT_FALSE(mapper.isEmpty());
}

TEST(CameraMapperTest, ContainsSpecificCamera)
{
    auto &mapper = getCameraMapper();

    EXPECT_TRUE(mapper.contains("MQ022HG-IM-SM4X4-VIS"));
}
