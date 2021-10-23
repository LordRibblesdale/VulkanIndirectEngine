/* Created by LordRibblesdale on 16/10/2021.
 * MIT License
 */

#pragma once

// Including GLFW window manager
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Including Vulkan graphic libraries
#include <vulkan/vulkan.hpp>

/**
 * @brief VIEngine class representing Vulkan engine
 */
class VIEngine {
    GLFWwindow* mainWindow;

    VkInstance mainInstance;
    VkApplicationInfo applicationInfo;
    VkInstanceCreateInfo engineCreationInfo;

    unsigned int glfwExtensionCount;
    const char** glfwExtensions;

public:
    /**
     * @brief VIEngine::initialiseGLFW for GLFW window manager initialisation
     */
    void initialiseGLFW();

    /**
     * @brief VIEngine::initialiseVulkanLibraries for Vulkan structures initialisation
     */
    void initialiseVulkanLibraries();

    /**
     * @brief VIEngine::preparePhysicalDevice for Vulkan rendering device assignment
     */
    void preparePhysicalDevice();

    /**
     * @brief VIEngine::cleanEngine for cleaning all structures and window pointers
     */
    void cleanEngine();

    /**
     * @brief VIEngine::runEngine for running up all processes (initialisation, preparation, running, cleaning)
     */
    void runEngine();
};