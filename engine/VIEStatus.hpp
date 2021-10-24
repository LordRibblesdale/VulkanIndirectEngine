/* Created by LordRibblesdale on 19/10/2021.
 * MIT License
 */

#pragma once

/**
 * VIEStatus enumerator for engine status
 */
enum VIEStatus : unsigned int {
    UNINITIALISED                       = 0,    ///< Uninitialised state
    SETTINGS_LOADED                     = 1,    ///< Settings-only (via XML) initialised state
    GLFW_LOADED                         = 2,    ///< GLFW window (without API) initialised state
    WINDOW_SURFACE_CREATED              = 3,    ///< GLFW native OS bing and its Vulkan surface created state
    VULKAN_INSTANCE_CREATED             = 4,    ///< Vulkan instanced program state
    VULKAN_PHYSICAL_DEVICES_PREPARED    = 5,    ///< Vulkan physical device chosen state
    VULKAN_DEVICE_CREATED               = 6,    ///< Vulkan logic device created state
};
