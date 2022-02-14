/* Created by LordRibblesdale on 29/01/2022.
 * MIT License
 */

#pragma once

#include <vulkan/vulkan.h>
#include <string_view>
#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>
#include "engine/VIESettings.hpp"

// TODO fix with logger
#define return_log_if(if_condition, string, ret_condition)  \
if (if_condition) {                                         \
    std::cout << (string) << std::endl;                     \
    return ret_condition;                                   \
}

#define break_if(if_condition)  \
if (if_condition) {             \
    break;                      \
}

#define skip_if(if_condition)   \
if (if_condition) {             \
    continue;                   \
}

constexpr auto kUint32Max{std::numeric_limits<uint32_t>::max()};

namespace debug {
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                        void* pUserData) {
        std::string severity;

        switch (messageSeverity) {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                severity = "VERBOSE";
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                severity = "INFO";
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                severity = "WARNING";
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                severity = "ERROR";
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
                severity = "GENERIC";
                break;
        }

        std::cout << fmt::format("[{}] Validation layer: {}", severity, pCallbackData->pMessage) << std::endl;

        return VK_FALSE;
    }
}

namespace tools {
    template <typename VkFunc, typename Vector, typename... Args>
    inline void gatherVkData(VkFunc &vkCall, Vector &v, uint32_t &count, Args &&...requiredArgs) {
        vkCall(requiredArgs..., &count, nullptr);
        v.resize(count);
        vkCall(requiredArgs..., &count, v.data());
    }

    bool selectSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableSurfaceFormats,
                             const VkFormat &requiredFormat, const VkColorSpaceKHR &requiredColorSpace,
                             VkSurfaceFormatKHR &returnSurfaceFormat);

    VkPresentModeKHR selectSurfacePresentation(const std::vector<VkPresentModeKHR> &availablePresentationModes,
                                               const VkPresentModeKHR &requiredPresentationMode);

    bool selectPhysicalDevice(const VkPhysicalDevice &deviceToCheck, VkPhysicalDevice &compatibleDevice,
                              uint32_t &selectedQueueFamily, uint32_t &selectedPresentFamily,
                              const VkSurfaceKHR &surface, VkSurfaceCapabilitiesKHR &surfaceCapabilities,
                              std::vector<VkSurfaceFormatKHR> &formats,
                              std::vector<VkPresentModeKHR> &presentationModes,
                              const VIESettings &settings);
}
