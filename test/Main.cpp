/* Created by LordRibblesdale on 16/10/2021.
 * MIT License
 */

#include "../engine/VIESettings.hpp"
#include "../engine/VIEngine.hpp"

/**
 * @brief Main function of IndirectEngine
 * @param argc number of starting arguments
 * @param argv list of arguments given
 * @return execution return code
 */
int main(int argc, char** argv) {
    // Reading settings
    VIESettings settings("VulkanIndirectEngine", 1, 0, 0, 1920, 1080, VK_PRESENT_MODE_IMMEDIATE_KHR);

    settings.setPreferredDeviceSelection([](VkPhysicalDevice& device) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    });

    settings.addValidationLayer("VK_LAYER_KHRONOS_validation");

    // TODO create an input structure asking engine to create shaders from locations defined by user
    //  like: struct VIEShaderRequestEntry { str: vert.location, str: frag.location }

    // Initialising engine
    std::cout << "Sizeof VIEngine: " << sizeof(VIEngine) << " bytes" << std::endl;
    std::cout << "Sizeof VIESettings: " << sizeof(VIESettings) << " bytes" << std::endl;
    auto engine(std::make_unique<VIEngine>(settings));
    engine->prepareEngine();
    engine.reset();
}
