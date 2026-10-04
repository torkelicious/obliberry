#include "Math/GLMUtils.h"

//
// Some helpers used for unit tests.
//
namespace TestUtils {

    // used for glm::vec types.
    template <Math::GLMUtils::GlmFloatVec T> void GLM_VecExpectFloat(const T &actual, const T &expected) {
        for (glm::length_t i = 0; i < actual.length(); ++i) {
            using Scalar = typename T::value_type;

            if constexpr (std::same_as<Scalar, float>) {
                EXPECT_FLOAT_EQ(actual[i], expected[i]);
            } else {
                EXPECT_DOUBLE_EQ(actual[i], expected[i]);
            }
        }
    }

    template <Math::GLMUtils::GlmFloatMat T> void GLM_MatExpectNear(const T &actual, const T &expected, typename T::value_type tolerance = typename T::value_type(1e-5)) {
        for (glm::length_t col = 0; col < actual.length(); ++col) {
            for (glm::length_t row = 0; row < actual[col].length(); ++row) {
                EXPECT_NEAR(actual[col][row], expected[col][row], tolerance) << " [column " << col << ", row " << row << "]";
            }
        }
    }

} // namespace TestUtils
