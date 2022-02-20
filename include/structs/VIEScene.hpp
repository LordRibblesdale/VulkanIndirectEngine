/* Created by LordRibblesdale on 16/02/2022.
 * MIT License
 */

#pragma once

#include <glm/mat4x4.hpp>
#include <memory>

struct VIECamera {
    glm::mat4x4 viewMatrix;
    glm::vec4 center;
    glm::vec4 lookAt;
    glm::vec4 up;
};

class VIEScene {
    std::unique_ptr<VIECamera> screenCamera;
    std::unique_ptr<VIECamera> leftEyeCamera;
    std::unique_ptr<VIECamera> rightEyeCamera;


};
