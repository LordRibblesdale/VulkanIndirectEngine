/* Created by LordRibblesdale on 16/10/2021.
 * MIT License
 */

#pragma once

// Including GLFW window manager
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Including Vulkan graphic libraries
#include <vulkan/vulkan.hpp>

#include <unordered_map>

/**
 * @brief VIEngine class representing Vulkan engine
 */
class VIEngine {
    GLFWwindow* mainWindow;                         ///< GLFW window pointer

    VkInstance mainInstance;                        ///< Vulkan runtime instance
    VkApplicationInfo applicationInfo;              ///< Vulkan application data
    VkInstanceCreateInfo engineCreationInfo;        ///< Vulkan essential data for creation driving

    VkPhysicalDevice mainPhysicalDevice;            ///< Vulkan physical device object (for actual device representation)

    std::optional<unsigned int> mainDeviceSelectedQueueFamily;  ///< Queue family chosen for the main device
    VkDevice mainDevice;                    ///< Vulkan logical device object (for state, resources used by instance)

    unsigned int glfwExtensionCount;        ///< GLFW extensions count for Vulkan ext. initialisation
    const char** glfwExtensions;            ///< GLFW extensions (APIs) to be used by Vulkan for interacting with window

    /**
     * @brief VIEngine::isDeviceCompliantToQueueFamilies for device validation in terms of operation queue families
     * @tparam VkQueueFlagBits VkQueueFlagBits parameter pack
     * @param queueFamilyIndex
     * @param device VkPhysicalDevice to be checked
     * @param flags VkQueueFlagBits to be checked if available for the given device
     * @return true if the device is valid, false otherwise
     */
    template<typename ... VkQueueFlagBits>
    static bool isDeviceCompliantToQueueFamilies(const VkPhysicalDevice &device,
                                                 std::optional<unsigned int>& queueFamilyIndex,
                                                 VkQueueFlagBits&&... flags);
public:
    /**
     * @brief VIEngine::initialiseGLFW for GLFW window manager initialisation
     * A GLFW window is initialised by glfwInit and main properties are assigned to it. The program status is changed
     *  accordingly.
     * Note: This function can be called only if the system has only main settings in order to initialise the window
     */
    void initialiseGLFW();

    /**
     * @brief VIEngine::initialiseVulkanLibraries for Vulkan structures initialisation
     * All main Vulkan objects are initialised (application info, data structure for instance creation instructions,
     *  validation layers (API for text/debug options)).
     * Note: This function can be called only if the system has a GLFW window initialised
     */
    void initialiseVulkanLibraries();

    // TODO import commands required by the used who imports the engine
    // TODO import a lambda function so that everyone can implement how the main rendering cycle will work
    /**
     * @brief VIEngine::preparePhysicalDevices for Vulkan rendering device assignment
     * The main device for drawing is found and checked if it has all needed requirements for used commands in the
     *  application
     */
    void preparePhysicalDevices();

    /**
     * @brief VIEngine::createLogicDevice create the runtime instance of a physical device, for runtime device own state
     */
    void createLogicDevice();

    /**
     * @brief VIEngine::cleanEngine for cleaning all structures and window pointers
     * All structures created during the engine runtime are destroyed at the end.
     */
    void cleanEngine();

    /**
     * @brief VIEngine::runEngine for running up all processes (initialisation, preparation, running, cleaning)
     * This function collects all functions needed for running the engine, from initialisation, to drawing, to cleaning.
     */
    void runEngine();
};