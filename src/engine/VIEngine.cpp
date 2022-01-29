/* Created by LordRibblesdale on 16/10/2021.
 * MIT License
 */

#include "engine/VIEngine.hpp"
#include "engine/VIESettings.hpp"

VIEngine::VIEngine(const VIESettings &settings) : settings(settings) {}

VIEngine::~VIEngine() {
    if (engineStatus != VIEStatus::UNINITIALISED) {
        cleanEngine();
    }
}

bool VIEngine::checkDeviceExtensionSupport(const VkPhysicalDevice &device) {
    uint32_t extensionCount;
    // Getting number of supported extensions
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    // Getting supported extensions
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    return settings.getExtensionsCompatibility(availableExtensions);
}

bool VIEngine::checkQueueFamilyCompatibilityWithDevice(const VkPhysicalDevice &device, VkSurfaceKHR &surface,
                                                       std::optional<uint32_t> &queueFamilyIndex,
                                                       std::optional<uint32_t> &presentQueueFamilyIndex) {
    uint32_t queueFamilyCount = 0;
    // Getting number of supported queue families
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> deviceQueueFamilies(queueFamilyCount);
    // Getting supported queue families
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, deviceQueueFamilies.data());

    uint32_t foundQueueFamilyIndex = -1;

    // Checking if there is the interested queue family with requested flags and with surface support
    auto foundQueueFamily =
            std::ranges::find_if(
                    deviceQueueFamilies,
                    [&foundQueueFamilyIndex, &presentQueueFamilyIndex, &device, &surface, this]
                            (const VkQueueFamilyProperties &queueFamilyProperties) {
                        std::function<bool(const VkQueueFlagBits &)> flagsContainsFunction(
                                [&queueFamilyProperties](const VkQueueFlagBits &flagBits) {
                                    return queueFamilyProperties.queueFlags & flagBits;
                                });
                        // Keeping track of queue family index
                        ++foundQueueFamilyIndex;

                        VkBool32 isSurfaceSupported = false;
                        vkGetPhysicalDeviceSurfaceSupportKHR(device, foundQueueFamilyIndex, surface,
                                                             &isSurfaceSupported);

                        if (isSurfaceSupported) {
                            presentQueueFamilyIndex = foundQueueFamilyIndex;
                        }

                        return settings.getFlagsCompatibility(flagsContainsFunction) && isSurfaceSupported;
                    }
            );

    if (foundQueueFamilyIndex != queueFamilyCount) {
        queueFamilyIndex = foundQueueFamilyIndex;
    }
    return foundQueueFamily != deviceQueueFamilies.end();
}

bool VIEngine::checkSurfaceCapabilitiesFromDevice(const VkPhysicalDevice &device, VkSurfaceKHR &surface,
                                                  VkSurfaceCapabilitiesKHR &surfaceCapabilities,
                                                  std::vector<VkSurfaceFormatKHR> &surfaceAvailableFormats,
                                                  std::vector<VkPresentModeKHR> &surfacePresentationModes) {
    // Obtaining all capabilities of a surface, depending on the linked device
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &surfaceCapabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        surfaceAvailableFormats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                             surfaceAvailableFormats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        surfacePresentationModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount,
                                                  surfacePresentationModes.data());
    }

    return formatCount != 0 && presentModeCount != 0;
}

