/* Created by LordRibblesdale on 16/10/2021.
 * MIT License
 */

#pragma once

// Including Vulkan graphic libraries
#ifdef _WIN64
#define VK_USE_PLATFORM_WIN32_KHR
#elif __linux__
#define VK_USE_PLATFORM_WAYLAND_KHR
#endif
// #include <vulkan/vulkan.hpp>

// Including GLFW window manager
#define GLFW_INCLUDE_VULKAN
#ifdef _WIN64
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#elif __linux__
#define GLFW_EXPOSE_NATIVE_WAYLAND
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <vector>
#include <optional>
#include <iostream>
#include <algorithm>

// TODO write functions for creating additional devices or recreate entirely devices

/**
 * @brief VIEngine class representing Vulkan engine
 */
class VIEngine {
    // TODO check which of these elements could be freed from memory after prepareEngine
    // GLFW
    GLFWwindow* mainWindow{};                               ///< GLFW window pointer
    unsigned int glfwExtensionCount{};                      ///< GLFW extensions count for Vulkan ext. initialisation
    const char** glfwExtensions{};                          /**< GLFW extensions (GLFW APIs and functions) to be used by
                                                             *    Vulkan for interacting with window */

    // Vulkan instance
    VkInstance mainInstance{};                              ///< Vulkan runtime instance
    VkApplicationInfo applicationInfo{};                    ///< Vulkan application data
    VkInstanceCreateInfo engineCreationInfo{};              ///< Vulkan essential data for creation procedure

    // Vulkan physical device
    VkPhysicalDevice mainPhysicalDevice{};                  /**< Vulkan physical device object (for actual device
                                                             *    representation) */
    std::optional<unsigned int> mainDeviceSelectedQueueFamily;    ///< Queue family chosen for the main device
    VkPhysicalDeviceFeatures mainPhysicalDeviceFeatures{};  ///< Main device features to set for chosen device

    // Vulkan logic device
    VkDevice mainDevice{};                                  /**< Vulkan logical device object (for state, resources
                                                             *    used by instance) */
    VkDeviceCreateInfo mainDeviceCreationInfo{};            /**< Vulkan main device object (for logical device
                                                             *    representation) */

    // Vulkan queue family
    VkDeviceQueueCreateInfo mainDeviceQueueCreationInfo{};  /**< Vulkan essential data for device queue family creation
                                                             *    procedure */
    float mainQueueFamilyPriority = 1.0f;                   ///< Main queue family priority

    // Vulkan graphics queue
    VkQueue graphicsQueue{};                                ///< Main rendering queue

    // Vulkan window surface
    VkSurfaceKHR surface{};                                 ///< Window surface for GLFW
    VkSurfaceCapabilitiesKHR surfaceCapabilities{};         ///< Window surface capabilities for swap chain implementation
    VkSurfaceFormatKHR chosenSurfaceFormat{};               ///< Window surface chosen format and color space
    VkPresentModeKHR chosenSurfacePresentationMode{};       /**< Window surface images refresh and representation mode
                                                             *    on surface */

    // Device available parameters
    std::vector<VkSurfaceFormatKHR> surfaceAvailableFormats;    ///< List of available color formats for swap chain
    std::vector<VkPresentModeKHR> surfacePresentationModes;     ///< List of available presentation modes for the surface

    static bool checkDeviceExtensionSupport(const VkPhysicalDevice& device);

    /**
     * @brief VIEngine::checkQueueFamilyCompatibilityWithDevice for device validation in terms of operation queue families
     * @param device VkPhysicalDevice to be checked
     * @param surface Engine surface for checking device compatibility
     * @param flags vector list to be checked if available for the given device
     * @return true if the device is valid, false otherwise
     */
    static bool checkQueueFamilyCompatibilityWithDevice(const VkPhysicalDevice& device, VkSurfaceKHR& surface,
                                                        std::optional<unsigned int>& queueFamilyIndex);

    /**
     * @brief VIEngine::checkSurfaceCapabilitiesFromDevice
     * @param device
     * @param surface
     * @param surfaceAvailableFormats
     * @param surfacePresentationModes
     */
    static bool checkSurfaceCapabilitiesFromDevice(const VkPhysicalDevice& device, VkSurfaceKHR& surface,
                                                   VkSurfaceCapabilitiesKHR& surfaceCapabilities,
                                                   std::vector<VkSurfaceFormatKHR>& surfaceAvailableFormats,
                                                   std::vector<VkPresentModeKHR>& surfacePresentationModes);

    /**
     * @brief VIEngine::initialiseGLFW for GLFW window manager initialisation
     * A GLFW window is initialised by glfwInit and main properties are assigned to it. The program status is changed
     *  accordingly.
     */
    void initialiseGLFW();

    /**
     * @brief VIEngine::initialiseVulkanLibraries for Vulkan structures initialisation
     * All main Vulkan objects are initialised (application info, data structure for instance creation instructions,
     *  validation layers (API for text/debug options)).
     */
    void initialiseVulkanLibraries();

    /**
     * @brief VIEngine::preparePhysicalDevices for Vulkan rendering device assignment
     * The main device for drawing is found and checked if it has all needed requirements for used commands in the
     *  application
     */
    void preparePhysicalDevices();

    /**
     * @brief VIEngine::createLogicDevice create the runtime instance of a physical device, for runtime device own state
     * From the physical device, a logic device is created and the graphics queue is obtained
     */
    void createLogicDevice();

    /**
     * @brief VIEngine::prepareWindowSurface provides to GLFW the native window handle for Vulkan window attachment
     * The function chooses the correct operative system and the native handle is produced and provided to Vulkan to
     *  communicate with GLFW
     */
    void prepareWindowSurface();

    /**
     *
     */
    void prepareSwapChainFormatAndPresent();

    /**
     * @brief VIEngine::cleanEngine for cleaning all structures and window pointers
     * All structures created during the engine runtime are destroyed at the end.
     */
    void cleanEngine();
public:
    /**
     * @brief VIEngine::prepareEngine for running up all processes (initialisation, preparation, running, cleaning)
     * This function collects all functions needed for running the engine, from initialisation, to drawing, to cleaning
     */
    void prepareEngine();
};