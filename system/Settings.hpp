/* Created by LordRibblesdale on 19/10/2021.
 * MIT License
 */

#pragma once

#include <string>

#include <vector>
#include <functional>
#include <vulkan/vulkan.h>

#include "../engine/VIEStatus.hpp"

// TODO make variables private?
// TODO move all to VIESettings and provide setters for variables and engine state
/**
 * @brief Settings structure for data access around the engine
 */
struct Settings {
    inline static std::string engineName;                       ///< Engine name
    inline static std::string engineProgramName;                ///< Combined engine name and version
    inline static unsigned int engineMajorVersion;              ///< Engine major version
    inline static unsigned int engineMinorVersion;              ///< Engine minor version
    inline static unsigned int enginePatchVersion;              ///< Engine patch version

    inline static unsigned int xRes;                            ///< Horizontal window resolution
    inline static unsigned int yRes;                            ///< Vertical window resolution

    inline static FrameLimitHandler frameHandler;               ///< Frame limiter handler

    ///< Program status
    inline static VIEStatus engineStatus = VIEStatus::UNINITIALISED;
    inline static std::vector<const char*> validationLayers;    ///< Vulkan validation layers for text/debug output
    inline static std::vector<const char*> deviceExtensions;    ///< Vulkan device extensions for additional API support

    inline static bool checkPreferredGPUProperties;             ///< GPU properties references to be checked for a device

    ///< Preferred flag bits for queue family research
    inline static std::vector<VkQueueFlagBits> preferredFlagBits;

    ///< Lambda for preferred PhysicalDevice choice
    inline static std::function<bool(VkPhysicalDevice&)> preferredDeviceSelectionFunction;

    //TODO complete documentation
    inline static std::vector<VkQueueFlagBits> defaultFlags;
    inline static std::vector<VkFormat> defaultFormats;
    inline static VkColorSpaceKHR defaultColorSpace;
};