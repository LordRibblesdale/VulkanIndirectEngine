/* Created by LordRibblesdale on 20/02/2022.
 * MIT License
 */

#pragma once

#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

struct VIEVertex {
    glm::vec3 pos;
    glm::vec3 normal;

    glm::vec2 uvCoords;

    glm::vec3 tangent;
    glm::vec3 bitangent;

    void addDataToVector(std::vector<float> &vector) const {
        vector.insert(vector.end(), {pos.x, pos.y, pos.z, normal.x, normal.y, normal.z, uvCoords.s, uvCoords.t,
                                      tangent.x, tangent.y, tangent.z, bitangent.x, bitangent.y, bitangent.z});
    }
};