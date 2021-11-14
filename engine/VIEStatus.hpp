/* Created by LordRibblesdale on 19/10/2021.
 * MIT License
 */

#pragma once

#include <iostream>

/**
 * VIEStatus enumerator for engine status
 */
enum class VIEStatus : unsigned int {
    UNINITIALISED                       = 10,   ///< Uninitialised state
    SETTINGS_LOADED                     = 11,   ///< Settings-only initialised state
    GLFW_LOADED                         = 12,   ///< GLFW window (without API) initialised state
    VULKAN_SURFACE_CREATED              = 13,   ///< GLFW native OS bing and its Vulkan surface created state
    VULKAN_INSTANCE_CREATED             = 14,   ///< Vulkan instanced program state
    VULKAN_PHYSICAL_DEVICES_PREPARED    = 15,   ///< Vulkan physical device chosen state
    VULKAN_DEVICE_CREATED               = 16,   ///< Vulkan logic device created state
    VULKAN_SWAP_CHAIN_CREATED           = 17,   ///< Vulkan device swap chain created state
};

inline std::string fromVIEStatusToString(VIEStatus status) {
    switch (status) {
        case VIEStatus::UNINITIALISED:
            return "Engine status: UNINITIALISED.";
        case VIEStatus::SETTINGS_LOADED:
            return "Engine status: SETTINGS_LOADED.";
        case VIEStatus::GLFW_LOADED:
            return "Engine status: GLFW_LOADED.";
        case VIEStatus::VULKAN_SURFACE_CREATED:
            return "Engine status: VULKAN_SURFACE_CREATED.";
        case VIEStatus::VULKAN_INSTANCE_CREATED:
            return "Engine status: VULKAN_INSTANCE_CREATED.";
        case VIEStatus::VULKAN_PHYSICAL_DEVICES_PREPARED:
            return "Engine status: VULKAN_PHYSICAL_DEVICES_PREPARED.";
        case VIEStatus::VULKAN_DEVICE_CREATED:
            return "Engine status: VULKAN_DEVICE_CREATED.";
        case VIEStatus::VULKAN_SWAP_CHAIN_CREATED:
            return "Engine status: VULKAN_SWAP_CHAIN_CREATED.";
        default:
            return "Engine status: [ERROR: VIEStatus not recognised].";
    }
}