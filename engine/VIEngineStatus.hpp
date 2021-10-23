/* Created by LordRibblesdale on 19/10/2021.
 * MIT License
 */

#pragma once

/**
 * VIEngineStatus enumerator for engine status
 */
enum VIEngineStatus : unsigned int {
    UNINITIALISED                       = 0,    ///< Uninitialised state
    SETTINGS_LOADED                     = 1,    ///< Settings-only (via XML) initialised state
    GLFW_LOADED                         = 2,    ///< GLFW window (without API) initialised state
    VULKAN_INSTANCE_CREATED             = 3,    ///< Vulkan instanced program state
    VULKAN_PHYSICAL_DEVICES_PREPARED    = 4,    ///< Vulkan physical device chosen state
    VULKAN_DEVICE_CREATED               = 5,    ///< Vulkan logic device created state
};
