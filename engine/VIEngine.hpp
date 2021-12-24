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
#include "VIEStatus.hpp"
#include "VIESettings.hpp"
#include "VIEUberShader.hpp"

// TODO write functions for creating additional devices or recreate entirely devices
//  -> Collect all creation functions inside structs, in order to create multiple modules whenever needed by user
// TODO keep all CreateInfo and Info stored into structs? If not useful for later use, store them locally
// TODO complete documentation for each structure

// TODO extract modules into specific VIEModule source file?
struct VIEModuleNativeWindow {
    GLFWwindow* glfwWindow{};              ///< GLFW window pointer
    uint32_t glfwExtensionCount{};         ///< GLFW extensions count for Vulkan ext. initialisation
    const char** glfwExtensions{};         /**< GLFW extensions (GLFW APIs and functions) to be used by
                                            *    Vulkan for interacting with window */
};

struct VIEModuleVulkanLibraries {
    VkInstance vkInstance{};                       ///< Vulkan runtime instance
    VkApplicationInfo applicationInfo{};           ///< Vulkan application data
    VkInstanceCreateInfo engineCreationInfo{};     ///< Vulkan essential data for creation procedure
};

struct VIEModuleMainPhysicalDevice {
    VkPhysicalDevice vkPhysicalDevice{};                              /**< Vulkan physical device object (for used device
                                                                       *    representation) */
    VkPhysicalDeviceFeatures vkPhysicalDeviceFeatures{};              ///< Main device features to set for chosen device
    std::optional<uint32_t> vkPhysicalDeviceSelectedQueueFamily;      ///< Queue family chosen for the main device
    std::optional<uint32_t> vkPhysicalDeviceSelectedPresentFamily;    ///< Present family chosen for the mail device
    std::vector<VkSurfaceFormatKHR> surfaceAvailableFormats;          ///< List of available surface color spaces for the surface
    std::vector<VkPresentModeKHR> surfacePresentationModes;           ///< List of available presentation modes for the surface
};

struct VIEModuleMainLogicalDevice {
    VkDevice vkDevice{};                                /**< Vulkan logical device object (for state, resources
                                                         *    used by instance) */
    VkDeviceCreateInfo vkDeviceCreateInfo{};            /**< Vulkan main device object (for logical device
                                                         *    representation) */
    VkDeviceQueueCreateInfo vkDeviceQueueCreateInfo{};  /**< Vulkan essential data for device queue family creation
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
    VkSwapchainKHR swapChain{};                             ///< Swap chain system for framebuffers queue management
    VkExtent2D chosenSwapExtent{};                          ///< Swap chain images resolution definition
    VkSwapchainCreateInfoKHR swapChainCreationInfo{};       ///< Swap chain creation data

    std::vector<VkImage> swapChainImages{};                 ///< Swap chain extracted images
    std::vector<VkImageView> swapChainImageViews{};         ///< Swap chain extracted image viewers
};

// TODO Add functions for inserting vertex shaders (use std::optional for understanding access)
// TODO create two of those pipelines for shadow-mapping, offline rendering and viewport visualization
// TODO create shader pipeline list with drawing functions in order to customise experience (use custom constructors)
// TODO separate all submodules into structs with their createinfo structs and with their creation functions for multiple calls
struct VIEModuleShaderPipeline {
    /* Rendering phases:
     * - Phase 0: Vertex input      (mandatory step for defining input data structure at the beginning of the shader
     *                               rendering process)
     * - Phase 1: Input assembly    (mandatory step for data type given to a shader)
     * - Phase 2: Vertex shader     (mandatory step for vertex processing (from local space to NDC space, through world
     *                               space, view space and clip space)
     * - Phase 3: Tessellation      (optional step for geometry refinement and mesh quality increase)
     * - Phase 4: Geometry shader   (optional step or geometry handling basing on geometry type)
     * - Phase 5: Rasterization     (mandatory step for geometry discretization from tridimensional (space) to
     *                               bidimensional (viewport) space)
     * - Phase 6: Fragment shader   (mandatory step for pixel processing and information selection (like depth test))
     * - Phase 7: Color blending    (mandatory step for color combination between previous and current framebuffer data)
     * - Phase 8: Framebuffer display into viewport
     */

