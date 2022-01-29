/* Created by LordRibblesdale on 29/01/2022.
 * MIT License
 */

#pragma once

#include <vulkan/vulkan.h>
#include <string_view>
#include <iostream>
#include <vector>
#include <algorithm>

namespace logger {
    class Logger {

    };
}

namespace tools {
    void checkValidationLayersCompatibility(std::vector<const char*> &validationLayers) {
        // Getting number of available layers
        uint32_t availableInSystemLayersNum = 0;
        vkEnumerateInstanceLayerProperties(&availableInSystemLayersNum, nullptr);

        // Getting all layers, now known the number of layers
        std::vector<VkLayerProperties> availableInSystemLayers(availableInSystemLayersNum);
        vkEnumerateInstanceLayerProperties(&availableInSystemLayersNum, availableInSystemLayers.data());

        erase_if(validationLayers,
                 [&availableInSystemLayers](std::string_view requestedLayer) {
                     auto foundLayer = std::ranges::find_if(availableInSystemLayers,
                                                            [&requestedLayer](const VkLayerProperties &properties) {
                                                                return requestedLayer == properties.layerName;
                                                            });

                     return foundLayer == availableInSystemLayers.end();
                 });
    }

    /**
     * @brief checkDeviceExtensionSupport checks if a device supports the defined list of Vulkan extensions
     * @param device VkPhysicalDevice to be checked
     * @return true if the device is compatible, false otherwise
     */
    bool checkDeviceExtensionSupport(const VkPhysicalDevice &device, const std::vector<const char*> &deviceExtensions) {
        uint32_t extensionCount;
        // Getting number of supported extensions
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        // Getting supported extensions
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        return std::ranges::all_of(deviceExtensions,
                                   [&availableExtensions](std::string_view extension) {
                                       return std::ranges::any_of(
                                               availableExtensions,
                                               [&extension](const VkExtensionProperties &extensionProperties) {
                                                   return extension == extensionProperties.extensionName;
                                               });
                                   });
    }

    bool selectSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableSurfaceFormats,
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

    VkPresentModeKHR selectSurfacePresentation(const std::vector<VkPresentModeKHR> &availablePresentationModes,
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
}
