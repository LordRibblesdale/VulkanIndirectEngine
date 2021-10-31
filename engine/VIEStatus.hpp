/* Created by LordRibblesdale on 19/10/2021.
 * MIT License
 */

#pragma once

enum FrameLimitHandler {
    IMMEDIATE,          ///< A frame is sent right away whenever it is ready
    VSYNC,              ///< A frame is sent only when a vertical sync happens
    TRIPLE_BUFFER,      /**< A frame is sent only when a vertical sync happens and updating older frames if present in
                         *    buffer*/
};

/**
 * VIEStatus enumerator for engine status
 */
enum class VIEStatus : unsigned int {
    UNINITIALISED                       = 10,    ///< Uninitialised state
    SETTINGS_LOADED                     = 11,    ///< Settings-only initialised state
    GLFW_LOADED                         = 12,    ///< GLFW window (without API) initialised state
    VULKAN_SURFACE_CREATED              = 13,    ///< GLFW native OS bing and its Vulkan surface created state
    VULKAN_INSTANCE_CREATED             = 14,    ///< Vulkan instanced program state
    VULKAN_PHYSICAL_DEVICES_PREPARED    = 15,    ///< Vulkan physical device chosen state
    VULKAN_DEVICE_CREATED               = 16,    ///< Vulkan logic device created state
};
