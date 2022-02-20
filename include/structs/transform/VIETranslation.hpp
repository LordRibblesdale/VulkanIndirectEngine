/* Created by LordRibblesdale on 20/02/2022.
 * MIT License
 */

#pragma once

#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>

class VIETranslation {
    glm::mat4x4 translationMatrix;
    float xTranslation;
    float yTranslation;
    float zTranslation;

public:
    const glm::mat4x4 &getTranslationMatrix() const {
        return translationMatrix;
    }

    void setTranslationMatrix(const glm::mat4x4 &translationMatrix) {
        VIETranslation::translationMatrix = translationMatrix;

        xTranslation = translationMatrix[3].x;
        yTranslation = translationMatrix[3].y;
        zTranslation = translationMatrix[3].z;
    }

    void setTranslationMatrix(const glm::vec3 &translationVector) {
        translationMatrix = glm::translate(glm::identity<glm::mat4x4>(), translationVector);

        xTranslation = translationVector.x;
        yTranslation = translationVector.y;
        zTranslation = translationVector.z;
    }

    float getX() const {
        return xTranslation;
    }

    void setX(float x) {
        VIETranslation::xTranslation = x;

        translationMatrix = glm::translate(glm::identity<glm::mat4x4>(), {x, yTranslation, zTranslation});
    }

    float getY() const {
        return yTranslation;
    }

    void setY(float y) {
        VIETranslation::yTranslation = y;

        translationMatrix = glm::translate(glm::identity<glm::mat4x4>(), {xTranslation, y, zTranslation});
    }

    float getZ() const {
        return zTranslation;
    }

    void setZ(float z) {
        VIETranslation::zTranslation = z;

        translationMatrix = glm::translate(glm::identity<glm::mat4x4>(), {xTranslation, yTranslation, z});
    }
};

