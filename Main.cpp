/* Created by LordRibblesdale on 16/10/2021.
 * MIT License
 */

#include "system/Settings.hpp"
#include "engine/VIEngine.hpp"

#define FMT_HEADER_ONLY
#include <fmt/format.h>

/**
 * Main function of IndirectEngine
 * @param argc number of starting arguments
 * @param argv list of arguments given
 * @return execution return code
 */
int main(int argc, char** argv) {
    // Predeclaring variables
    VIEngine engine{};

    // Reading settings
    Settings::engineName = "VulkanIndirectEngine";
    Settings::engineMajorVersion = 1;
    Settings::engineMinorVersion = 0;
    Settings::enginePatchVersion = 0;
    Settings::engineProgramName = fmt::format("{} - {}.{}.{}", Settings::engineName, Settings::engineMajorVersion,
                                              Settings::engineMinorVersion, Settings::enginePatchVersion);
    Settings::xRes = 1920;
    Settings::yRes = 1080;

    Settings::refreshMode = VK_PRESENT_MODE_IMMEDIATE_KHR;

    Settings::checkPreferredGPUProperties = true;

    if (Settings::checkPreferredGPUProperties) {
        Settings::preferredDeviceSelectionFunction = [](VkPhysicalDevice& device) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);

            return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        };
    }

    if (true) {
        // TODO get vector size and reserve before inserting
        Settings::validationLayers = {
                "VK_LAYER_KHRONOS_validation"
        };
    }

    Settings::engineStatus = VIEStatus::SETTINGS_LOADED;

    // Initialising engine
    engine.prepareEngine();
}
