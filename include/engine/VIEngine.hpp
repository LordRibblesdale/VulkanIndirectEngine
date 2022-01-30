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

/**
 * @brief VIEngine class representing Vulkan engine
 */
class VIEngine {
    VIESettings settings;
    VIEStatus engineStatus{VIEStatus::UNINITIALISED};

    // TODO check which of these elements could be freed from memory after prepareEngine
    // GLFW
    GLFWwindow *glfwWindow{};           ///< GLFW window pointer

    // Vulkan instance
    VkInstance vkInstance{};            ///< Vulkan runtime instance

    // Vulkan logic device
    VkDevice vkDevice{};                ///< Vulkan logical device object (for state, resources used by instance)

    VkShaderModule vertexModule{};
    VkShaderModule fragmentModule{};

    // Vulkan window surface
    VkSurfaceKHR surface{};             ///< Window surface for GLFW

    // Vulkan swap chain
    std::optional<uint32_t> selectedQueueFamily;            ///< Queue family chosen for the main device
    std::optional<uint32_t> selectedPresentFamily;          ///< Present family chosen for the mail device
    std::vector<VkSurfaceFormatKHR> surfaceAvailableFormats;    ///< List of available surface color spaces for the surface
    std::vector<VkPresentModeKHR> surfacePresentationModes;     ///< List of available presentation modes for the surface
    VkSurfaceFormatKHR chosenSurfaceFormat{};               ///< Window surface chosen format and color space
    VkPresentModeKHR chosenSurfacePresentationMode{};       /**< Window surface images refresh and representation mode
                                                             *    on surface */
    VkExtent2D chosenSwapExtent{};                          ///< Swap chain images resolution definition
    VkSurfaceCapabilitiesKHR surfaceCapabilities{};         ///< Window surface capabilities for swap chain implementation
    std::vector<VkImage> swapChainImages{};                 ///< Swap chain extracted images
    std::vector<VkImageView> swapChainImageViews{};         ///< Swap chain extracted image viewers
    VkSwapchainKHR swapChain{};          ///< Swap chain system for framebuffers queue management

    // Vulkan rendering pipeline
    std::unique_ptr<VIEUberShader> uberShader;

    ///<
    std::array<VkDynamicState, 2> dynamicStates{
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_LINE_WIDTH
    };
    VkRenderPass renderPass{};
    VkPipelineLayout pipelineLayout{};
    VkPipeline graphicsPipeline{};

    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;

    // Vulkan graphics queue
    VkQueue graphicsQueue{};                                ///< Main rendering queue
    VkQueue presentQueue{};                                 ///< Main frame representation queue


    /**
     * @brief VIEngine::checkQueueFamilyCompatibilityWithDevice checks if a device is compatible with specified queue
     *          families
     * @param device VkPhysicalDevice to be checked
     * @param selectedSurface Engine selectedSurface for checking device compatibility
     * @param flags vector list to be checked if available for the given device
     * @return true if the device is valid, false otherwise
     */
    bool checkQueueFamilyCompatibilityWithDevice(const VkPhysicalDevice &device, VkSurfaceKHR &selectedSurface,
                                                 std::optional<uint32_t> &queueFamilyIndex,
                                                 std::optional<uint32_t> &presentQueueFamilyIndex);

    /**
     * @brief VIEngine::checkSurfaceCapabilitiesFromDevice checks if a surface, related to its device, supports a set of
     *          defined color spaces and frame presentation modes
     * @param device VKPhysicalDevice to be checked
     * @param surface VkSurfaceKHR to be checked
     * @param surfaceAvailableFormats list of required surface formats
     * @param surfacePresentationModes list of required presentation modes
     */
    static bool checkSurfaceCapabilitiesFromDevice(const VkPhysicalDevice &device, VkSurfaceKHR &surface,
                                                   VkSurfaceCapabilitiesKHR &surfaceCapabilities,
                                                   std::vector<VkSurfaceFormatKHR> &surfaceAvailableFormats,
                                                   std::vector<VkPresentModeKHR> &surfacePresentationModes);

    [[nodiscard]] bool drawFrame();

    bool createSwapchains();
    bool createImageViews();
    bool prepareRenderPasses();
    bool generateGraphicsPipeline();
    bool initializeFramebuffers();
    bool prepareCommandBuffers();

    bool recreateSwapchain();

    void cleanSwapchain();

public:
    VIEngine() = delete;
    explicit VIEngine(const VIESettings &settings);
    VIEngine(const VIEngine &) = delete;
    VIEngine(VIEngine &&) = default;
    ~VIEngine();

    /**
     * @brief VIEngine::prepareEngine for running up all processes (initialisation, preparation, running, cleaning)
     * This function collects all functions needed for running the engine for initialization
     */
    bool prepareEngine();

    /** TODO complete documentation
     * @brief
     */
    void runEngine();

    /**
     * @brief VIEngine::cleanEngine for cleaning all structures and window pointers
     * All structures created during the engine runtime are destroyed at the end.
     */
    void cleanEngine();
};