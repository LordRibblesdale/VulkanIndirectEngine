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

struct VIEModuleNativeWindow {
    GLFWwindow* mainWindow{};                               ///< GLFW window pointer
    unsigned int glfwExtensionCount{};                      ///< GLFW extensions count for Vulkan ext. initialisation
    const char** glfwExtensions{};                          /**< GLFW extensions (GLFW APIs and functions) to be used by
                                                             *    Vulkan for interacting with window */
};

struct VIEModuleVulkanLibraries {
    VkInstance mainInstance{};                              ///< Vulkan runtime instance
    VkApplicationInfo applicationInfo{};                    ///< Vulkan application data
    VkInstanceCreateInfo engineCreationInfo{};              ///< Vulkan essential data for creation procedure
};

struct VIEModuleMainPhysicalDevice {
    VkPhysicalDevice mainPhysicalDevice{};                          /**< Vulkan physical device object (for used device
                                                                     *    representation) */
    VkPhysicalDeviceFeatures mainPhysicalDeviceFeatures{};          ///< Main device features to set for chosen device
    std::optional<unsigned int> mainDeviceSelectedQueueFamily;      ///< Queue family chosen for the main device
    std::optional<unsigned int> mainDeviceSelectedPresentFamily;    ///< Present family chosen for the mail device
    std::vector<VkSurfaceFormatKHR> surfaceAvailableFormats;        ///< List of available surface color spaces for the surface
    std::vector<VkPresentModeKHR> surfacePresentationModes;         ///< List of available presentation modes for the surface
};

struct VIEModuleMainLogicalDevice {
    VkDevice mainDevice{};                                  /**< Vulkan logical device object (for state, resources
                                                             *    used by instance) */
    VkDeviceCreateInfo mainDeviceCreationInfo{};            /**< Vulkan main device object (for logical device
                                                             *    representation) */
    VkDeviceQueueCreateInfo mainDeviceQueueCreationInfo{};  /**< Vulkan essential data for device queue family creation
                                                             *    procedure */
};

struct VIEModuleSurface {
    VkSurfaceKHR surface{};                                 ///< Window surface for GLFW

    VkSurfaceFormatKHR chosenSurfaceFormat{};               ///< Window surface chosen format and color space
    VkPresentModeKHR chosenSurfacePresentationMode{};       /**< Window surface images refresh and representation mode
                                                             *    on surface */
    VkSurfaceCapabilitiesKHR surfaceCapabilities{};         ///< Window surface capabilities for swap chain implementation
};

struct VIEModuleSwapChain {
    VkExtent2D chosenSwapExtent{};                          ///< TODO complete here
    VkSwapchainCreateInfoKHR swapChainCreationInfo{};       ///< TODO complete here
    VkSwapchainKHR mainSwapChain{};                         ///< TODO complete here
};

/**
 * @brief VIEngine class representing Vulkan engine
 */
class VIEngine {
    // TODO check which of these elements could be freed from memory after prepareEngine
    // GLFW
    VIEModuleNativeWindow mNativeWindow;                     ///< Module for native window

    // Vulkan instance
    VIEModuleVulkanLibraries mVulkanLibraries;              ///< Module for Vulkan libraries access

    // Vulkan physical device
    VIEModuleMainPhysicalDevice mPhysicalDevice;            ///< Module for main physical device for rendering

    // Vulkan logic device
    VIEModuleMainLogicalDevice mLogicDevice;                ///< Module for main logic device

    // Vulkan window surface
    VIEModuleSurface mSurface;                              ///< Module for rendering window surface

    // Vulkan swap chain
    VIEModuleSwapChain mSwapChain;                          ///< Module for rendering

    // Vulkan queue family
    float mainQueueFamilyPriority = 1.0f;                   ///< Main queue family priority

    // Vulkan graphics queue
    VkQueue graphicsQueue{};                                ///< Main rendering queue
    VkQueue presentQueue{};                                 ///< Main frame representation queue

    /**
     * @brief VIEngine::checkDeviceExtensionSupport checks if a device supports the defined list of Vulkan extensions
     * @param device VkPhysicalDevice to be checked
     * @return true if the device is compatible, false otherwise
     */
    static bool checkDeviceExtensionSupport(const VkPhysicalDevice& device);

    /**
     * @brief VIEngine::checkQueueFamilyCompatibilityWithDevice checks if a device is compatible with specified queue
     *          families
     * @param device VkPhysicalDevice to be checked
     * @param surface Engine surface for checking device compatibility
     * @param flags vector list to be checked if available for the given device
     * @return true if the device is valid, false otherwise
     */
    static bool checkQueueFamilyCompatibilityWithDevice(const VkPhysicalDevice& device, VkSurfaceKHR& surface,
                                                        std::optional<unsigned int>& queueFamilyIndex,
                                                        std::optional<unsigned int>& presentQueueFamilyIndex);

    /**
     * @brief VIEngine::checkSurfaceCapabilitiesFromDevice checks if a surface, related to its device, supports a set of
     *          defined color spaces and frame presentation modes
     * @param device VKPhysicalDevice to be checked
     * @param surface VkSurfaceKHR to be checked
     * @param surfaceAvailableFormats list of required surface formats
     * @param surfacePresentationModes list of required presentation modes
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
     * @brief VIEngine::prepareMainPhysicalDevices for Vulkan rendering device assignment
     * The main device for drawing is found and checked if it has all needed requirements for used commands in the
     *  application
     */
    void prepareMainPhysicalDevices();

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
     * TODO
     */
    void prepareSwapChain();

    /**
     * TODO
     */
    void prepareImageViews();

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