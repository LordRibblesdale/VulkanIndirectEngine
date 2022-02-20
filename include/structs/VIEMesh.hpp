/* Created by LordRibblesdale on 20/02/2022.
 * MIT License
 */

#pragma once

#include <vector>

#include "structs/VIEVertex.hpp"
#include "structs/transform/VIETransform.hpp"

class VIEMesh : public VIELocalTransform {
    std::vector<VIEVertex> vertices;
    std::vector<uint32_t> indices;

    // Textures?

public:
    VIEMesh(size_t vertexSize, size_t indicesSize) : vertices(vertexSize), indices(indicesSize) {}
};