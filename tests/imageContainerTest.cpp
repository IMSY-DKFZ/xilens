/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/

#include <gtest/gtest.h>

#include "src/imageContainer.h"
#include "mocks.h"


/**
 * Test that image container is capable of returning an image
 */
TEST(ImageContainer, GetCurrentImageReturnsValidImage) {
    std::shared_ptr<MockXiAPIWrapper> apiWrapper = std::make_shared<MockXiAPIWrapper>();
    auto imageContainer = ImageContainer();
    imageContainer.Initialize(apiWrapper);
    auto image = imageContainer.GetCurrentImage();

    ASSERT_EQ(typeid(image).name(), typeid(XI_IMG).name());
}
