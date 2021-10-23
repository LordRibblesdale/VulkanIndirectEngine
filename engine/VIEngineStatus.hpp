/* Created by LordRibblesdale on 19/10/2021.
 * MIT License
 */

#pragma once

/**
 * VIEngineStatus enumerator for engine status
 */
enum VIEngineStatus : unsigned int {
    UNINITIALISED               = 0,
    SETTINGS_LOADED             = 1,
    GLFW_LOADED                 = 2,
    VULKAN_INSTANCE_CREATED     = 3,
};
