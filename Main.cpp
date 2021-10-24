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

    // TODO read arguments

    // Starting from uninitialised state
    Settings::engineStatus = UNINITIALISED;

    // TODO call XML parser
    // Reading settings
    Settings::engineName = "IndirectEngine";
    Settings::engineMajorVersion = 1;
    Settings::engineMinorVersion = 0;
    Settings::enginePatchVersion = 0;
    Settings::engineProgramName = fmt::format("{} - {}.{}.{}", Settings::engineName, Settings::engineMajorVersion,
                                              Settings::engineMinorVersion, Settings::enginePatchVersion);
    Settings::xRes = 1920;
    Settings::yRes = 1080;

    Settings::checkPreferredGPUProperties = true;

    if (Settings::checkPreferredGPUProperties) {
        Settings::preferredDeviceSelectionFunction = [](VkPhysicalDevice& device) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);

            return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        };
    }

    Settings::deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME     // Enabling the possibility to swap buffers via "swap chains"
    };

    if (true) {
        // TODO get vector size and reserve before inserting
        Settings::validationLayers = {
                "VK_LAYER_KHRONOS_validation"
        };
    }

    Settings::engineStatus = SETTINGS_LOADED;

    // Initialising engine
    engine.runEngine();
}