    std::unique_ptr<VIEUberShader> uberShader;

    // TODO split vertex shader modules and bring vertex creation functions into structs
    VkShaderModule vertexModule{};                            ///< Vulkan shader module for SPIR-V compiled code (vert)
    VkShaderModule fragmentModule{};                          ///< Vulkan shader module for SPIR-V compiled code (frag)

    ///< Shader creation info for stage/pipeline definition (vertex) (phase 2)
    VkPipelineShaderStageCreateInfo vertexShaderStageCreationInfo{};
    ///< Shader creation info for stage/pipeline definition (fragment) (phase 6)
    VkPipelineShaderStageCreateInfo fragmentShaderStageCreationInfo{};

    ///< Shader creation info for rendering phase 0: vertex data handling
    VkPipelineVertexInputStateCreateInfo vertexShaderInputStageCreationInfo{};

    ///< Shader creation info for rendering phase 1: input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreationInfo{};

    ///< Shader creation info for rendering phase 5: rasterization
    VkPipelineRasterizationStateCreateInfo rasterizationCreationInfo{};

    ///< Shader creation info for rendering phase 7: color blending
    VkPipelineColorBlendStateCreateInfo colorBlendingAttachmentCreationInfo{};

    ///< Multisampling
    VkPipelineMultisampleStateCreateInfo multisamplingCreationInfo{};

    ///< Shader viewport creation info
    VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};

    // TODO std::optional viewport
    ///< Viewport definition for rendering output
    VkViewport viewport{};  // TODO set as an array for

    ///< Viewport scissor for entire or cut viewport display
    VkRect2D scissorRectangle{};
};

/**
 * @brief VIEngine class representing Vulkan engine
 */
class VIEngine {
    VIESettings settings;
    VIEStatus engineStatus {VIEStatus::UNINITIALISED};

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
    VIEModuleSwapChain mSwapChain;                          ///< Module for rendering framebuffer behaviour

    // Vulkan rendering pipeline
    VIEModuleShaderPipeline mShaderPipeline;                ///< Module for rendering pipeline definition

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
    bool checkDeviceExtensionSupport(const VkPhysicalDevice& device);

    /**
     * @brief VIEngine::checkQueueFamilyCompatibilityWithDevice checks if a device is compatible with specified queue
     *          families
     * @param device VkPhysicalDevice to be checked
     * @param surface Engine surface for checking device compatibility
     * @param flags vector list to be checked if available for the given device
     * @return true if the device is valid, false otherwise
     */
    bool checkQueueFamilyCompatibilityWithDevice(const VkPhysicalDevice& device, VkSurfaceKHR& surface,
                                                 std::optional<uint32_t>& queueFamilyIndex,
                                                 std::optional<uint32_t>& presentQueueFamilyIndex);

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
     * @brief VIEngine::prepareSwapChain prepares the framebuffer queue system, defining the correct and supported color
     *  space and frame buffering mode
     */
    void prepareSwapChain();

    /** TODO complete documentation
     * @brief
     */
    void prepareImageViews();

    /** TODO complete documentation
     * @brief
     */
    void prepareGraphicsPipeline();

    /**
     * @brief VIEngine::cleanEngine for cleaning all structures and window pointers
     * All structures created during the engine runtime are destroyed at the end.
     */
    void cleanEngine();
public:
    VIEngine() = delete;
    explicit VIEngine(const VIESettings& settings);
    VIEngine(const VIEngine&) = delete;
    VIEngine(VIEngine&&) = default;
    ~VIEngine();

    /**
     * @brief VIEngine::prepareEngine for running up all processes (initialisation, preparation, running, cleaning)
     * This function collects all functions needed for running the engine, from initialisation, to drawing, to cleaning
     */
    void prepareEngine();

    /** TODO complete documentation
     * @brief
     */
    void runEngine();
};