/* Created by LordRibblesdale on 20/02/2022.
 * MIT License
 */

#pragma once

#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>

class VIEScale {
    glm::mat4x4 scaleMatrix;
    float xScale;
    float yScale;
    float zScale;

public:
    const glm::mat4x4 &getScaleMatrix() const {
        return scaleMatrix;
    }

    void setScaleMatrix(const glm::mat4x4 &scaleMatrix) {
        VIEScale::scaleMatrix = scaleMatrix;

        xScale = scaleMatrix[0].x;
        yScale = scaleMatrix[1].y;
        zScale = scaleMatrix[2].z;
    }

    void setScaleMatrix(const glm::vec3 &scaleVector) {
        scaleMatrix = glm::scale(glm::identity<glm::mat4x4>(), scaleVector);

        xScale = scaleMatrix[0].x;
        yScale = scaleMatrix[1].y;
        zScale = scaleMatrix[2].z;
    }

    float getXScale() const {
        return xScale;
    }

    void setXScale(float xScale) {
        VIEScale::xScale = xScale;

        scaleMatrix = glm::scale(glm::identity<glm::mat4x4>(), {xScale, yScale, zScale});
    }

    float getYScale() const {
        return yScale;
    }

    void setYScale(float yScale) {
        VIEScale::yScale = yScale;

        scaleMatrix = glm::scale(glm::identity<glm::mat4x4>(), {xScale, yScale, zScale});
    }

    float getZScale() const {
        return zScale;
    }

    void setZScale(float zScale) {
        VIEScale::zScale = zScale;

        scaleMatrix = glm::scale(glm::identity<glm::mat4x4>(), {xScale, yScale, zScale});
    }
};