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
class VIESettings {
    uint32_t xRes;                        ///< Horizontal window resolution
    uint32_t yRes;                        ///< Vertical window resolution

    ///< Lambda for preferred PhysicalDevice choice
    std::function<bool(VkPhysicalDevice &)> preferredDeviceSelectionFunction;
    std::vector<const char *> validationLayers;    ///< Vulkan validation layers for text/debug output

    ///< Vulkan device extensions for additional API support
    std::vector<const char *> deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    VkPresentModeKHR refreshMode;                ///< Frame limiter handler

    //TODO complete documentation
    ///< Default and preferred flag bits for queue family research
    std::vector<VkQueueFlagBits> defaultFlags{VK_QUEUE_GRAPHICS_BIT};
    std::vector<VkQueueFlagBits> preferredFlagBits;

    std::vector<VkFormat> defaultFormats{VK_FORMAT_R8_SRGB, VK_FORMAT_R8G8B8A8_SRGB};
    VkColorSpaceKHR defaultColorSpace{VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

public:
    const std::string kEngineName{"VulkanIndirectEngine"};
    const uint32_t kEngineMajorVersion{1};       ///< Engine major version
    const uint32_t kEngineMinorVersion{0};       ///< Engine minor version
    const uint32_t kEnginePatchVersion{0};       ///< Engine patch version

    const std::string kApplicationName;          ///< Application name
    const uint32_t kApplicationMajorVersion;     ///< Application major version
    const uint32_t kApplicationMinorVersion;     ///< Application minor version
    const uint32_t kApplicationPatchVersion;     ///< Application patch version

    const std::string kApplicationProgramName;   ///< Combined application name and version

    const uint32_t kDefaultXRes;
    const uint32_t kDefaultYRes;


    VIESettings() = delete;

    VIESettings(const std::string &applicationName, uint32_t applicationMajorVersion, uint32_t applicationMinorVersion,
                uint32_t applicationPatchVersion, uint32_t xRes, uint32_t yRes,
                VkPresentModeKHR refreshMode = VK_PRESENT_MODE_IMMEDIATE_KHR)
            : xRes(xRes), yRes(yRes), refreshMode(refreshMode), kApplicationName(applicationName),
              kApplicationMajorVersion(applicationMajorVersion),
              kApplicationMinorVersion(applicationMinorVersion), kApplicationPatchVersion(applicationPatchVersion),
              kApplicationProgramName(fmt::format("{} - {}.{}.{}", applicationName,
                                                  applicationMajorVersion, applicationMinorVersion,
                                                  applicationPatchVersion)),
              kDefaultXRes(xRes), kDefaultYRes(yRes) {}

    VIESettings(const VIESettings &) = default;
    VIESettings(VIESettings &&) = default;
    ~VIESettings() = default;

    template<typename Predicate>
    inline void setPreferredDeviceSelection(Predicate &&predicate) {
        preferredDeviceSelectionFunction = std::forward<Predicate>(predicate);
    }

    void addDeviceExtension(const char *extension);

    void addValidationLayer(const char *validationLayer);

    bool getExtensionsCompatibility(const std::vector<VkExtensionProperties> &availableExtensions);

    template<typename Predicate>
    bool getFlagsCompatibility(Predicate &&predicate) {
        return std::ranges::all_of(defaultFlags, std::forward<Predicate>(predicate)) &&
               std::ranges::all_of(preferredFlagBits, std::forward<Predicate>(predicate));
    }

    bool areValidationLayersEmpty() const;

    template<typename Predicate>
    void eraseValidationLayersIf(Predicate &&predicate) {
        erase_if(validationLayers, std::forward<Predicate>(predicate));
    }

    bool isPreferableDevice(VkPhysicalDevice &device) const;

    std::vector<VkSurfaceFormatKHR>::iterator
    findSurfaceFormat(std::vector<VkSurfaceFormatKHR> &surfaceAvailableFormats) const;

    // -------------------------------------------------------------------------

    uint32_t getDefaultXRes() const;

    uint32_t getDefaultYRes() const;

    VkPresentModeKHR getRefreshMode() const;

    size_t getValidationLayersSize() const;

    const char *const *getValidationLayersData() const;

    size_t getDeviceExtensionsSize() const;

    const char *const *getDeviceExtensionsData() const;
};
