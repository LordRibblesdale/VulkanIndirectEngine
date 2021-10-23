/* Created by LordRibblesdale on 19/10/2021.
 * MIT License
 */

#pragma once

#include <string>

#define FMT_HEADER_ONLY
#include <fmt/format.h>
#include <vulkan/vulkan.h>

#include "../engine/VIEngineStatus.hpp"


struct Settings {
    inline static std::string engineName;                       ///< Engine name
    inline static std::string engineProgramName;                ///< Combined engine name and version
    inline static unsigned int engineMajorVersion;              ///< Engine major version
    inline static unsigned int engineMinorVersion;              ///< Engine minor version
    inline static unsigned int enginePatchVersion;              ///< Engine patch version

    inline static unsigned int xRes;                            ///< Horizontal window resolution
    inline static unsigned int yRes;                            ///< Vertical window resolution

    inline static VIEngineStatus engineStatus;                  ///< Program status
    inline static std::vector<const char*> validationLayers;    ///< Vulkan validation layers for text/debug output

    inline static bool checkPreferredGPUProperties;             ///< GPU properties references to be checked for a device

    ///< GPU preferred type
    inline static VkPhysicalDeviceType preferredGPUType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
};