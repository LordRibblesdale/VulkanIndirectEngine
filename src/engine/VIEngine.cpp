/* Created by LordRibblesdale on 16/10/2021.
 * MIT License
 */

#include <ranges>
#include "engine/VIEngine.hpp"
#include "engine/VIESettings.hpp"
#include "tools/VIETools.hpp"

VIEngine::VIEngine(const VIESettings &settings) : settings(settings) {}

VIEngine::~VIEngine() {
    if (engineStatus != VIEStatus::UNINITIALISED) {
        cleanEngine();
    }
}

void VIEngine::framebufferResizeCallback(GLFWwindow *window, int width, int height) {
    auto engine = static_cast<VIEngine*>(glfwGetWindowUserPointer(window));
    engine->isFramebufferResized = true;
}

bool VIEngine::generateRendererCore() {
    /// -- Swap chain --
    // Selecting surface format (channels and color space)
    return_log_if(!tools::selectSurfaceFormat(surfaceAvailableFormats, settings.defaultFormat,
                                              settings.defaultColorSpace, chosenSurfaceFormat),
                  "No compatible surface format found for main physical device...", false)

    // Selecting presentation mode by a swap chain (describing how and when images are represented on screen)
    chosenSurfacePresentationMode = tools::selectSurfacePresentation(surfacePresentationModes,
                                                                     settings.preferredPresentMode);

    // Selecting swap extent (resolution of the swap chain images in pixels)
    /*
    if (surfaceCapabilities.currentExtent.width != kUint32Max) {
        chosenSwapExtent = surfaceCapabilities.currentExtent;
        std::cout << fmt::format("W: {}, H: {}", chosenSwapExtent.width, chosenSwapExtent.height) << std::endl;
    } else {
        int width;
        int height;
        glfwGetFramebufferSize(glfwWindow, &width, &height);
        std::cout << fmt::format("W: {}, H: {}", width, height) << std::endl;

        chosenSwapExtent = VkExtent2D{
                .width = std::clamp(static_cast<uint32_t>(width), surfaceCapabilities.minImageExtent.width,
                                    surfaceCapabilities.maxImageExtent.width),
                .height = std::clamp(static_cast<uint32_t>(height), surfaceCapabilities.minImageExtent.height,
                                     surfaceCapabilities.maxImageExtent.height)
        };
    }
     */
    int width;
    int height;
    glfwGetFramebufferSize(glfwWindow, &width, &height);

    chosenSwapExtent = VkExtent2D{
            .width = std::clamp(static_cast<uint32_t>(width), surfaceCapabilities.minImageExtent.width,
                                surfaceCapabilities.maxImageExtent.width),
            .height = std::clamp(static_cast<uint32_t>(height), surfaceCapabilities.minImageExtent.height,
                                 surfaceCapabilities.maxImageExtent.height)
    };

    // Setting the number of images that the swap chain needs to create, depending on a necessary minimum and maximum
    uint32_t swapChainImagesCount = std::min(surfaceCapabilities.minImageCount + 1, surfaceCapabilities.maxImageCount);

    // Choosing frame handling mode by swap chain
    std::array<uint32_t, 2> queueIndices({selectedQueueFamily, selectedPresentFamily});

    // VIESettings how swap chain will be created
    VkSwapchainCreateInfoKHR swapChainCreationInfo{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = surface,
            .minImageCount = swapChainImagesCount,
            .imageFormat = chosenSurfaceFormat.format,
            .imageColorSpace = chosenSurfaceFormat.colorSpace,
            .imageExtent = chosenSwapExtent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = (selectedQueueFamily != selectedPresentFamily) ? VK_SHARING_MODE_CONCURRENT
                                                                               : VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = (selectedQueueFamily != selectedPresentFamily) ? 2u : 0u,
            .pQueueFamilyIndices = (selectedQueueFamily != selectedPresentFamily) ? queueIndices.data() : nullptr,
            .preTransform = surfaceCapabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = chosenSurfacePresentationMode,
            .clipped = VK_TRUE,
            .oldSwapchain = VK_NULL_HANDLE
    };

    return_log_if(vkCreateSwapchainKHR(vkDevice, &swapChainCreationInfo, nullptr, &swapChain),
                  "Cannot create swap chain for main device!", false)

    tools::gatherVkData(vkGetSwapchainImagesKHR, swapChainImages, swapChainImagesCount, vkDevice, swapChain);

    engineStatus = VIEStatus::VULKAN_SWAP_CHAIN_CREATED;

    /// -- Image views --
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; VkImageView &imageView: swapChainImageViews) {
        VkImageViewCreateInfo imageViewCreationInfo{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = swapChainImages.at(i),
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = chosenSurfaceFormat.format,
                .components = VkComponentMapping{VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                                                 VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
                .subresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
        };

        return_log_if(vkCreateImageView(vkDevice, &imageViewCreationInfo, nullptr, &imageView) != VK_SUCCESS,
                      fmt::format("Cannot generate image view {}.", i), false)

        ++i;
    }

    engineStatus = VIEStatus::VULKAN_IMAGE_VIEWS_CREATED;

    /// -- Pipeline functions --
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 0,
            .pSetLayouts = nullptr,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr
    };

    return_log_if(
            vkCreatePipelineLayout(vkDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS,
            "Failed to create pipeline layout...", false)

    engineStatus = VIEStatus::VULKAN_PIPELINE_STATES_PREPARED;

    /// -- Render passes --
    VkAttachmentDescription colorAttachment{
            .format = chosenSurfaceFormat.format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference colorAttachmentReference{
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpassDescription{
            .flags = 0,
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .inputAttachmentCount = 0,
            .pInputAttachments = nullptr,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentReference,
            .pResolveAttachments = nullptr,
            .pDepthStencilAttachment = nullptr,
            .preserveAttachmentCount = 0,
            .pPreserveAttachments = nullptr
    };

    VkSubpassDependency dependency{
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = 0
    };

    VkRenderPassCreateInfo renderPassCreateInfo{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &colorAttachment,
            .subpassCount = 1,
            .pSubpasses = &subpassDescription,
            .dependencyCount = 1,
            .pDependencies = &dependency
    };

    return_log_if(vkCreateRenderPass(vkDevice, &renderPassCreateInfo, nullptr, &renderPass) != VK_SUCCESS,
                  "Failed to create render pass...", false)

    engineStatus = VIEStatus::VULKAN_RENDER_PASSES_GENERATED;

    /// -- Graphics pipeline --
    // Shader creation info for stage/pipeline definition (vertex) (phase 2)
    // TODO move into shader and define a config file in order to tell "pName" if necessary
    VkPipelineShaderStageCreateInfo vertexShaderStageCreationInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertexModule,
            .pName = "main",
            .pSpecializationInfo = nullptr
    };

    // Shader creation info for stage/pipeline definition (fragment) (phase 6)
    VkPipelineShaderStageCreateInfo fragmentShaderStageCreationInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragmentModule,
            .pName = "main",
            .pSpecializationInfo = nullptr
    };

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{
            vertexShaderStageCreationInfo,
            fragmentShaderStageCreationInfo
    };

    // Shader creation info for rendering phase 0: vertex data handling
    VkPipelineVertexInputStateCreateInfo vertexShaderInputStageCreationInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 0,
            .pVertexBindingDescriptions = nullptr,
            .vertexAttributeDescriptionCount = 0,
            .pVertexAttributeDescriptions = nullptr
    };

    // Shader creation info for rendering phase 1: input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreationInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE
    };

    // Setting viewport size wrt framebuffers/swap chain
    VkViewport viewport{
            .x = 0,
            .y = 0,
            .width = static_cast<float>(chosenSwapExtent.width),
            .height = static_cast<float>(chosenSwapExtent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f
    };

    VkRect2D scissorRectangle{.offset = {0, 0}, .extent = chosenSwapExtent};

    // Shader viewport creation info
    VkPipelineViewportStateCreateInfo viewportStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports = &viewport,
            .scissorCount = 1,
            .pScissors = &scissorRectangle
    };

    // TODO enable for shadow mapping, requires a GPU feature to check in function-like "enableShadowMapping" (maybe presets for each module and submodule)
    // Shader creation info for rendering phase 5: rasterization
    // TODO CW or CCW?
    VkPipelineRasterizationStateCreateInfo rasterizationCreationInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_BACK_BIT,      // FRONT for Shadow Mapping
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .depthBiasConstantFactor = 0.0f,
            .depthBiasClamp = 0.0f,
            .depthBiasSlopeFactor = 0.0f,
            .lineWidth = 1.0f
    };

    /*
    // Multisampling
    // TODO add MULTISAMPLING feature to device features (or check if available)
    VkPipelineMultisampleStateCreateInfo multisamplingCreationInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 1.0f,
            .pSampleMask = nullptr,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE
    };

    // TODO implement DEPTH STENCIL TEST
    // VkPipelineDepthStencilStateCreateInfo
     */

    // https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions
    VkPipelineColorBlendAttachmentState colorBlendAttachmentState{
            .blendEnable = VK_FALSE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                              VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments = &colorBlendAttachmentState,
            .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}
    };

    /*
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
            .pDynamicStates = dynamicStates.data()
    };
     */

    // TODO https://vulkan-tutorial.com/en/Drawing_a_triangle/Graphics_pipeline_basics/Conclusion
    VkGraphicsPipelineCreateInfo pipelineCreateInfo{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = shaderStages.size(),
            .pStages = shaderStages.data(),
            .pVertexInputState = &vertexShaderInputStageCreationInfo,
            .pInputAssemblyState = &inputAssemblyCreationInfo,
            .pTessellationState = nullptr,
            .pViewportState = &viewportStateCreateInfo,
            .pRasterizationState = &rasterizationCreationInfo,
            .pMultisampleState = nullptr,
            // .pMultisampleState = &multisamplingCreationInfo,
            .pDepthStencilState = nullptr,
            .pColorBlendState = &colorBlendStateCreateInfo,
            .pDynamicState = nullptr,   // TODO check why dynamicStateCreateInfo doesn't work (black screen)
            //.pDynamicState = &dynamicStateCreateInfo,
            .layout = pipelineLayout,
            .renderPass = renderPass,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = -1
    };

    return_log_if(vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,
                                            &graphicsPipeline) != VK_SUCCESS,
                  "Failed to create graphics pipeline...", false)

    engineStatus = VIEStatus::VULKAN_GRAPHICS_PIPELINE_GENERATED;

    /// -- Framebuffers --
    // Framebuffers linked to swap chains and image views
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; const VkImageView &attachment: swapChainImageViews) {
        VkFramebufferCreateInfo framebufferCreateInfo{
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = renderPass,
                .attachmentCount = 1,
                .pAttachments = &attachment,
                .width = chosenSwapExtent.width,
                .height = chosenSwapExtent.height,
                .layers = 1
        };

        return_log_if(vkCreateFramebuffer(vkDevice, &framebufferCreateInfo, nullptr, &swapChainFramebuffers.at(i)) !=
                      VK_SUCCESS, fmt::format("Cannot create framebuffer {}", i), false)

        ++i;
    }

    engineStatus = VIEStatus::VULKAN_FRAMEBUFFERS_CREATED;

    /// -- Command buffers --
    commandBuffers.resize(swapChainFramebuffers.size());

    VkCommandBufferAllocateInfo commandBufferAllocateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = static_cast<uint32_t>(commandBuffers.size())
    };

    return_log_if(
            vkAllocateCommandBuffers(vkDevice, &commandBufferAllocateInfo, commandBuffers.data()) != VK_SUCCESS,
            "Cannot create command buffers...", false)

    engineStatus = VIEStatus::VULKAN_COMMAND_BUFFERS_PREPARED;

    // TODO integrate custom render pass and draw commands so that others could implement their shaders and related commands
    //  maybe to split in separate function in order to implement one or more lambdas
    for (size_t i = 0; const VkCommandBuffer &buffer: commandBuffers) {
        VkCommandBufferBeginInfo commandBufferBeginInfo{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pInheritanceInfo = nullptr
        };

        return_log_if(vkBeginCommandBuffer(buffer, &commandBufferBeginInfo) != VK_SUCCESS,
                      fmt::format("Cannot begin recording command buffer {}", i), false)

        VkClearValue clearColor{{{0.0f, 0.0f, 0.0f, 1.0f}}};
        VkRenderPassBeginInfo renderPassBeginInfo{
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass = renderPass,
                .framebuffer = swapChainFramebuffers.at(i),
                .renderArea = VkRect2D{{0, 0}, chosenSwapExtent},
                .clearValueCount = 1,
                .pClearValues = &clearColor
        };

        vkCmdBeginRenderPass(buffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        vkCmdDraw(buffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(buffer);

        return_log_if(vkEndCommandBuffer(buffer) != VK_SUCCESS, "Failed to record command buffer...", false)

        ++i;
    }

    engineStatus = VIEStatus::VULKAN_COMMAND_POOL_CREATED;

    return true;
}

bool VIEngine::regenerateRendererCore() {
    // TODO check if program should pause or not during minimizing (by flag?)
    /*
    int width = 0, height = 0;
    glfwGetFramebufferSize(glfwWindow, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(glfwWindow, &width, &height);
        glfwWaitEvents();
    }
     */
    if (engineStatus < VIEStatus::VULKAN_ENGINE_RUNNING) {
        return true;
    }

    vkDeviceWaitIdle(vkDevice);

    cleanSwapchain();

    return_log_if(!generateRendererCore(), "(Re)Error generating renderer core", false)

    return true;
}

bool VIEngine::prepareEngine() {
    std::vector<const char*> vGlfwExtensions;    ///< GLFW extensions count for Vulkan ext. initialisation

    VkPhysicalDevice vkPhysicalDevice{};                    /**< Vulkan physical device object (for used device
                                                             *    representation) */
    VkPhysicalDeviceFeatures vkPhysicalDeviceFeatures{};    ///< Main device features to set for chosen device

    float mainQueueFamilyPriority = 1.0f;                   ///< Main queue family priority

    // GLFW initialization lambda
    // https://www.glfw.org/docs/3.3/group__init.html
    auto initializeGlfw([this, &vGlfwExtensions]() {
        // Initializing GLFW library
        /* Calling glfwInit() -> GLFW_TRUE(1) or GLFW_FALSE(0) */
        return_log_if(!glfwInit(), "GLFW not initialised...", false)

        // Hinting GLFW to not load APIs (Vulkan APIs not defined in GLFW)
        /* Calling glfwWindowHint(windowHint, valueToSet) */
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        // Creating pointer to GLFWwindow struct
        /* Calling glfwCreateWindow(width, height, title, monitor, shareDependencyWindow) -> GLFWwindow* */
        glfwWindow = glfwCreateWindow(static_cast<int>(settings.kDefaultXRes),
                                      static_cast<int>(settings.kDefaultYRes),
                                      settings.kApplicationProgramName.c_str(), nullptr, nullptr);

        return_log_if(!glfwWindow, "GLFW window not initialised...", false)

        // Getting extensions from GLFW for Vulkan implementation
        /* Calling glfwGetRequiredInstanceExtensions(pointer to uint32_t count) -> const char** */
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        vGlfwExtensions.reserve(glfwExtensionCount);
        vGlfwExtensions.insert(vGlfwExtensions.begin(), glfwExtensions, glfwExtensions + glfwExtensionCount);

        // Setting up current engine pointer for callbacks
        glfwSetWindowUserPointer(glfwWindow, this);

        // Setting up window resize callback
        glfwSetFramebufferSizeCallback(glfwWindow, framebufferResizeCallback);

        return true;
    });

    auto createVulkanInstance([this, &vGlfwExtensions]() {
        // Creating the application details
        // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkApplicationInfo.html
        //TODO update to 1.3
        VkApplicationInfo applicationInfo{
                .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                .pApplicationName = settings.kApplicationName.c_str(),
                .applicationVersion = settings.kApplicationVersion,
                .pEngineName = "VulkanIndirectEngine",
                .engineVersion = settings.kEngineVersion,
                .apiVersion = VK_API_VERSION_1_2
        };

        // Checking validation layers
        uint32_t layerCount;
        std::vector<VkLayerProperties> availableLayers;
        tools::gatherVkData(vkEnumerateInstanceLayerProperties, availableLayers, layerCount);

        // Removing the one that are not compatible
        std::erase_if(settings.validationLayers, [&availableLayers](std::string_view layer) {
            return std::ranges::none_of(availableLayers, [&layer](const VkLayerProperties &availableLayer) {
                return layer == availableLayer.layerName;
            });
        });

        if (settings.enableMessageCallback) {
            vGlfwExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = debug::debugCallback
        };

        // Creating Vulkan instance
        VkInstanceCreateInfo engineCreationInfo{
                .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                .pNext = settings.enableMessageCallback ? &debugCreateInfo : nullptr,
                .pApplicationInfo = &applicationInfo,
                .enabledLayerCount = static_cast<uint32_t>(settings.validationLayers.size()),
                .ppEnabledLayerNames = settings.validationLayers.data(),
                .enabledExtensionCount = static_cast<uint32_t>(vGlfwExtensions.size()),
                .ppEnabledExtensionNames = vGlfwExtensions.data()
        };

        // Calling vkCreateInstance(pointer to createInfo structure, pointer to custom memory allocator
        return_log_if(vkCreateInstance(&engineCreationInfo, nullptr, &vkInstance) != VK_SUCCESS,
                      "Vulkan instance not created...", false)

        return true;
    });

    auto createWindowSurface([this]() {
#if _WIN64
        // Creating Vulkan surface based on WindowsNT native bindings
        VkWin32SurfaceCreateInfoKHR ntWindowSurfaceCreationInfo{
                .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
                .hinstance = GetModuleHandle(nullptr),
                .hwnd = glfwGetWin32Window(glfwWindow)
        };

        return_log_if(vkCreateWin32SurfaceKHR(vkInstance, &ntWindowSurfaceCreationInfo, nullptr, &surface) !=
                      VK_SUCCESS, "Cannot create native NT window surface to bind to Vulkan...", false)
#elif __linux__
        // Creating Vulkan surface based on Wayland native bindings
        return_log_if(glfwCreateWindowSurface(mVulkanLibraries.vkInstance, mNativeWindow.glfwWindow, nullptr,
                                              &mSurface.surface), "Error creating surface in Wayland environment...",
                      false);
#endif

        return true;
    });

    auto preparePhysicalDevice([this, &vkPhysicalDevice]() {
        // Looking for devices
        uint32_t devicesCount = 0;
        std::vector<VkPhysicalDevice> availableDevices;
        tools::gatherVkData(vkEnumeratePhysicalDevices, availableDevices, devicesCount, vkInstance);

        return_log_if(devicesCount == 0, "No physical devices found for Vulkan rendering...", false)

        // Erasing from list devices that are not considered valid ones (if isPreferrableDevice is
        if (settings.isPreferrableDevice) {
            std::erase_if(availableDevices, [this](const VkPhysicalDevice &physicalDevice) {
                return !settings.isPreferrableDevice(physicalDevice);
            });
        }

        // Selecting and sorting devices
        // For one device, the selection is at the beginning of availableDevices vector
        bool isDeviceSet = (std::ranges::any_of(
                availableDevices,
                [this, &vkPhysicalDevice](const VkPhysicalDevice &physicalDevice) {
                    return tools::selectPhysicalDevice(physicalDevice, vkPhysicalDevice, selectedQueueFamily,
                                                       selectedPresentFamily, surface, surfaceCapabilities,
                                                       surfaceAvailableFormats, surfacePresentationModes,
                                                       settings);
                }));

        return_log_if(!isDeviceSet, "Error looking for physical device...", false)

        return true;
    });

    auto prepareLogicalDevice([this, &vkPhysicalDevice, &vkPhysicalDeviceFeatures, &mainQueueFamilyPriority]() {
        // Preparing command queue family for the main device
        VkDeviceQueueCreateInfo vkDeviceQueueCreateInfo{
                .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                .queueFamilyIndex = selectedQueueFamily,
                .queueCount = 1,
                .pQueuePriorities = &mainQueueFamilyPriority
        };

        // Defining logical device creation, basing on queue priority, validation layers and physical device features
        VkDeviceCreateInfo vkDeviceCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .queueCreateInfoCount = 1,
                .pQueueCreateInfos = &vkDeviceQueueCreateInfo,
                .enabledLayerCount = static_cast<uint32_t>(settings.validationLayers.size()),
                .ppEnabledLayerNames = settings.validationLayers.data(),
                .enabledExtensionCount = static_cast<uint32_t>(settings.deviceExtensions.size()),
                .ppEnabledExtensionNames = settings.deviceExtensions.data(),
                .pEnabledFeatures = &vkPhysicalDeviceFeatures,
        };

        return_log_if(vkCreateDevice(vkPhysicalDevice, &vkDeviceCreateInfo, nullptr, &vkDevice) != VK_SUCCESS,
                      "Vulkan logic device not created...", false)

        // Obtaining graphics queue family from logic device via stored index
        vkGetDeviceQueue(vkDevice, selectedQueueFamily, 0, &graphicsQueue);
        vkGetDeviceQueue(vkDevice, selectedPresentFamily, 0, &presentQueue);

        return true;
    });

    auto generateShaderModules([this]() {
        // TODO make generic for every pipeline and every input shader and both code and binary
        if (!uberShader) {
            uberShader = std::make_unique<VIEUberShader>("./shaders/debug/shader.vert", "./shaders/debug/shader.frag");
        }

        vertexModule = uberShader->createVertexModuleFromSPIRV(vkDevice);
        return_log_if(vertexModule == nullptr, "Cannot create vertex module...", false)

        fragmentModule = uberShader->createFragmentModuleFromSPIRV(vkDevice);
        return_log_if(fragmentModule == nullptr, "Cannot create fragment module...", false)

        return true;
    });

    auto createCommandPool([this]() {
        VkCommandPoolCreateInfo commandPoolCreateInfo{
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .queueFamilyIndex = selectedQueueFamily
        };

        return_log_if(vkCreateCommandPool(vkDevice, &commandPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS,
                      "Cannot create command pool...", false)

        return true;
    });

    auto createSemaphores([this]() {
        imageAvailableSemaphores.resize(settings.kMaxFramesInFlight);
        renderFinishedSemaphores.resize(settings.kMaxFramesInFlight);
        inFlightFences.resize(settings.kMaxFramesInFlight);
        imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreCreateInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

        VkFenceCreateInfo fenceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };

        for (uint8_t i = 0; i < settings.kMaxFramesInFlight; ++i) {
            return_log_if(vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphores[i]) !=
                          VK_SUCCESS, fmt::format("Cannot create image semaphore {}...", i), false)

            return_log_if(vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores[i]) !=
                          VK_SUCCESS, fmt::format("Cannot create render semaphore {}...", i), false)

            return_log_if(vkCreateFence(vkDevice, &fenceCreateInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS,
                          fmt::format("Cannot create fence {}...", i), false)
        }

        return true;
    });

    return_log_if(!initializeGlfw(), "Error initializeGlfw()", false)
    engineStatus = VIEStatus::GLFW_LOADED;

    return_log_if(!createVulkanInstance(), "Error createVulkanInstance()", false)
    engineStatus = VIEStatus::VULKAN_INSTANCE_CREATED;

    return_log_if(!createWindowSurface(), "Error createWindowSurface()", false)
    engineStatus = VIEStatus::VULKAN_SURFACE_CREATED;

    return_log_if(!preparePhysicalDevice(), "Error preparePhysicalDevice()", false)
    engineStatus = VIEStatus::VULKAN_PHYSICAL_DEVICES_PREPARED;

    return_log_if(!prepareLogicalDevice(), "Error prepareLogicalDevice()", false)
    engineStatus = VIEStatus::VULKAN_LOGICAL_DEVICE_CREATED;

    return_log_if(!generateShaderModules(), "Error generateShaderModules()", false)
    engineStatus = VIEStatus::VULKAN_SHADERS_COMPILED;

    return_log_if(!createCommandPool(), "Error createCommandPool()", false)
    engineStatus = VIEStatus::VULKAN_COMMAND_POOL_CREATED;

    return_log_if(!generateRendererCore(), "Error generateRendererCore()", false)
    engineStatus = VIEStatus::VULKAN_RENDERER_CORE_INIT;

    return_log_if(!createSemaphores(), "Error createSemaphores()", false)
    engineStatus = VIEStatus::VULKAN_SEMAPHORES_CREATED;

    return true;
}

