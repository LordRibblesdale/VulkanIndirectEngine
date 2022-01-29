/* Created by LordRibblesdale on 19/10/2021.
 * MIT License
 */

#pragma once

#include <iostream>

/**
 * VIEStatus enumerator for engine status
 * Up to 256 status
 */
enum class VIEStatus : uint8_t {
    UNINITIALISED                       = 10,   ///< Uninitialised state
    SETTINGS_LOADED                     = 11,   ///< VIESettings-only initialised state
    GLFW_LOADED                         = 12,   ///< GLFW window (without API) initialised state
    VULKAN_SURFACE_CREATED              = 13,   ///< GLFW native OS bing and its Vulkan surface created state
    VULKAN_INSTANCE_CREATED             = 14,   ///< Vulkan instanced program state
    VULKAN_PHYSICAL_DEVICES_PREPARED    = 15,   ///< Vulkan physical device chosen state
    VULKAN_DEVICE_CREATED               = 16,   ///< Vulkan logic device created state
    VULKAN_SWAP_CHAIN_CREATED           = 17,   ///< Vulkan device swap chain created state
    VULKAN_IMAGE_VIEWS_CREATED          = 18,   ///< Vulkan image views created state
    VULKAN_SHADERS_COMPILED             = 19,   ///< Vulkan graphics pipeline generated state
    VULKAN_PIPELINE_STATES_PREPARED     = 20,   ///<
    VULKAN_RENDER_PASSES_GENERATED      = 21,   ///<
    VULKAN_GRAPHICS_PIPELINE_GENERATED  = 22,   ///<
    VULKAN_FRAMEBUFFERS_CREATED         = 23,   ///<
    VULKAN_COMMAND_POOL_CREATED         = 24,   ///<
    VULKAN_COMMAND_BUFFERS_PREPARED     = 25,   ///<
    VULKAN_SEMAPHORES_CREATED           = 26,   ///<
};

// inline std::string fromVIEStatusToString(VIEStatus status) {
inline std::ostream& operator<<(std::ostream& ostream, VIEStatus status) {
    switch (status) {
        case VIEStatus::UNINITIALISED:
            return ostream << "UNINITIALISED";
        case VIEStatus::SETTINGS_LOADED:
            return ostream << "SETTINGS_LOADED";
        case VIEStatus::GLFW_LOADED:
            return ostream << "GLFW_LOADED";
        case VIEStatus::VULKAN_SURFACE_CREATED:
            return ostream << "VULKAN_SURFACE_CREATED";
        case VIEStatus::VULKAN_INSTANCE_CREATED:
            return ostream << "VULKAN_INSTANCE_CREATED";
        case VIEStatus::VULKAN_PHYSICAL_DEVICES_PREPARED:
            return ostream << "VULKAN_PHYSICAL_DEVICES_PREPARED";
        case VIEStatus::VULKAN_DEVICE_CREATED:
            return ostream << "VULKAN_DEVICE_CREATED";
        case VIEStatus::VULKAN_SWAP_CHAIN_CREATED:
            return ostream << "VULKAN_SWAP_CHAIN_CREATED";
        case VIEStatus::VULKAN_IMAGE_VIEWS_CREATED:
            return ostream << "VULKAN_IMAGE_VIEWS_CREATED";
        case VIEStatus::VULKAN_SHADERS_COMPILED:
            return ostream << "VULKAN_SHADERS_COMPILED";
        default:
            return ostream << "[ERROR: VIEStatus not recognised]";
    }
}