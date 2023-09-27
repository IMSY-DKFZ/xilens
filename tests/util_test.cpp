#include <gtest/gtest.h>

#include "src/util.h"

TEST(UtilTest, HandleResultTest){
    int cam_status = 0;
    ASSERT_NO_THROW(HandleResult(cam_status, "test camera status"));
    cam_status = 1;
    ASSERT_THROW(HandleResult(cam_status, "test camera status"), std::runtime_error);
}