bool VIEngine::prepareEngine() {
    // GLFW initialisation
    if (!glfwInit()) {
        std::cout << "GLFW not initialised..." << engineStatus;
        return false;
    }

    // Hinting GLFW to not load APIs (Vulkan APIs not defined in GLFW)
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // Initialising window
    mNativeWindow.glfwWindow = glfwCreateWindow(static_cast<int>(settings.getDefaultXRes()),
                                                static_cast<int>(settings.getDefaultYRes()),
                                                settings.kApplicationProgramName.c_str(), nullptr, nullptr);

    if (!mNativeWindow.glfwWindow) {
        std::cout << "GLFW window not initialised..." << engineStatus;
        return false;
    }

    // Getting extensions from GLFW for Vulkan implementation
    mNativeWindow.glfwExtensions = glfwGetRequiredInstanceExtensions(&mNativeWindow.glfwExtensionCount);

    engineStatus = VIEStatus::GLFW_LOADED;

    // Creating an instance for GPU drivers and application connection
    // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkApplicationInfo.html
    //TODO update to 1.3
    VkApplicationInfo applicationInfo{
            VK_STRUCTURE_TYPE_APPLICATION_INFO,                     // sType
            nullptr,                                                // pNext
            settings.kApplicationName.c_str(),                      // pApplicationName
            VK_MAKE_VERSION(settings.kApplicationMajorVersion,      // applicationVersion
                            settings.kApplicationMinorVersion,
                            settings.kApplicationPatchVersion),
            "VulkanIndirectEngine",                                 // pEngineName
            VK_MAKE_VERSION(settings.kEngineMajorVersion,           // engineVersion
                            settings.kEngineMinorVersion,
                            settings.kEnginePatchVersion),
            VK_API_VERSION_1_2                                      // apiVersion
    };

    // 1.9) Adding validation layers
    if (!settings.areValidationLayersEmpty()) {
        // Getting number of available layers
        uint32_t availableInSystemLayersNum = 0;
        vkEnumerateInstanceLayerProperties(&availableInSystemLayersNum, nullptr);

        // Getting all layers, now known the number of layers
        std::vector<VkLayerProperties> availableInSystemLayers(availableInSystemLayersNum);
        vkEnumerateInstanceLayerProperties(&availableInSystemLayersNum, availableInSystemLayers.data());

        settings.eraseValidationLayersIf([&availableInSystemLayers](const char *&requestedLayer) {
            std::string requestedLayerStr(requestedLayer);
            auto foundLayer = std::ranges::find_if(availableInSystemLayers,
                                                   [&requestedLayerStr](const VkLayerProperties &properties) {
                                                       return requestedLayerStr == properties.layerName;
                                                   });

            return foundLayer == availableInSystemLayers.end();
        });
    }

    // 2) Instance info for extensions to integrate global extensions and validation layers
    // TODO implement additional debug
    // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkInstanceCreateInfo.html

    std::cout << settings.getValidationLayersSize() << std::endl;

    for (auto i = 0; i < settings.getValidationLayersSize(); ++i) {
        std::cout << settings.getValidationLayersData()[i] << std::endl;
    }

    VkInstanceCreateInfo engineCreationInfo{
            VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,                     // sType
            nullptr,                                                    // pNext
            0,                                                          // flags
            &applicationInfo,                                           // pApplicationInfo
            static_cast<uint32_t>(settings.getValidationLayersSize()),  // enabledLayerCount
            settings.getValidationLayersData(),                         // ppEnabledLayerNames
            mNativeWindow.glfwExtensionCount,                           // enabledExtensionCount
            mNativeWindow.glfwExtensions                                // ppEnabledExtensionNames
    };

    if (vkCreateInstance(&engineCreationInfo, nullptr, &vkInstance) != VK_SUCCESS) {
        std::cout << "Vulkan instance not created..." << engineStatus;
        return false;
    }

    engineStatus = VIEStatus::VULKAN_INSTANCE_CREATED;

#if _WIN64
    // Creating Vulkan surface based on WindowsNT native bindings
    VkWin32SurfaceCreateInfoKHR ntWindowSurfaceCreationInfo{};
    ntWindowSurfaceCreationInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    // Requesting a "Handle to a window" native Windows object
    ntWindowSurfaceCreationInfo.hwnd = glfwGetWin32Window(mNativeWindow.glfwWindow);
    // Providing a native NT instance
    ntWindowSurfaceCreationInfo.hinstance = GetModuleHandle(nullptr);

    if (vkCreateWin32SurfaceKHR(vkInstance, &ntWindowSurfaceCreationInfo, nullptr, &mSurface.surface) != VK_SUCCESS) {
        std::cout << "Cannot create native NT window surface to bind to Vulkan..." << engineStatus;
        return false;
    }
#elif __linux__
    // Creating Vulkan surface based on Wayland native bindings
        // TODO see if in Windows this creation type is compatible
        if (glfwCreateWindowSurface(mVulkanLibraries.vkInstance, mNativeWindow.glfwWindow, nullptr, &mSurface.surface)) {
            throw VIERunException("Error creating surface in Wayland environment...", VIESettings::engineStatus);
        }
#endif

    engineStatus = VIEStatus::VULKAN_SURFACE_CREATED;

    // Looking for devices
    uint32_t devicesCount = 0;
    vkEnumeratePhysicalDevices(vkInstance, &devicesCount, nullptr);

    // Gathering devices info
    if (devicesCount == 0) {
        std::cout << "No physical devices found for Vulkan rendering..." << engineStatus;
        return false;
    }

    std::vector<VkPhysicalDevice> availableDevices(devicesCount);
    vkEnumeratePhysicalDevices(vkInstance, &devicesCount, availableDevices.data());

    // Boolean for checking if a device has been found
    bool areDevicesSet = false;

    // Selecting and sorting devices
    // For one device, the selection is at the beginning of availableDevices vector
    // TODO optimize here
    for (VkPhysicalDevice &device: availableDevices) {
        if (!settings.isPreferableDevice(device)) {
            continue;
        }

        std::optional<uint32_t> mainDeviceSelectedQueueFamily;
        std::optional<uint32_t> mainDeviceSelectedPresentFamily;
        std::vector<VkSurfaceFormatKHR> surfaceAvailableFormats;
        std::vector<VkPresentModeKHR> surfacePresentationModes;

        bool isDeviceCompatibleWithExtensions = checkDeviceExtensionSupport(device);

        bool isDeviceCompatibleWithQueueFamily =
                checkQueueFamilyCompatibilityWithDevice(device, mSurface.surface, mainDeviceSelectedQueueFamily,
                                                        mainDeviceSelectedPresentFamily);

        bool isSurfaceSwapChainBasicSupportAvailable =
                checkSurfaceCapabilitiesFromDevice(device, mSurface.surface, mSurface.surfaceCapabilities,
                                                   surfaceAvailableFormats, surfacePresentationModes);

        if (isDeviceCompatibleWithExtensions && isDeviceCompatibleWithQueueFamily &&
            isSurfaceSwapChainBasicSupportAvailable) {
            mPhysicalDevice.vkPhysicalDevice = device;
            mPhysicalDevice.vkPhysicalDeviceSelectedQueueFamily = mainDeviceSelectedQueueFamily;
            mPhysicalDevice.vkPhysicalDeviceSelectedPresentFamily = mainDeviceSelectedPresentFamily;
            mPhysicalDevice.surfaceAvailableFormats = std::move(surfaceAvailableFormats);
            mPhysicalDevice.surfacePresentationModes = std::move(surfacePresentationModes);

            areDevicesSet = true;
        }
    }

    if (!areDevicesSet) {
        std::cout << "Error looking for physical device..." << engineStatus;
        return false;
    }

    engineStatus = VIEStatus::VULKAN_PHYSICAL_DEVICES_PREPARED;

    // TODO check if the additional "presentQueue" is necessary for later use
    // Preparing command queue family for the main device
    VkDeviceQueueCreateInfo vkDeviceQueueCreateInfo{
            VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,                         // sType
            nullptr,                                                        // pNext
            0,                                                              // flags
            mPhysicalDevice.vkPhysicalDeviceSelectedQueueFamily.value(),    // queueFamilyIndex
            1,                                                              // queueCount
            &mainQueueFamilyPriority                                        // pQueuePriorities
    };

    // TODO edit so that where vkPhysicalDeviceFeatures is more understandable
    VkDeviceCreateInfo vkDeviceCreateInfo{
            VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,                       // sType
            nullptr,                                                    // pNext
            0,                                                          // flags
            1,                                                          // queueCreateInfoCount
            &vkDeviceQueueCreateInfo,                                   // pQueueCreateInfos
            static_cast<uint32_t>(settings.getValidationLayersSize()),  // enabledLayerCount
            settings.getValidationLayersData(),                         // ppEnabledLayerNames
            static_cast<uint32_t>(settings.getDeviceExtensionsSize()),  // enabledExtensionCount
            settings.getDeviceExtensionsData(),                         // ppEnabledExtensionNames
            &mPhysicalDevice.vkPhysicalDeviceFeatures,                  // pEnabledFeatures
    };

    if (vkCreateDevice(mPhysicalDevice.vkPhysicalDevice, &vkDeviceCreateInfo, nullptr, &vkDevice) != VK_SUCCESS) {
        std::cout << "Vulkan logic device not created..." << engineStatus;
        return false;
    }

    // Obtaining graphics queue family from logic device via stored index
    vkGetDeviceQueue(vkDevice, mPhysicalDevice.vkPhysicalDeviceSelectedQueueFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(vkDevice, mPhysicalDevice.vkPhysicalDeviceSelectedPresentFamily.value(), 0, &presentQueue);

    engineStatus = VIEStatus::VULKAN_DEVICE_CREATED;

    // Selecting surface format (channels and color space)
    // TODO clean here
    auto foundSurfaceFormat(settings.findSurfaceFormat(mPhysicalDevice.surfaceAvailableFormats));

    mSurface.chosenSurfaceFormat = (foundSurfaceFormat != mPhysicalDevice.surfaceAvailableFormats.end()) ?
                                   *foundSurfaceFormat : mPhysicalDevice.surfaceAvailableFormats.at(0);

    // Selecting presentation mode by a swap chain (describing how and when images are represented on screen)
    auto foundSurfacePresentation = std::ranges::find(mPhysicalDevice.surfacePresentationModes,
                                                      settings.getRefreshMode());
    mSurface.chosenSurfacePresentationMode = (foundSurfacePresentation !=
                                              mPhysicalDevice.surfacePresentationModes.end()) ?
                                             *foundSurfacePresentation : VK_PRESENT_MODE_FIFO_KHR;

    // Selecting swap extent (resolution of the swap chain images in pixels)
    if (mSurface.surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        mSwapChain.chosenSwapExtent = mSurface.surfaceCapabilities.currentExtent;
    } else {
        int width;
        int height;
        glfwGetFramebufferSize(mNativeWindow.glfwWindow, &width, &height);

        mSwapChain.chosenSwapExtent.width = std::clamp(static_cast<uint32_t>(width),
                                                       mSurface.surfaceCapabilities.minImageExtent.width,
                                                       mSurface.surfaceCapabilities.maxImageExtent.width);
        mSwapChain.chosenSwapExtent.height = std::clamp(static_cast<uint32_t>(height),
                                                        mSurface.surfaceCapabilities.minImageExtent.height,
                                                        mSurface.surfaceCapabilities.maxImageExtent.height);
    }

    // Setting the number of images that the swap chain needs to create, depending on a necessary minimum and maximum
    uint32_t swapChainImagesCount = std::min(mSurface.surfaceCapabilities.minImageCount + 1,
                                             mSurface.surfaceCapabilities.maxImageCount);

    // Choosing frame handling mode by swap chain
    std::array<uint32_t, 2> queueIndices({mPhysicalDevice.vkPhysicalDeviceSelectedQueueFamily.value(),
                                          mPhysicalDevice.vkPhysicalDeviceSelectedPresentFamily.value()});

    // VIESettings how swap chain will be created
    VkSwapchainCreateInfoKHR swapChainCreationInfo{
            VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,    // sType
            nullptr,                                        // pNext
            0,                                              // flags
            mSurface.surface,                               // surface
            swapChainImagesCount,                           // minImageCount
            mSurface.chosenSurfaceFormat.format,            // imageFormat
            mSurface.chosenSurfaceFormat.colorSpace,        // imageColorSpace
            mSwapChain.chosenSwapExtent,                    // imageExtent
            1,                                              // imageArrayLayers
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,            // imageUsage
            (mPhysicalDevice.vkPhysicalDeviceSelectedQueueFamily !=
             mPhysicalDevice.vkPhysicalDeviceSelectedPresentFamily) ? VK_SHARING_MODE_CONCURRENT
                                                                    : VK_SHARING_MODE_EXCLUSIVE,  // imageSharingMode
            2,                                              // queueFamilyIndexCount
            queueIndices.data(),                            // pQueueFamilyIndices
            mSurface.surfaceCapabilities.currentTransform,  // preTransform
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,              // compositeAlpha
            mSurface.chosenSurfacePresentationMode,         // presentMode
            VK_TRUE,                                        // clipped
            VK_NULL_HANDLE                                  // oldSwapchain
    };

    if (vkCreateSwapchainKHR(vkDevice, &swapChainCreationInfo, nullptr, &mSwapChain.swapChain)) {
        std::cout << "Cannot create swap chain for main device!" << engineStatus;
        return false;
    }

    vkGetSwapchainImagesKHR(vkDevice, mSwapChain.swapChain, &swapChainImagesCount, nullptr);
    mSwapChain.swapChainImages.resize(swapChainImagesCount);
    vkGetSwapchainImagesKHR(vkDevice, mSwapChain.swapChain, &swapChainImagesCount, mSwapChain.swapChainImages.data());

    engineStatus = VIEStatus::VULKAN_SWAP_CHAIN_CREATED;

    // TODO https://vulkan-tutorial.com/en/Drawing_a_triangle/Presentation/Swap_chain get swap chain images?
    // TODO https://vulkan-tutorial.com/en/Drawing_a_triangle/Presentation/Image_views get textures for other parts? example shadow mapping?

    mSwapChain.swapChainImageViews.resize(mSwapChain.swapChainImages.size());

    // TODO improve documentation here (https://vulkan-tutorial.com/en/Drawing_a_triangle/Presentation/Image_views)


    // TODO complete component names (not only last one)
    for (size_t i = 0; i < mSwapChain.swapChainImageViews.size(); ++i) {
        VkImageViewCreateInfo imageViewCreationInfo{
                VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,   // sType
                nullptr,                                    // pNext
                0,                                          // flags
                mSwapChain.swapChainImages.at(i),           // image
                VK_IMAGE_VIEW_TYPE_2D,                      // viewType
                mSurface.chosenSurfaceFormat.format,        // format
                VkComponentMapping{VK_COMPONENT_SWIZZLE_IDENTITY,
                                   VK_COMPONENT_SWIZZLE_IDENTITY,
                                   VK_COMPONENT_SWIZZLE_IDENTITY,
                                   VK_COMPONENT_SWIZZLE_IDENTITY},  // components
                VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT,
                                        0, 1, 0, 1},                // subresourceRange
        };

        if (vkCreateImageView(vkDevice, &imageViewCreationInfo, nullptr, &mSwapChain.swapChainImageViews.at(i)) !=
            VK_SUCCESS) {
            std::cout << fmt::format("Cannot generate image view {}.", i) << engineStatus;
            return false;
        }

        engineStatus = VIEStatus::VULKAN_IMAGE_VIEWS_CREATED;
    }

    // TODO make generic for every pipeline and every input shader and both code and binary
    if (!mShaderPipeline.uberShader) {
        mShaderPipeline.uberShader = std::make_unique<VIEUberShader>("./shaders/debug/shader.vert",
                                                                     "./shaders/debug/shader.frag");
    }

    if ((vertexModule = mShaderPipeline.uberShader->createVertexModuleFromSPIRV(vkDevice)) == nullptr) {
        std::cout << "Cannot create vertex module..." << std::endl;
        return false;
    }

    if ((fragmentModule = mShaderPipeline.uberShader->createFragmentModuleFromSPIRV(vkDevice)) == nullptr) {
        std::cout << "Cannot create fragment module..." << std::endl;
        return false;
    }

    engineStatus = VIEStatus::VULKAN_SHADERS_COMPILED;

    // Shader creation info for stage/pipeline definition (vertex) (phase 2)
    VkPipelineShaderStageCreateInfo vertexShaderStageCreationInfo{
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,    // sType
            nullptr,                                                // pNext
            0,                                                      // flags
            VK_SHADER_STAGE_VERTEX_BIT,                             // stage
            vertexModule,                                           // module
            "main",                                                 // pName
            nullptr                                                 // pSpecializationInfo
    };

    // Shader creation info for stage/pipeline definition (fragment) (phase 6)
    VkPipelineShaderStageCreateInfo fragmentShaderStageCreationInfo{
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,    // sType
            nullptr,                                                // pNext
            0,                                                      // flags
            VK_SHADER_STAGE_FRAGMENT_BIT,                           // stage
            fragmentModule,                                         // module
            "main",                                                 // pName
            nullptr                                                 // pSpecializationInfo
    };

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{
            vertexShaderStageCreationInfo,
            fragmentShaderStageCreationInfo
    };

    ///< Shader creation info for rendering phase 0: vertex data handling
    VkPipelineVertexInputStateCreateInfo vertexShaderInputStageCreationInfo{
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,  // sType
            nullptr,                                                    // pNext
            0,                                                          // flags
            0,                                                          // vertexBindingDescriptionCount
            nullptr,                                                    // pVertexBindingDescriptions
            0,                                                          // vertexAttributeDescriptionCount
            nullptr                                                     // pVertexAttributeDescriptions
    };

    ///< Shader creation info for rendering phase 1: input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreationInfo{
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,    // sType
            nullptr,                                                        // pNext
            0,                                                              // flags
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,                            // topology
            VK_FALSE                                                        // primitiveRestartEnable
    };

    // Setting viewport size wrt framebuffers/swap chain
    mShaderPipeline.viewport = VkViewport{
            0,                                                      // x
            0,                                                      // y
            static_cast<float>(mSwapChain.chosenSwapExtent.width),  // width
            static_cast<float>(mSwapChain.chosenSwapExtent.height), // height
            0.0f,                                                   // minDepth
            1.0f                                                    // maxDepth
    };

    mShaderPipeline.scissorRectangle = VkRect2D{
            {0, 0},                         // offset
            mSwapChain.chosenSwapExtent     // extent
    };

    ///< Shader viewport creation info
    VkPipelineViewportStateCreateInfo viewportStateCreateInfo{
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,  // sType
            nullptr,                                                // pNext
            0,                                                      // flags
            1,                                                      // viewportCount
            &mShaderPipeline.viewport,                              // pViewports
            1,                                                      // scissorCount
            &mShaderPipeline.scissorRectangle                       // pScissors
    };

    // TODO enable for shadow mapping, requires a GPU feature to check in function-like "enableShadowMapping" (maybe presets for each module and submodule)
    ///< Shader creation info for rendering phase 5: rasterization
    // TODO CW or CCW?
    VkPipelineRasterizationStateCreateInfo rasterizationCreationInfo{
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, // sType
            nullptr,                                                    // pNext
            0,                                                          // flags
            VK_FALSE,                                                   // depthClampEnable
            VK_FALSE,                                                   // rasterizerDiscardEnable
            VK_POLYGON_MODE_FILL,                                       // polygonMode
            VK_CULL_MODE_BACK_BIT,                                      // cullMode                 // FRONT for Shadow Mapping
            VK_FRONT_FACE_COUNTER_CLOCKWISE,                            // frontFace
            VK_FALSE,                                                   // depthBiasEnable
            0,                                                          // depthBiasConstantFactor
            VK_FALSE,                                                   // depthClampEnable
            0,                                                          // depthBiasSlopeFactor
            1.0f                                                        // lineWidth
    };

    ///< Multisampling
    VkPipelineMultisampleStateCreateInfo multisamplingCreationInfo{
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,   // sType
            nullptr,                                                    // pNext
            0,                                                          // flags
            VK_SAMPLE_COUNT_4_BIT,                                      // rasterizationSamples
            VK_FALSE,                                                   // sampleShadingEnable
            1.0f,                                                       // sampleShadingEnable
            nullptr,                                                    // pSampleMask
            VK_FALSE,                                                   // alphaToCoverageEnable
            VK_FALSE                                                    // alphaToOneEnable
    };

    // TODO implement DEPTH STENCIL TEST
    // VkPipelineDepthStencilStateCreateInfo

    // https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions
    VkPipelineColorBlendAttachmentState colorBlendAttachmentState{
            VK_TRUE,                                // blendEnable
            VK_BLEND_FACTOR_ONE,                    // srcColorBlendFactor
            VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,    // dstColorBlendFactor
            VK_BLEND_OP_ADD,                        // colorBlendOp
            VK_BLEND_FACTOR_ONE,                    // srcAlphaBlendFactor
            VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,    // dstAlphaBlendFactor
            VK_BLEND_OP_ADD,                        // alphaBlendOp
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT                // colorWriteMask
    };

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,   // sType
            nullptr,                                                    // pNext
            0,                                                          // flags
            VK_TRUE,                                                    // logicOpEnable
            VK_LOGIC_OP_COPY,                                           // logicOp
            1,                                                          // attachmentCount
            &colorBlendAttachmentState,                                 // pAttachments
            {0.0f, 0.0f, 0.0f, 0.0f}                                    // blendConstants[4]
    };

    // TODO save externally as constants
    std::array<VkDynamicState, 2> dynamicStates{
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_LINE_WIDTH
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{
            VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,   // sType
            nullptr,                                                // pNext
            0,                                                      // flags
            dynamicStates.size(),                                   // dynamicStateCount
            dynamicStates.data()                                    // pDynamicStates
    };

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
            VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,          // sType
            nullptr,                                                // pNext
            0,                                                      // flags
            0,                                                      // setLayoutCount
            nullptr,                                                // pSetLayouts
            0,                                                      // pushConstantRangeCount
            nullptr                                                 // pPushConstantRanges
    };

    if (vkCreatePipelineLayout(vkDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        std::cout << "Failed to create pipeline layout..." << std::endl;
        return false;
    }

    engineStatus = VIEStatus::VULKAN_PIPELINE_STATES_PREPARED;

    VkAttachmentDescription colorAttachment{
            0,                                      // flags
            mSurface.chosenSurfaceFormat.format,    // format
            VK_SAMPLE_COUNT_1_BIT,                  // samples
            VK_ATTACHMENT_LOAD_OP_CLEAR,            // loadOp
            VK_ATTACHMENT_STORE_OP_STORE,           // storeOp
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,            // stencilLoadOp
            VK_ATTACHMENT_STORE_OP_DONT_CARE,           // stencilStoreOp
            VK_IMAGE_LAYOUT_UNDEFINED,              // initialLayout
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR         // finalLayout
    };

    VkAttachmentReference colorAttachmentReference{
            0,                                          // attachment
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL    // layout
    };

    VkSubpassDescription subpassDescription{
            0,                                          // flags
            VK_PIPELINE_BIND_POINT_GRAPHICS,            // pipelineBindPoint
            0,                                          // inputAttachmentCount
            nullptr,                                    // pInputAttachments
            1,                                          // colorAttachmentCount
            &colorAttachmentReference,                  // pColorAttachments
            nullptr,                                    // pResolveAttachments
            nullptr,                                    // pDepthStencilAttachment
            0,                                          // preserveAttachmentCount
            nullptr                                     // pPreserveAttachments
    };

    VkSubpassDependency dependency{
            VK_SUBPASS_EXTERNAL,                            // srcSubpass
            0,                                              // dstSubpass
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // srcStageMask
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // dstStageMask
            0,                                              // srcAccessMask
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,           // dstAccessMask
            0                                               // dependencyFlags
    };

    VkRenderPassCreateInfo renderPassCreateInfo{
            VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,  // sType
            nullptr,                                    // pNext
            0,                                          // flags
            1,                                          // attachmentCount
            &colorAttachment,                           // pAttachments
            1,                                          // subpassCount
            &subpassDescription,                        // pSubpasses
            1,                                          // dependencyCount
            &dependency                                 // pDependencies
    };

    if (vkCreateRenderPass(vkDevice, &renderPassCreateInfo, nullptr, &renderPass) != VK_SUCCESS) {
        std::cout << "Failed to create render pass..." << std::endl;
        return false;
    }

    engineStatus = VIEStatus::VULKAN_RENDER_PASSES_GENERATED;

    // TODO https://vulkan-tutorial.com/en/Drawing_a_triangle/Graphics_pipeline_basics/Conclusion
    VkGraphicsPipelineCreateInfo pipelineCreateInfo{
            VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,    // sType
            nullptr,                                            // pNext
            0,                                                  // flags
            shaderStages.size(),                                // stageCount
            shaderStages.data(),                                // pStages
            &vertexShaderInputStageCreationInfo,                // pVertexInputState
            &inputAssemblyCreationInfo,                         // pInputAssemblyState
            nullptr,                                            // pTessellationState
            &viewportStateCreateInfo,                           // pViewportState
            &rasterizationCreationInfo,                         // pRasterizationState
            &multisamplingCreationInfo,                         // pMultisampleState
            nullptr,                                            // pDepthStencilState
            &colorBlendStateCreateInfo,                         // pColorBlendState
            &dynamicStateCreateInfo,                            // pDynamicState
            pipelineLayout,                                     // layout
            renderPass,                                         // renderPass
            0,                                                  // subpass
            VK_NULL_HANDLE,                                     // basePipelineHandle
            -1                                                  // basePipelineIndex
    };

    if (vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline) !=
        VK_SUCCESS) {
        std::cout << "Failed to create graphics pipeline..." << std::endl;
    }

    engineStatus = VIEStatus::VULKAN_GRAPHICS_PIPELINE_GENERATED;

    // Framebuffers linked to swap chains and image views
    swapChainFramebuffers.resize(mSwapChain.swapChainImageViews.size());

    for (size_t i = 0; const VkImageView &attachment: mSwapChain.swapChainImageViews) {
        VkFramebufferCreateInfo framebufferCreateInfo{
                VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
                nullptr,                                    // pNext
                0,                                          // flags
                renderPass,                                 // renderPass
                1,                                          // attachmentCount
                &attachment,                                // pAttachments
                mSwapChain.chosenSwapExtent.width,          // width
                mSwapChain.chosenSwapExtent.height,         // height
                1                                           // layers
        };

        if (vkCreateFramebuffer(vkDevice, &framebufferCreateInfo, nullptr, &swapChainFramebuffers.at(i)) != VK_SUCCESS) {
            std::cout << "Cannot create framebuffer " << i << std::endl;
            return false;
        }

        ++i;
    }

    engineStatus = VIEStatus::VULKAN_FRAMEBUFFERS_CREATED;

    VkCommandPoolCreateInfo commandPoolCreateInfo{
            VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,                     // sType
            nullptr,                                                        // pNext
            0,                                                              // flags
            mPhysicalDevice.vkPhysicalDeviceSelectedQueueFamily.value()     // queueFamilyIndex
    };

    if (vkCreateCommandPool(vkDevice, &commandPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS) {
        std::cout << "Cannot create command pool..." << std::endl;
        return false;
    }

    engineStatus = VIEStatus::VULKAN_COMMAND_POOL_CREATED;

    commandBuffers.resize(swapChainFramebuffers.size());

    VkCommandBufferAllocateInfo commandBufferAllocateInfo{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,     // sType
            nullptr,                                            // pNext
            commandPool,                                        // commandPool
            VK_COMMAND_BUFFER_LEVEL_PRIMARY,                    // level
            static_cast<uint32_t>(commandBuffers.size())        // commandBufferCount
    };

    if (vkAllocateCommandBuffers(vkDevice, &commandBufferAllocateInfo, commandBuffers.data()) != VK_SUCCESS) {
        std::cout << "Cannot create command buffers..." << std::endl;
        return false;
    }

    // TODO integrate custom render pass and draw commands so that others could implement their shaders and related commands
    //  maybe to split in separate function in order to implement one or more lambdas
    for (size_t i = 0; const VkCommandBuffer &buffer: commandBuffers) {
        VkCommandBufferBeginInfo commandBufferBeginInfo{
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,    // sType
                nullptr,                                        // pNext
                0,                                              // flags
                nullptr                                         // pInheritanceInfo
        };

        if (vkBeginCommandBuffer(buffer, &commandBufferBeginInfo) != VK_SUCCESS) {
            std::cout << "Cannot begin recording command buffer " << buffer;
            return false;
        }

        VkClearValue clearColor{
                VkClearColorValue{                  // color
                        {0.0f, 0.0f, 0.0f, 1.0f}    // float32
                }
        };
        VkRenderPassBeginInfo renderPassBeginInfo{
                VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,   // sType
                nullptr,                                    // pNext
                renderPass,                                 // renderPass
                swapChainFramebuffers.at(i),                // framebuffer
                VkRect2D{                                   // renderArea
                        {0, 0},
                        mSwapChain.chosenSwapExtent
                },
                1,                                          // clearValueCount
                &clearColor                                 // pClearValues
        };

        vkCmdBeginRenderPass(buffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        vkCmdDraw(buffer,   // commandBuffer
                  3,        // vertexCount
                  1,        // instanceCount
                  0,        // firstVertex
                  0);       // firstInstance

        vkCmdEndRenderPass(buffer);

        if (vkEndCommandBuffer(buffer) != VK_SUCCESS) {
            std::cout << "Failed to record command buffer..." << std::endl;
            return false;
        }

        ++i;
    }

    engineStatus = VIEStatus::VULKAN_COMMAND_BUFFERS_PREPARED;

    VkSemaphoreCreateInfo semaphoreCreateInfo{
            VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,    // sType
            nullptr,                                    // pNext
            0                                           // flags
    };

    if (vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {
        std::cout << "Cannot create semaphores..." << std::endl;
        return false;
    }

    engineStatus = VIEStatus::VULKAN_SEMAPHORES_CREATED;

    return true;
}

void VIEngine::runEngine() {
    // TODO create function for defining key and mouse inputs
    while(!glfwWindowShouldClose(mNativeWindow.glfwWindow)) {
        glfwPollEvents();

        if (!drawFrame()) {
            break;
        }
    }

    vkDeviceWaitIdle(vkDevice);
}

bool VIEngine::drawFrame() {
    uint32_t imageIndex;
    vkAcquireNextImageKHR(vkDevice, mSwapChain.swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE,
                          &imageIndex);

    // TODO move as constant
    std::array<VkPipelineStageFlags, 1> waitStages {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo{
            VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
            nullptr,                        // pNext
            1,                              // waitSemaphoreCount
            &imageAvailableSemaphore,       // pWaitSemaphores
            waitStages.data(),              // pWaitDstStageMask
            1,                              // commandBufferCount
            &commandBuffers.at(imageIndex), // pCommandBuffers
            1,                              // signalSemaphoreCount
            &renderFinishedSemaphore,       // pSignalSemaphores
    };

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        std::cout << "Cannot submit draw command buffer..." << std::endl;
        return false;
    }

    std::array<VkSwapchainKHR, 1> swapChainsKHR{mSwapChain.swapChain};
    VkPresentInfoKHR presentInfo{
            VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,     // sType
            nullptr,                                // pNext
            1,                                      // waitSemaphoreCount
            &renderFinishedSemaphore,               // pWaitSemaphores
            swapChainsKHR.size(),                   // swapchainCount
            swapChainsKHR.data(),                   // pSwapchains
            &imageIndex,                            // pImageIndices
            nullptr                                 // pResults
    };

    vkQueuePresentKHR(presentQueue, &presentInfo);

    return true;
}

void VIEngine::cleanEngine() {
    if (engineStatus >= VIEStatus::VULKAN_SEMAPHORES_CREATED) {
        vkDestroySemaphore(vkDevice, renderFinishedSemaphore, nullptr);
        vkDestroySemaphore(vkDevice, imageAvailableSemaphore, nullptr);
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
        for (auto &imageView: mSwapChain.swapChainImageViews) {
            vkDestroyImageView(vkDevice, imageView, nullptr);
        }
    }

    if (engineStatus >= VIEStatus::VULKAN_SWAP_CHAIN_CREATED) {
        vkDestroySwapchainKHR(vkDevice, mSwapChain.swapChain, nullptr);
    }

    if (engineStatus >= VIEStatus::VULKAN_INSTANCE_CREATED) {
        vkDestroySurfaceKHR(vkInstance, mSurface.surface, nullptr);
        vkDestroyDevice(vkDevice, nullptr);
        vkDestroyInstance(vkInstance, nullptr);
    }

    if (engineStatus >= VIEStatus::GLFW_LOADED) {
        glfwDestroyWindow(mNativeWindow.glfwWindow);
        glfwTerminate();
    }

    engineStatus = VIEStatus::UNINITIALISED;
}
