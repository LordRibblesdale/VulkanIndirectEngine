/* Created by LordRibblesdale on 20/02/2022.
 * MIT License
 */

#pragma once

#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>

class VIERotation {
    glm::quat quaternion{1, 0, 0, 0};
    float roll{0};
    float pitch{0};
    float yaw{0};

public:
    VIERotation() = default;

    glm::mat4x4 getRotationMatrix() const {
        return mat4_cast(quaternion);
    }

    const glm::quat &getQuaternion() const {
        return quaternion;
    }

    void setQuaternion(const glm::quat &quaternion) {
        VIERotation::quaternion = quaternion;

        // https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
        // http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/
        // TODO understand why glm::quat generates a 321 rotation, but importing it is necessary a 312 setup
        yaw = glm::roll(quaternion);
        roll = glm::pitch(quaternion);
        pitch = glm::yaw(quaternion);
    }

    float getRoll() const {
        return roll;
    }

    void setRoll(float roll) {
        VIERotation::roll = glm::radians(roll);

        quaternion = glm::quat({VIERotation::roll, pitch, yaw});
    }

    float getPitch() const {
        return pitch;
    }

    void setPitch(float pitch) {
        VIERotation::pitch = glm::radians(pitch);

        quaternion = glm::quat ({roll, VIERotation::pitch, yaw});
    }

    float getYaw() const {
        return yaw;
    }

    void setYaw(float yaw) {
        VIERotation::yaw = glm::radians(yaw);

        quaternion = glm::quat({roll, pitch, VIERotation::yaw});
    }

    auto getAngles() const {
        return glm::degrees<3, float, glm::defaultp>({roll, pitch, yaw});
    }

    void setAngles(const glm::vec3 &angles) {
        roll = angles.x;
        pitch = angles.y;
        yaw = angles.z;

        quaternion = glm::quat({roll, pitch, yaw});
    }
};