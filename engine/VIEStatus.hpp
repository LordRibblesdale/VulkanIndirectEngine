/* Created by LordRibblesdale on 19/10/2021.
 * MIT License
 */

#pragma once

#include <iostream>

/**
 * VIEStatus enumerator for engine status
 */
enum class VIEStatus : uint32_t {
    UNINITIALISED                       = 10,   ///< Uninitialised state
    SETTINGS_LOADED                     = 11,   ///< VIESettings-only initialised state
    GLFW_LOADED                         = 12,   ///< GLFW window (without API) initialised state
    VULKAN_SURFACE_CREATED              = 13,   ///< GLFW native OS bing and its Vulkan surface created state
    VULKAN_INSTANCE_CREATED             = 14,   ///< Vulkan instanced program state
    VULKAN_PHYSICAL_DEVICES_PREPARED    = 15,   ///< Vulkan physical device chosen state
    VULKAN_DEVICE_CREATED               = 16,   ///< Vulkan logic device created state
    VULKAN_SWAP_CHAIN_CREATED           = 17,   ///< Vulkan device swap chain created state
    VULKAN_IMAGE_VIEWS_CREATED          = 18,   ///< Vulkan image views created state
};

inline std::string fromVIEStatusToString(VIEStatus status) {
    switch (status) {
        case VIEStatus::UNINITIALISED:
            return "UNINITIALISED";
        case VIEStatus::SETTINGS_LOADED:
            return "SETTINGS_LOADED";
        case VIEStatus::GLFW_LOADED:
            return "GLFW_LOADED";
        case VIEStatus::VULKAN_SURFACE_CREATED:
            return "VULKAN_SURFACE_CREATED";
        case VIEStatus::VULKAN_INSTANCE_CREATED:
            return "VULKAN_INSTANCE_CREATED";
        case VIEStatus::VULKAN_PHYSICAL_DEVICES_PREPARED:
            return "VULKAN_PHYSICAL_DEVICES_PREPARED";
        case VIEStatus::VULKAN_DEVICE_CREATED:
            return "VULKAN_DEVICE_CREATED";
        case VIEStatus::VULKAN_SWAP_CHAIN_CREATED:
            return "VULKAN_SWAP_CHAIN_CREATED";
        case VIEStatus::VULKAN_IMAGE_VIEWS_CREATED:
            return "VULKAN_IMAGE_VIEWS_CREATED";
        default:
            return "[ERROR: VIEStatus not recognised]";
    }
}