void VIEngine::runEngine() {
    if (engineStatus < VIEStatus::VULKAN_SEMAPHORES_CREATED) {
        return;
    }

    engineStatus = VIEStatus::VULKAN_ENGINE_RUNNING;

    // TODO create function for defining key and mouse inputs
    while (!glfwWindowShouldClose(glfwWindow)) {
        glfwPollEvents();

        if (!drawFrame()) {
            std::cout << "Error drawing frame..." << std::endl;
        }
    }

    vkDeviceWaitIdle(vkDevice);
}

// TODO make generic also for VR!!
bool VIEngine::drawFrame() {
    uint32_t imageIndex = 0;

    vkWaitForFences(vkDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    // TODO For framerate limiter https://vkguide.dev/docs/chapter-1/vulkan_mainloop_code/
    if (VkResult result{vkAcquireNextImageKHR(vkDevice, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame],
                                              VK_NULL_HANDLE, &imageIndex)};
            result == VK_ERROR_OUT_OF_DATE_KHR) {
        regenerateRendererCore();
        return true;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        std::cout << "Error acquiring next VkImage..." << std::endl;
        return false;
    }

    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(vkDevice, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    // TODO move as constant
    std::array<VkPipelineStageFlags, 1> waitStages{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &imageAvailableSemaphores[currentFrame],
            .pWaitDstStageMask = waitStages.data(),
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffers.at(imageIndex),
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &renderFinishedSemaphores[currentFrame],
    };

    vkResetFences(vkDevice, 1, &inFlightFences[currentFrame]);

    if (VkResult result{vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame])};
            result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || isFramebufferResized) {
        isFramebufferResized = false;
        regenerateRendererCore();
    } else if (result != VK_SUCCESS) {
        std::cout << "Cannot submit draw command buffer..." << std::endl;
        return false;
    }

    std::array<VkSwapchainKHR, 1> swapChainsKHR{swapChain};
    VkPresentInfoKHR presentInfo{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &renderFinishedSemaphores[currentFrame],
            .swapchainCount = swapChainsKHR.size(),
            .pSwapchains = swapChainsKHR.data(),
            .pImageIndices = &imageIndex,
            .pResults = nullptr
    };

    vkQueuePresentKHR(presentQueue, &presentInfo);

    vkQueueWaitIdle(presentQueue);

    ++currentFrame;
    if (currentFrame == settings.kMaxFramesInFlight) {
        currentFrame = 0;
    }

    return true;
}

