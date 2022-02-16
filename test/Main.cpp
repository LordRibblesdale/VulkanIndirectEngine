/* Created by LordRibblesdale on 16/10/2021.
 * MIT License
 */

#include "engine/VIESettings.hpp"
#include "engine/VIEngine.hpp"

int main(int argc, char** argv) {
    // Initialising engine
    std::cout << "Sizeof VIEngine: " << sizeof(VIEngine) << " bytes" << std::endl;
    std::cout << "Sizeof VIESettings: " << sizeof(VIESettings) << " bytes" << std::endl;
    auto engine(std::make_unique<VIEngine>(VIESettings("./data/settings.xml")));
    engine->prepareEngine();
    engine->runEngine();
    engine.reset();
}
