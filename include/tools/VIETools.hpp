/* Created by LordRibblesdale on 29/01/2022.
 * MIT License
 */

#pragma once

#include <vulkan/vulkan.h>
#include <string_view>
#include <iostream>
#include <vector>
#include <algorithm>

inline constexpr auto kUint32Max{std::numeric_limits<uint32_t>::max()};

namespace logger {
    class Logger {

    };
}

namespace tools {
    inline bool selectSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableSurfaceFormats,
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

    inline VkPresentModeKHR selectSurfacePresentation(const std::vector<VkPresentModeKHR> &availablePresentationModes,
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
