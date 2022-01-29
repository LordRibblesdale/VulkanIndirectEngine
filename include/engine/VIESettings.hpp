/* Created by LordRibblesdale on 19/10/2021.
 * MIT License
 */

#pragma once

#include <string>

#include <vector>
#include <functional>
#include <vulkan/vulkan.h>

#define FMT_HEADER_ONLY
#include <fmt/format.h>

#include "VIEStatus.hpp"

/**
 * @brief VIESettings structure for data access around the engine
 */
struct VIESettings {
    std::vector<const char*> validationLayers;    ///< Vulkan validation layers for text/debug output

    ///< Vulkan device extensions for additional API support
    std::vector<const char*> deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    // ----------------------

    uint32_t xRes;                        ///< Horizontal window resolution
    uint32_t yRes;                        ///< Vertical window resolution

    ///< Lambda for preferred PhysicalDevice choice
    std::function<bool(const VkPhysicalDevice &)> preferredDeviceSelectionFunction{};

    VkPresentModeKHR preferredPresentMode;                ///< Frame limiter handler

    //TODO complete documentation
    ///< Default and preferred flag bits for queue family research
    std::vector<VkQueueFlagBits> defaultFlags{VK_QUEUE_GRAPHICS_BIT};
    std::vector<VkQueueFlagBits> preferredFlagBits;

    // VkFormat shadowMappingFormat{VK_FORMAT_R8_SRGB};
    VkFormat defaultFormat{VK_FORMAT_R8G8B8A8_SRGB};
    VkColorSpaceKHR defaultColorSpace{VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};


    const std::string kEngineName{"VulkanIndirectEngine"};
    const uint32_t kEngineVersion{VK_MAKE_API_VERSION(0, 1, 0, 0)};    ///< Engine version

    const std::string kApplicationName;          ///< Application name
    const uint32_t kAppMajorVersion;             ///< Application major version
    const uint32_t kAppMinorVersion;             ///< Application minor version
    const uint32_t kAppPatchVersion;             ///< Application patch version
    const uint32_t kApplicationVersion;

    const std::string kApplicationProgramName;   ///< Combined application name and version

    const uint32_t kDefaultXRes;
    const uint32_t kDefaultYRes;

    VIESettings() = delete;

    VIESettings(const std::string &applicationName, uint32_t appMajorVersion, uint32_t appMinorVersion,
                uint32_t appPatchVersion, uint32_t xRes, uint32_t yRes,
                VkPresentModeKHR refreshMode = VK_PRESENT_MODE_IMMEDIATE_KHR)
            : xRes(xRes), yRes(yRes), preferredPresentMode(refreshMode), kApplicationName(applicationName),
              kAppMajorVersion(appMajorVersion), kAppMinorVersion(appMinorVersion), kAppPatchVersion(appPatchVersion),
              kApplicationVersion(VK_MAKE_API_VERSION(0, appMajorVersion, appMinorVersion, appPatchVersion)),
              kApplicationProgramName(fmt::format("{} - {}.{}.{}", applicationName, appMajorVersion, appMinorVersion,
                                                  appPatchVersion)), kDefaultXRes(xRes), kDefaultYRes(yRes) {}

    VIESettings(const VIESettings &) = default;
    VIESettings(VIESettings &&) = default;
    ~VIESettings() = default;

    template<typename Predicate>
    inline void setPreferredDeviceSelection(Predicate &&predicate) {
        preferredDeviceSelectionFunction = std::forward<Predicate>(predicate);
    }
};