void VIEngine::cleanSwapchain() {
    for (VkFramebuffer &framebuffer: swapChainFramebuffers) {
        vkDestroyFramebuffer(vkDevice, framebuffer, nullptr);
    }

    vkFreeCommandBuffers(vkDevice, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

    vkDestroyPipeline(vkDevice, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(vkDevice, pipelineLayout, nullptr);
    vkDestroyRenderPass(vkDevice, renderPass, nullptr);

    for (VkImageView& image: swapChainImageViews) {
        vkDestroyImageView(vkDevice, image, nullptr);
    }

    vkDestroySwapchainKHR(vkDevice, swapChain, nullptr);
}

void VIEngine::cleanEngine() {
    if (engineStatus >= VIEStatus::VULKAN_SEMAPHORES_CREATED) {
        for (uint8_t i = 0; i < settings.kMaxFramesInFlight; ++i) {
            vkDestroyFence(vkDevice, inFlightFences[i], nullptr);
            vkDestroySemaphore(vkDevice, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(vkDevice, imageAvailableSemaphores[i], nullptr);
        }
    }

    if (engineStatus >= VIEStatus::VULKAN_COMMAND_POOL_CREATED) {
        vkDestroyCommandPool(vkDevice, commandPool, nullptr);
    }

    if (engineStatus >= VIEStatus::VULKAN_FRAMEBUFFERS_CREATED) {
        for (const VkFramebuffer &framebuffer: swapChainFramebuffers) {
            vkDestroyFramebuffer(vkDevice, framebuffer, nullptr);
        }
    }

    if (engineStatus >= VIEStatus::VULKAN_GRAPHICS_PIPELINE_GENERATED) {
        vkDestroyPipeline(vkDevice, graphicsPipeline, nullptr);
    }

    if (engineStatus >= VIEStatus::VULKAN_PIPELINE_STATES_PREPARED) {
        vkDestroyPipelineLayout(vkDevice, pipelineLayout, nullptr);
    }

    if (engineStatus >= VIEStatus::VULKAN_SHADERS_COMPILED) {
        // TODO extend when having multiple VIEModules, shader modules
        vkDestroyShaderModule(vkDevice, vertexModule, nullptr);
        vkDestroyShaderModule(vkDevice, fragmentModule, nullptr);
    }

    if (engineStatus >= VIEStatus::VULKAN_IMAGE_VIEWS_CREATED) {
        for (auto &imageView: swapChainImageViews) {
            vkDestroyImageView(vkDevice, imageView, nullptr);
        }
    }

    if (engineStatus >= VIEStatus::VULKAN_SWAP_CHAIN_CREATED) {
        vkDestroySwapchainKHR(vkDevice, swapChain, nullptr);
    }

    if (engineStatus >= VIEStatus::VULKAN_LOGICAL_DEVICE_CREATED) {
        vkDestroyDevice(vkDevice, nullptr);
    }

    if (engineStatus >= VIEStatus::VULKAN_SURFACE_CREATED) {
        vkDestroySurfaceKHR(vkInstance, surface, nullptr);
    }

    if (engineStatus >= VIEStatus::VULKAN_INSTANCE_CREATED) {
        vkDestroyInstance(vkInstance, nullptr);
    }

    if (engineStatus >= VIEStatus::GLFW_LOADED) {
        // Destroying window and terminating GLFW instance
        glfwDestroyWindow(glfwWindow);
        glfwTerminate();
    }

    engineStatus = VIEStatus::UNINITIALISED;
}
