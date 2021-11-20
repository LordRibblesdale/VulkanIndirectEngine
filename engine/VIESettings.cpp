/* Created by LordRibblesdale on 24/10/2021.
 * MIT License
 */

#include "VIESettings.hpp"

void VIESettings::addDeviceExtension(const char* extension) {
    deviceExtensions.push_back(extension);
}

void VIESettings::addValidationLayer(const char* validationLayer) {
    validationLayers.push_back(validationLayer);
}

bool VIESettings::getExtensionsCompatibility(const std::vector<VkExtensionProperties> &availableExtensions) {
    return std::ranges::all_of(
            deviceExtensions,
            [&availableExtensions](const char*& extension) {
                std::string extensionStr(extension);
                auto foundExtension =
                        std::ranges::find_if(availableExtensions,
                                             [&extensionStr](const VkExtensionProperties& extensionProperties) {
                                                 return extensionStr == extensionProperties.extensionName;
                                             });

                return foundExtension != availableExtensions.end();
            }
    );
}

bool VIESettings::areValidationLayersEmpty() const {
    return validationLayers.empty();
}

bool VIESettings::isPreferableDevice(VkPhysicalDevice& device) const {
    return preferredDeviceSelectionFunction && preferredDeviceSelectionFunction(device);
}

const std::string &VIESettings::getEngineName() const {
    return engineName;
}

uint32_t VIESettings::getEngineMajorVersion() const {
    return engineMajorVersion;
}

uint32_t VIESettings::getEngineMinorVersion() const {
    return engineMinorVersion;
}

uint32_t VIESettings::getEnginePatchVersion() const {
    return enginePatchVersion;
}

const std::string &VIESettings::getEngineProgramName() const {
    return engineProgramName;
}

uint32_t VIESettings::getDefaultXRes() const {
    return defaultXRes;
}

uint32_t VIESettings::getDefaultYRes() const {
    return defaultYRes;
}

VkPresentModeKHR VIESettings::getRefreshMode() const {
    return refreshMode;
}

std::vector<VkSurfaceFormatKHR>::iterator
VIESettings::findSurfaceFormat(std::vector<VkSurfaceFormatKHR>& surfaceAvailableFormats) const {
    return std::ranges::find_if(
            surfaceAvailableFormats,
            [this](const VkSurfaceFormatKHR& surfaceFormat) {
                return (std::ranges::find(defaultFormats, surfaceFormat.format) != defaultFormats.end()) &&
                       surfaceFormat.colorSpace == defaultColorSpace;
            });
}

size_t VIESettings::getValidationLayersSize() const {
    return validationLayers.size();
}

const char *const *VIESettings::getValidationLayersData() const {
    return validationLayers.data();
}

size_t VIESettings::getDeviceExtensionsSize() const {
    return deviceExtensions.size();
}

const char *const *VIESettings::getDeviceExtensionsData() const {
    return deviceExtensions.data();
}
