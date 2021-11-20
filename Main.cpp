/* Created by LordRibblesdale on 16/10/2021.
 * MIT License
 */

#include "engine/VIESettings.hpp"
#include "engine/VIEngine.hpp"

/**
 * Main function of IndirectEngine
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

    // Initialising engine
    VIEngine engine(settings);
    engine.prepareEngine();
}
