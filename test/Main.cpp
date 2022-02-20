/* Created by LordRibblesdale on 16/10/2021.
 * MIT License
 */

#include "engine/VIESettings.hpp"
#include "engine/VIEngine.hpp"
#include "structs/transform/VIERotation.hpp"

#define FMT_HEADER_ONLY
#include <fmt/format.h>

int main(int argc, char** argv) {
    // Initialising engine
    std::cout << "Sizeof VIEngine: " << sizeof(VIEngine) << " bytes" << std::endl;
    std::cout << "Sizeof VIESettings: " << sizeof(VIESettings) << " bytes" << std::endl;
    auto engine(std::make_unique<VIEngine>(VIESettings("./data/settings.xml")));
    engine->prepareEngine();
    engine->runEngine();
    engine.reset();

    /*
    VIERotation rotation;
    // 321 rotation
    rotation.setRoll(40);
    rotation.setPitch(15.44f);
    rotation.setYaw(-3.88f);

    auto angles(rotation.getAngles());
    std::cout << fmt::format("Roll {}, Pitch {}, Yaw {}\n", angles.x, angles.y, angles.z);
    auto quat(rotation.getQuaternion());
    std::cout << fmt::format("Quaternion {} {} {} {}\n", quat.x, quat.y, quat.z, quat.w);

    rotation.setQuaternion({0.92908657f, 0.3429992f, 0.1146581f, -0.07744095f});
    auto angles2(rotation.getAngles());
    std::cout << fmt::format("Roll {}, Pitch {}, Yaw {}\n", angles2.x, angles2.y, angles2.z);
     */
}
