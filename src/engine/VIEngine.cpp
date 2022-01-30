/* Created by LordRibblesdale on 16/10/2021.
 * MIT License
 */

#include "engine/VIEngine.hpp"
#include "engine/VIESettings.hpp"
#include "tools/VIETools.hpp"

// TODO fix with logger
#define return_log_if(if_condition, string, ret_condition)  \
if(if_condition) {                                          \
    std::cout << (string) << std::endl;                     \
    return ret_condition;                                   \
}

VIEngine::VIEngine(const VIESettings &settings) : settings(settings) {}

VIEngine::~VIEngine() {
    if (engineStatus != VIEStatus::UNINITIALISED) {
        cleanEngine();
    }
}

bool VIEngine::checkQueueFamilyCompatibilityWithDevice(const VkPhysicalDevice &device, VkSurfaceKHR &selectedSurface,
                                                       std::optional<uint32_t> &queueFamilyIndex,
                                                       std::optional<uint32_t> &presentQueueFamilyIndex) {

    uint32_t queueFamilyCount = 0;
    // Getting number of supported queue families
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> deviceQueueFamilies(queueFamilyCount);
    // Getting supported queue families
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, deviceQueueFamilies.data());

    queueFamilyCount = std::numeric_limits<uint32_t>::max();
    // Checking if there is the interested queue family with requested flags and with selectedSurface support
    bool foundQueueFamily =
            std::ranges::any_of(
                    deviceQueueFamilies,
                    [&queueFamilyCount, &presentQueueFamilyIndex, &device, &selectedSurface, this]
                            (const VkQueueFamilyProperties &queueFamilyProperties) {
                        auto flagsContainsFunction([&queueFamilyProperties](const VkQueueFlagBits &flagBits) {
                            return queueFamilyProperties.queueFlags & flagBits;
                        });
                        // Keeping track of queue family index
                        ++queueFamilyCount;

                        VkBool32 isSurfaceSupported = false;
                        vkGetPhysicalDeviceSurfaceSupportKHR(device, queueFamilyCount, selectedSurface,
                                                             &isSurfaceSupported);

                        if (isSurfaceSupported) {
                            presentQueueFamilyIndex = queueFamilyCount;

                            return std::ranges::all_of(settings.defaultFlags, flagsContainsFunction) &&
                                   std::ranges::all_of(settings.preferredFlagBits, flagsContainsFunction);
                        }

                        return false;
                    });

    if (queueFamilyCount != deviceQueueFamilies.size()) {
        queueFamilyIndex = queueFamilyCount;
    }

    return foundQueueFamily;
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
    // TODO condensate into internal structures
    uint32_t glfwExtensionCount{};         ///< GLFW extensions count for Vulkan ext. initialisation
    const char **glfwExtensions{};         /**< GLFW extensions (GLFW APIs and functions) to be used by
                                            *    Vulkan for interacting with window */

    VkPhysicalDevice vkPhysicalDevice{};                    /**< Vulkan physical device object (for used device
                                                             *    representation) */
    VkPhysicalDeviceFeatures vkPhysicalDeviceFeatures{};    ///< Main device features to set for chosen device
    std::optional<uint32_t> selectedQueueFamily;            ///< Queue family chosen for the main device
    std::optional<uint32_t> selectedPresentFamily;          ///< Present family chosen for the mail device
    std::vector<VkSurfaceFormatKHR> surfaceAvailableFormats;    ///< List of available surface color spaces for the surface
    std::vector<VkPresentModeKHR> surfacePresentationModes;     ///< List of available presentation modes for the surface
    float mainQueueFamilyPriority = 1.0f;                   ///< Main queue family priority

    VkSurfaceFormatKHR chosenSurfaceFormat{};               ///< Window surface chosen format and color space
    VkPresentModeKHR chosenSurfacePresentationMode{};       /**< Window surface images refresh and representation mode
                                                             *    on surface */
    VkSurfaceCapabilitiesKHR surfaceCapabilities{};         ///< Window surface capabilities for swap chain implementation

    VkExtent2D chosenSwapExtent{};                          ///< Swap chain images resolution definition
    std::vector<VkImage> swapChainImages{};                 ///< Swap chain extracted images

    std::array<VkDynamicState, 2> dynamicStates{
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_LINE_WIDTH
    };

    // TODO std::optional viewport// TODO set as an array for

    auto initializeGlfw([this, &glfwExtensionCount, &glfwExtensions]() {
        // GLFW initialisation
        return_log_if(!glfwInit(), "GLFW not initialised...", false)

        // Hinting GLFW to not load APIs (Vulkan APIs not defined in GLFW)
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        glfwWindow = glfwCreateWindow(static_cast<int>(settings.kDefaultXRes),
                                      static_cast<int>(settings.kDefaultYRes),
                                      settings.kApplicationProgramName.c_str(), nullptr, nullptr);

        return_log_if(!glfwWindow, "GLFW window not initialised...", false)

        // Getting extensions from GLFW for Vulkan implementation
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        return true;
    });

    auto createVulkanInstance([this, &glfwExtensionCount, &glfwExtensions]() {
        // Creating an instance for GPU drivers and application connection
        // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkApplicationInfo.html
        //TODO update to 1.3
        VkApplicationInfo applicationInfo{
                VK_STRUCTURE_TYPE_APPLICATION_INFO,                     // sType
                nullptr,                                                // pNext
                settings.kApplicationName.c_str(),                      // pApplicationName
                settings.kApplicationVersion,                           // applicationVersion
                "VulkanIndirectEngine",                                 // pEngineName
                settings.kEngineVersion,                                // engineVersion
                VK_API_VERSION_1_2                                      // apiVersion
        };

        // 1.9) Checking validation layers
        tools::checkValidationLayersCompatibility(settings.validationLayers);

        // 2) Instance info for extensions to integrate global extensions and validation layers
        // TODO implement additional debug
        // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkInstanceCreateInfo.html

        VkInstanceCreateInfo engineCreationInfo{
                VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,                     // sType
                nullptr,                                                    // pNext
                0,                                                          // flags
                &applicationInfo,                                           // pApplicationInfo
                static_cast<uint32_t>(settings.validationLayers.size()),    // enabledLayerCount
                settings.validationLayers.data(),                           // ppEnabledLayerNames
                glfwExtensionCount,                                         // enabledExtensionCount
                glfwExtensions                                              // ppEnabledExtensionNames
        };

        return_log_if(vkCreateInstance(&engineCreationInfo, nullptr, &vkInstance) != VK_SUCCESS,
                      "Vulkan instance not created...", false)

        return true;
    });

    auto createWindowSurface([this]() {
#if _WIN64
        // Creating Vulkan surface based on WindowsNT native bindings
        VkWin32SurfaceCreateInfoKHR ntWindowSurfaceCreationInfo{
                VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,    // sType
                nullptr,                                            // pNext
                0,                                                  // flags
                GetModuleHandle(nullptr),                           // hinstance
                glfwGetWin32Window(glfwWindow)                      // hwnd
        };

        return_log_if(
                vkCreateWin32SurfaceKHR(vkInstance, &ntWindowSurfaceCreationInfo, nullptr, &surface) != VK_SUCCESS,
                "Cannot create native NT window surface to bind to Vulkan...", false)
#elif __linux__
        // Creating Vulkan surface based on Wayland native bindings
        // TODO see if in Windows this creation type is compatible
        if (glfwCreateWindowSurface(mVulkanLibraries.vkInstance, mNativeWindow.glfwWindow, nullptr, &mSurface.surface)) {
            throw VIERunException("Error creating surface in Wayland environment...", VIESettings::engineStatus);
        }
#endif

        return true;
    });

    auto preparePhysicalDevice([this, &vkPhysicalDevice, &selectedPresentFamily, &selectedQueueFamily,
                                       &surfaceCapabilities, &surfaceAvailableFormats]() {
        // Looking for devices
        uint32_t devicesCount = 0;
        vkEnumeratePhysicalDevices(vkInstance, &devicesCount, nullptr);

        // Gathering devices info
        return_log_if(devicesCount == 0, "No physical devices found for Vulkan rendering...", false)

        std::vector<VkPhysicalDevice> availableDevices(devicesCount);
        vkEnumeratePhysicalDevices(vkInstance, &devicesCount, availableDevices.data());

        erase_if(availableDevices, [this](const VkPhysicalDevice &physicalDevice) {
            return settings.preferredDeviceSelectionFunction ? !settings.preferredDeviceSelectionFunction(
                    physicalDevice)
                                                             : true;
        });

        // Boolean for checking if a device has been found
        bool areDevicesSet = false;

        // Selecting and sorting devices
        // For one device, the selection is at the beginning of availableDevices vector
        for (VkPhysicalDevice &device: availableDevices) {
            std::optional<uint32_t> mainDeviceSelectedQueueFamily;
            std::optional<uint32_t> mainDeviceSelectedPresentFamily;
            std::vector<VkSurfaceFormatKHR> surfaceFormats;
            std::vector<VkPresentModeKHR> surfacePresentModes;

            bool isDeviceCompatibleWithExtensions = tools::checkDeviceExtensionSupport(device,
                                                                                       settings.deviceExtensions);

            // TODO optimize those two checks
            bool isDeviceCompatibleWithQueueFamily =
                    checkQueueFamilyCompatibilityWithDevice(device, surface, mainDeviceSelectedQueueFamily,
                                                            mainDeviceSelectedPresentFamily);

            bool isSurfaceSwapChainBasicSupportAvailable =
                    checkSurfaceCapabilitiesFromDevice(device, surface, surfaceCapabilities,
                                                       surfaceFormats, surfacePresentModes);

            if (isDeviceCompatibleWithExtensions && isDeviceCompatibleWithQueueFamily &&
                isSurfaceSwapChainBasicSupportAvailable) {
                vkPhysicalDevice = device;
                selectedQueueFamily = mainDeviceSelectedQueueFamily;
                selectedPresentFamily = mainDeviceSelectedPresentFamily;
                surfaceFormats = std::move(surfaceFormats);
                surfacePresentModes = std::move(surfacePresentModes);

                areDevicesSet = true;
            }
        }

        return_log_if(!areDevicesSet, "Error looking for physical device...", false)

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            surfaceAvailableFormats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, surface, &formatCount,
                                                 surfaceAvailableFormats.data());
        }

        return true;
    });

    auto prepareLogicalDevice([this, &vkPhysicalDevice, &vkPhysicalDeviceFeatures, &selectedQueueFamily,
                                      &selectedPresentFamily, &mainQueueFamilyPriority]() {
        // TODO check if the additional "presentQueue" is necessary for later use
        // Preparing command queue family for the main device
        VkDeviceQueueCreateInfo vkDeviceQueueCreateInfo{
                VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,     // sType
                nullptr,                                    // pNext
                0,                                          // flags
                selectedQueueFamily.value(),                // queueFamilyIndex
                1,                                          // queueCount
                &mainQueueFamilyPriority                    // pQueuePriorities
        };

        // TODO edit so that where vkPhysicalDeviceFeatures is more understandable
        VkDeviceCreateInfo vkDeviceCreateInfo{
                VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,                       // sType
                nullptr,                                                    // pNext
                0,                                                          // flags
                1,                                                          // queueCreateInfoCount
                &vkDeviceQueueCreateInfo,                                   // pQueueCreateInfos
                static_cast<uint32_t>(settings.validationLayers.size()),    // enabledLayerCount
                settings.validationLayers.data(),                           // ppEnabledLayerNames
                static_cast<uint32_t>(settings.deviceExtensions.size()),    // enabledExtensionCount
                settings.deviceExtensions.data(),                           // ppEnabledExtensionNames
                &vkPhysicalDeviceFeatures,                  // pEnabledFeatures
        };

        return_log_if(vkCreateDevice(vkPhysicalDevice, &vkDeviceCreateInfo, nullptr, &vkDevice) != VK_SUCCESS,
                      "Vulkan logic device not created...", false)

        // Obtaining graphics queue family from logic device via stored index
        vkGetDeviceQueue(vkDevice, selectedQueueFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(vkDevice, selectedPresentFamily.value(), 0, &presentQueue);

        return true;
    });

    auto createSwapchains([this, &surfaceAvailableFormats, &chosenSurfaceFormat, &surfacePresentationModes,
                                  &chosenSurfacePresentationMode, &surfaceCapabilities, &chosenSwapExtent,
                                  &selectedQueueFamily, &selectedPresentFamily, &swapChainImages]() {
        // Selecting surface format (channels and color space)
        return_log_if(!tools::selectSurfaceFormat(surfaceAvailableFormats, settings.defaultFormat,
                                                  settings.defaultColorSpace, chosenSurfaceFormat),
                      "No compatible surface format found for main physical device...", false)

        // Selecting presentation mode by a swap chain (describing how and when images are represented on screen)
        chosenSurfacePresentationMode = tools::selectSurfacePresentation(surfacePresentationModes,
                                                                         settings.preferredPresentMode);

        // Selecting swap extent (resolution of the swap chain images in pixels)
        if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            chosenSwapExtent = surfaceCapabilities.currentExtent;
        } else {
            int width;
            int height;
            glfwGetFramebufferSize(glfwWindow, &width, &height);

            chosenSwapExtent = VkExtent2D{
                    std::clamp(static_cast<uint32_t>(width), surfaceCapabilities.minImageExtent.width,
                               surfaceCapabilities.maxImageExtent.width),
                    std::clamp(static_cast<uint32_t>(height), surfaceCapabilities.minImageExtent.height,
                               surfaceCapabilities.maxImageExtent.height)
            };
        }

        // Setting the number of images that the swap chain needs to create, depending on a necessary minimum and maximum
        uint32_t swapChainImagesCount = std::min(surfaceCapabilities.minImageCount + 1,
                                                 surfaceCapabilities.maxImageCount);

        // Choosing frame handling mode by swap chain
        std::array<uint32_t, 2> queueIndices({selectedQueueFamily.value(),
                                              selectedPresentFamily.value()});

        // VIESettings how swap chain will be created
        VkSwapchainCreateInfoKHR swapChainCreationInfo{
                VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,    // sType
                nullptr,                                        // pNext
                0,                                              // flags
                surface,                                        // surface
                swapChainImagesCount,                           // minImageCount
                chosenSurfaceFormat.format,                     // imageFormat
                chosenSurfaceFormat.colorSpace,                 // imageColorSpace
                chosenSwapExtent,                               // imageExtent
                1,                                              // imageArrayLayers
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,            // imageUsage
                (selectedQueueFamily != selectedPresentFamily) ? VK_SHARING_MODE_CONCURRENT
                                                               : VK_SHARING_MODE_EXCLUSIVE, // imageSharingMode
                2,                                              // queueFamilyIndexCount
                queueIndices.data(),                            // pQueueFamilyIndices
                surfaceCapabilities.currentTransform,           // preTransform
                VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,              // compositeAlpha
                chosenSurfacePresentationMode,                  // presentMode
                VK_TRUE,                                        // clipped
                VK_NULL_HANDLE                                  // oldSwapchain
        };

        return_log_if(vkCreateSwapchainKHR(vkDevice, &swapChainCreationInfo, nullptr, &swapChain),
                      "Cannot create swap chain for main device!", false)

        vkGetSwapchainImagesKHR(vkDevice, swapChain, &swapChainImagesCount, nullptr);
        swapChainImages.resize(swapChainImagesCount);
        vkGetSwapchainImagesKHR(vkDevice, swapChain, &swapChainImagesCount, swapChainImages.data());

        return true;
    });

    auto createImageViews([this, &swapChainImages, &chosenSurfaceFormat]() {
        swapChainImageViews.resize(swapChainImages.size());

        // TODO complete component names (not only last one)
        for (size_t i = 0; VkImageView &imageView: swapChainImageViews) {
            VkImageViewCreateInfo imageViewCreationInfo{
                    VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,           // sType
                    nullptr,                                            // pNext
                    0,                                                  // flags
                    swapChainImages.at(i),                              // image
                    VK_IMAGE_VIEW_TYPE_2D,                              // viewType
                    chosenSurfaceFormat.format,                         // format
                    VkComponentMapping{VK_COMPONENT_SWIZZLE_IDENTITY,
                                       VK_COMPONENT_SWIZZLE_IDENTITY,
                                       VK_COMPONENT_SWIZZLE_IDENTITY,
                                       VK_COMPONENT_SWIZZLE_IDENTITY},  // components
                    VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT,
                                            0, 1, 0, 1}                 // subresourceRange
            };

            return_log_if(vkCreateImageView(vkDevice, &imageViewCreationInfo, nullptr, &imageView) != VK_SUCCESS,
                          fmt::format("Cannot generate image view {}.", i), false)

            ++i;
        }

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

    auto prepareFixedPipelineFunctions([this]() {
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
                VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,          // sType
                nullptr,                                                // pNext
                0,                                                      // flags
                0,                                                      // setLayoutCount
                nullptr,                                                // pSetLayouts
                0,                                                      // pushConstantRangeCount
                nullptr                                                 // pPushConstantRanges
        };

        return_log_if(
                vkCreatePipelineLayout(vkDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS,
                "Failed to create pipeline layout...", false)

        return true;
    });

    auto prepareRenderPasses([this, &chosenSurfaceFormat]() {
        VkAttachmentDescription colorAttachment{
                0,                                      // flags
                chosenSurfaceFormat.format,    // format
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

        return_log_if(vkCreateRenderPass(vkDevice, &renderPassCreateInfo, nullptr, &renderPass) != VK_SUCCESS,
                      "Failed to create render pass...", false)

        return true;
    });

    auto generateGraphicsPipeline([this, &chosenSwapExtent, &dynamicStates]() {
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
        VkViewport viewport{
                0,                                              // x
                0,                                              // y
                static_cast<float>(chosenSwapExtent.width),     // width
                static_cast<float>(chosenSwapExtent.height),    // height
                0.0f,                                           // minDepth
                1.0f                                            // maxDepth
        };

        VkRect2D scissorRectangle{
                {0, 0},             // offset
                chosenSwapExtent    // extent
        };

        ///< Shader viewport creation info
        VkPipelineViewportStateCreateInfo viewportStateCreateInfo{
                VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,  // sType
                nullptr,                                                // pNext
                0,                                                      // flags
                1,                                                      // viewportCount
                &viewport,                                              // pViewports
                1,                                                      // scissorCount
                &scissorRectangle                                       // pScissors
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
                VK_FRONT_FACE_CLOCKWISE,                                    // frontFace
                VK_FALSE,                                                   // depthBiasEnable
                0.0f,                                                       // depthBiasConstantFactor
                0.0f,                                                       // depthBiasClamp
                0.0f,                                                       // depthBiasSlopeFactor
                1.0f                                                        // lineWidth
        };

        ///< Multisampling
        VkPipelineMultisampleStateCreateInfo multisamplingCreationInfo{
                VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,   // sType
                nullptr,                                                    // pNext
                0,                                                          // flags
                VK_SAMPLE_COUNT_1_BIT,                                      // rasterizationSamples
                VK_FALSE,                                                   // sampleShadingEnable
                1.0f,                                                       // minSampleShading
                nullptr,                                                    // pSampleMask
                VK_FALSE,                                                   // alphaToCoverageEnable
                VK_FALSE                                                    // alphaToOneEnable
        };

        // TODO implement DEPTH STENCIL TEST
        // VkPipelineDepthStencilStateCreateInfo

        // https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions
        VkPipelineColorBlendAttachmentState colorBlendAttachmentState{
                VK_FALSE,                               // blendEnable
                VK_BLEND_FACTOR_ONE,                    // srcColorBlendFactor
                VK_BLEND_FACTOR_ZERO,                   // dstColorBlendFactor
                VK_BLEND_OP_ADD,                        // colorBlendOp
                VK_BLEND_FACTOR_ONE,                    // srcAlphaBlendFactor
                VK_BLEND_FACTOR_ZERO,                   // dstAlphaBlendFactor
                VK_BLEND_OP_ADD,                        // alphaBlendOp
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT                // colorWriteMask
        };

        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{
                VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,   // sType
                nullptr,                                                    // pNext
                0,                                                          // flags
                VK_FALSE,                                                   // logicOpEnable
                VK_LOGIC_OP_COPY,                                           // logicOp
                1,                                                          // attachmentCount
                &colorBlendAttachmentState,                                 // pAttachments
                {0.0f, 0.0f, 0.0f, 0.0f}                                    // blendConstants[4]
        };

        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{
                VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,   // sType
                nullptr,                                                // pNext
                0,                                                      // flags
                dynamicStates.size(),                                   // dynamicStateCount
                dynamicStates.data()                                    // pDynamicStates
        };

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

        return_log_if(vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,
                                                &graphicsPipeline) != VK_SUCCESS,
                      "Failed to create graphics pipeline...", false)

        return true;
    });

    auto initializeFramebuffers([this, &chosenSwapExtent]() {
        // Framebuffers linked to swap chains and image views
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; const VkImageView &attachment: swapChainImageViews) {
            VkFramebufferCreateInfo framebufferCreateInfo{
                    VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
                    nullptr,                                    // pNext
                    0,                                          // flags
                    renderPass,                                 // renderPass
                    1,                                          // attachmentCount
                    &attachment,                                // pAttachments
                    chosenSwapExtent.width,                     // width
                    chosenSwapExtent.height,                    // height
                    1                                           // layers
            };

            return_log_if(vkCreateFramebuffer(vkDevice, &framebufferCreateInfo, nullptr,
                                              &swapChainFramebuffers.at(i)) != VK_SUCCESS,
                          fmt::format("Cannot create framebuffer {}", i), false)

            ++i;
        }

        return true;
    });

    auto createCommandPool([this, &selectedQueueFamily]() {
        VkCommandPoolCreateInfo commandPoolCreateInfo{
                VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,                     // sType
                nullptr,                                                        // pNext
                0,                                                              // flags
                selectedQueueFamily.value()     // queueFamilyIndex
        };

        return_log_if(vkCreateCommandPool(vkDevice, &commandPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS,
                      "Cannot create command pool...", false)

        return true;
    });

    auto prepareCommandBuffers([this, &chosenSwapExtent]() {
        commandBuffers.resize(swapChainFramebuffers.size());

        VkCommandBufferAllocateInfo commandBufferAllocateInfo{
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,     // sType
                nullptr,                                            // pNext
                commandPool,                                        // commandPool
                VK_COMMAND_BUFFER_LEVEL_PRIMARY,                    // level
                static_cast<uint32_t>(commandBuffers.size())        // commandBufferCount
        };

        return_log_if(
                vkAllocateCommandBuffers(vkDevice, &commandBufferAllocateInfo, commandBuffers.data()) != VK_SUCCESS,
                "Cannot create command buffers...", false)

        // TODO integrate custom render pass and draw commands so that others could implement their shaders and related commands
        //  maybe to split in separate function in order to implement one or more lambdas
        for (size_t i = 0; const VkCommandBuffer &buffer: commandBuffers) {
            VkCommandBufferBeginInfo commandBufferBeginInfo{
                    VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,    // sType
                    nullptr,                                        // pNext
                    0,                                              // flags
                    nullptr                                         // pInheritanceInfo
            };

            return_log_if(vkBeginCommandBuffer(buffer, &commandBufferBeginInfo) != VK_SUCCESS,
                          fmt::format("Cannot begin recording command buffer {}", i), false)

            VkClearValue clearColor{{{0.0f, 0.0f, 0.0f, 1.0f}}};
            VkRenderPassBeginInfo renderPassBeginInfo{
                    VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,   // sType
                    nullptr,                                    // pNext
                    renderPass,                                 // renderPass
                    swapChainFramebuffers.at(i),                // framebuffer
                    VkRect2D{{0, 0}, chosenSwapExtent},         // renderArea
                    1,                                          // clearValueCount
                    &clearColor                                 // pClearValues
            };

            vkCmdBeginRenderPass(buffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

            vkCmdDraw(buffer, 3, 1, 0, 0);

            vkCmdEndRenderPass(buffer);

            return_log_if(vkEndCommandBuffer(buffer) != VK_SUCCESS, "Failed to record command buffer...", false)

            ++i;
        }

        return true;
    });

    auto createSemaphores([this]() {
        VkSemaphoreCreateInfo semaphoreCreateInfo{
                VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,    // sType
                nullptr,                                    // pNext
                0                                           // flags
        };

        return_log_if(
                (vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
                 vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS),
                "Cannot create semaphores...", false)

        return true;
    });

    return_log_if(!initializeGlfw(), "Error 1", false)
    engineStatus = VIEStatus::GLFW_LOADED;

    return_log_if(!createVulkanInstance(), "Error 2", false)
    engineStatus = VIEStatus::VULKAN_INSTANCE_CREATED;

    return_log_if(!createWindowSurface(), "Error 3", false)
    engineStatus = VIEStatus::VULKAN_SURFACE_CREATED;

    return_log_if(!preparePhysicalDevice(), "Error 4", false)
    engineStatus = VIEStatus::VULKAN_PHYSICAL_DEVICES_PREPARED;

    return_log_if(!prepareLogicalDevice(), "Error 5", false)
    engineStatus = VIEStatus::VULKAN_LOGICAL_DEVICE_CREATED;

    return_log_if(!createSwapchains(), "Error 6", false)
    engineStatus = VIEStatus::VULKAN_SWAP_CHAIN_CREATED;

    return_log_if(!createImageViews(), "Error 7", false)
    engineStatus = VIEStatus::VULKAN_IMAGE_VIEWS_CREATED;

    return_log_if(!generateShaderModules(), "Error 8", false)
    engineStatus = VIEStatus::VULKAN_SHADERS_COMPILED;

    return_log_if(!prepareFixedPipelineFunctions(), "Error 9", false)
    engineStatus = VIEStatus::VULKAN_PIPELINE_STATES_PREPARED;

    return_log_if(!prepareRenderPasses(), "Error 10", false)
    engineStatus = VIEStatus::VULKAN_RENDER_PASSES_GENERATED;

    return_log_if(!generateGraphicsPipeline(), "Error 11", false)
    engineStatus = VIEStatus::VULKAN_GRAPHICS_PIPELINE_GENERATED;

    return_log_if(!initializeFramebuffers(), "Error 12", false)
    engineStatus = VIEStatus::VULKAN_FRAMEBUFFERS_CREATED;

    return_log_if(!createCommandPool(), "Error 13", false)
    engineStatus = VIEStatus::VULKAN_COMMAND_POOL_CREATED;

    return_log_if(!prepareCommandBuffers(), "Error 14", false)
    engineStatus = VIEStatus::VULKAN_COMMAND_BUFFERS_PREPARED;

    return_log_if(!createSemaphores(), "Error 15", false)
    engineStatus = VIEStatus::VULKAN_SEMAPHORES_CREATED;

    return true;
}

void VIEngine::runEngine() {
    if (engineStatus < VIEStatus::VULKAN_SEMAPHORES_CREATED) {
        return;
    }

    // TODO create function for defining key and mouse inputs
    while (!glfwWindowShouldClose(glfwWindow)) {
        glfwPollEvents();

        if (!drawFrame()) {
            break;
        }
    }

    vkDeviceWaitIdle(vkDevice);
}

bool VIEngine::drawFrame() {
    uint32_t imageIndex;
    vkAcquireNextImageKHR(vkDevice, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    // TODO move as constant
    std::array<VkPipelineStageFlags, 1> waitStages{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
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

    return_log_if(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS,
                  "Cannot submit draw command buffer...", false)

    std::array<VkSwapchainKHR, 1> swapChainsKHR{swapChain};
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
        glfwDestroyWindow(glfwWindow);
        glfwTerminate();
    }

    engineStatus = VIEStatus::UNINITIALISED;
}
