#pragma once

#include "Math/GLMUtils.h"
#include "glm/glm.hpp"
#include <cassert>
#include <gtest/gtest.h>


//
// Some helpers used for unit tests.
//
namespace TestUtils {

    // used for glm::vec types.
    template <Math::GLMUtils::GlmVec T> void GLM_VecExpectFloat(const T &actual, const T &expected) {

        EXPECT_FLOAT_EQ(actual.length(), expected.length());

        for (glm::length_t i = 0; i < actual.length(); ++i) {
            EXPECT_FLOAT_EQ(actual[i], expected[i]);
        }
    }


} // namespace TestUtils
