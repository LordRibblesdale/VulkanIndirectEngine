#include "tools/VIETools.hpp"

bool tools::selectSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableSurfaceFormats,
                                const VkFormat &requiredFormat, const VkColorSpaceKHR &requiredColorSpace,
                                VkSurfaceFormatKHR &returnSurfaceFormat) {
    if (availableSurfaceFormats.empty()) {
        return false;
    }

    auto foundSurfaceFormat(std::ranges::any_of(
            availableSurfaceFormats,
            [&requiredFormat, &requiredColorSpace](const VkSurfaceFormatKHR &surfaceFormat) {
                return surfaceFormat.format == requiredFormat && surfaceFormat.colorSpace == requiredColorSpace;
            }));

    returnSurfaceFormat = foundSurfaceFormat ? VkSurfaceFormatKHR{requiredFormat, requiredColorSpace}
                                             : availableSurfaceFormats.at(0);

    return true;
}

VkPresentModeKHR tools::selectSurfacePresentation(const std::vector<VkPresentModeKHR> &availablePresentationModes,
                                                  const VkPresentModeKHR &requiredPresentationMode) {
    if (availablePresentationModes.empty()) {
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    auto foundPresentationMode(std::ranges::any_of(
            availablePresentationModes,
            [&requiredPresentationMode](const VkPresentModeKHR &presentModeKhr) {
                return presentModeKhr == requiredPresentationMode;
            }));

    return foundPresentationMode ? requiredPresentationMode : VK_PRESENT_MODE_FIFO_KHR;
}

bool tools::selectPhysicalDevice(VkPhysicalDevice const &deviceToCheck, VkPhysicalDevice &compatibleDevice,
                                 uint32_t &selectedQueueFamily, uint32_t &selectedPresentFamily,
                                 VkSurfaceKHR const &surface, VkSurfaceCapabilitiesKHR &surfaceCapabilities,
                                 std::vector<VkSurfaceFormatKHR> &formats,
                                 std::vector<VkPresentModeKHR> &presentationModes, const VIESettings &settings) {
    // Getting number of supported extensions
    uint32_t extensionCount;
    std::vector<VkExtensionProperties> availableExtensions;
    tools::gatherVkData(vkEnumerateDeviceExtensionProperties, availableExtensions, extensionCount, deviceToCheck,
                        nullptr);

    // Checking that all requested deviceToCheck extensions are compatible with the selected vkDevice
    if (!std::ranges::all_of(settings.deviceExtensions,
                             [&availableExtensions](std::string_view extension) {
                                 return std::ranges::any_of(
                                         availableExtensions,
                                         [&extension](const VkExtensionProperties &extensionProperties) {
                                             return extension == extensionProperties.extensionName;
                                         });
                             })) {
        return false;
    }

    // Getting number of supported queue families (subset of compatible queues with the physical deviceToCheck)
    uint32_t queueFamilyCount = 0;
    std::vector<VkQueueFamilyProperties> deviceQueueFamilies;
    tools::gatherVkData(vkGetPhysicalDeviceQueueFamilyProperties, deviceQueueFamilies, queueFamilyCount, deviceToCheck);

    // Checking if there is the interested queue family with requested flags and with selectedSurface support
    bool isDeviceQueueFamilyCompatible = false;
    uint32_t mainDeviceSelectedQueueFamily = kUint32Max;
    uint32_t mainDeviceSelectedPresentFamily = kUint32Max;
    for (uint32_t selectedIndex = 0; const VkQueueFamilyProperties &property: deviceQueueFamilies) {
        auto containsFlags([&property](const auto &flagBits) {
            return property.queueFlags & flagBits;
        });

        // If compatible, queue subset family index is saved
        if (std::ranges::all_of(settings.defaultFlags, containsFlags) &&
            std::ranges::all_of(settings.preferredFlagBits, containsFlags)) {
            mainDeviceSelectedQueueFamily = selectedIndex;
        }

        VkBool32 isSurfaceSupported = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(deviceToCheck, selectedIndex, surface, &isSurfaceSupported);

        if (isSurfaceSupported) {
            // If queue subset index is also compatible with current surface, its index is saved
            mainDeviceSelectedPresentFamily = selectedIndex;
        }

        if ((mainDeviceSelectedQueueFamily != kUint32Max) && (mainDeviceSelectedPresentFamily != kUint32Max)) {
            isDeviceQueueFamilyCompatible = true;
            break;
        }

        ++selectedIndex;
    }

    if (!isDeviceQueueFamilyCompatible) {
        return false;
    }

    // Gathering the number of supported surface formats
    uint32_t formatCount;
    uint32_t presentModeCount;
    tools::gatherVkData(vkGetPhysicalDeviceSurfaceFormatsKHR, formats, formatCount, deviceToCheck,
                        surface);
    tools::gatherVkData(vkGetPhysicalDeviceSurfacePresentModesKHR, presentationModes, presentModeCount,
                        deviceToCheck, surface);

    if ((formatCount == 0) || (presentModeCount == 0)) {
        return false;
    }

    compatibleDevice = deviceToCheck;
    // Selecting queue graphic
    selectedQueueFamily = mainDeviceSelectedQueueFamily;
    selectedPresentFamily = mainDeviceSelectedPresentFamily;

    // Obtaining all capabilities of a surface, depending on the linked deviceToCheck
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(deviceToCheck, surface, &surfaceCapabilities);

    return true;
}
