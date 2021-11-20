/* Created by LordRibblesdale on 16/10/2021.
 * MIT License
 */

#include "VIEngine.hpp"
#include "VIESettings.hpp"

#include "VIERunException.hpp"

VIEngine::VIEngine(const VIESettings &settings) : settings(settings), engineStatus(VIEStatus::SETTINGS_LOADED) {

}

VIEngine::~VIEngine() {
    cleanEngine();
}

bool VIEngine::checkDeviceExtensionSupport(const VkPhysicalDevice& device) {
    uint32_t extensionCount;
    // Getting number of supported extensions
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    // Getting supported extensions
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    return settings.getExtensionsCompatibility(availableExtensions);
}

bool VIEngine::checkQueueFamilyCompatibilityWithDevice(const VkPhysicalDevice &device, VkSurfaceKHR &surface,
                                                       std::optional<uint32_t>& queueFamilyIndex,
                                                       std::optional<uint32_t>& presentQueueFamilyIndex) {
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
                            (const VkQueueFamilyProperties& queueFamilyProperties) {
                        std::function<bool(const VkQueueFlagBits&)> flagsContainsFunction(
                                [&queueFamilyProperties](const VkQueueFlagBits& flagBits) {
                                    return queueFamilyProperties.queueFlags & flagBits;
                                });
                        // Keeping track of queue family index
                        ++foundQueueFamilyIndex;

                        VkBool32 isSurfaceSupported = false;
                        vkGetPhysicalDeviceSurfaceSupportKHR(device, foundQueueFamilyIndex, surface, &isSurfaceSupported);

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

bool VIEngine::checkSurfaceCapabilitiesFromDevice(const VkPhysicalDevice& device, VkSurfaceKHR& surface,
                                                  VkSurfaceCapabilitiesKHR& surfaceCapabilities,
                                                  std::vector<VkSurfaceFormatKHR>& surfaceAvailableFormats,
                                                  std::vector<VkPresentModeKHR>& surfacePresentationModes) {
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

void VIEngine::initialiseGLFW() {
    // Requiring engine to have at least loaded settings
    if (engineStatus == VIEStatus::SETTINGS_LOADED) {
        // GLFW initialisation
        if (!glfwInit()) {
            throw VIERunException("GLFW not initialised...", engineStatus);
        }

        // Hinting GLFW to not load APIs (Vulkan APIs not defined in GLFW)
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        // Initialising window
        mNativeWindow.glfwWindow = glfwCreateWindow(static_cast<int>(settings.getDefaultXRes()),
                                                    static_cast<int>(settings.getDefaultYRes()),
                                                    settings.getEngineName().c_str(), nullptr, nullptr);

        if (!mNativeWindow.glfwWindow) {
            throw VIERunException("GLFW window not initialised...", engineStatus);
        }

        // Getting extensions from GLFW for Vulkan implementation
        mNativeWindow.glfwExtensions = glfwGetRequiredInstanceExtensions(&mNativeWindow.glfwExtensionCount);

        engineStatus = VIEStatus::GLFW_LOADED;
    } else {
        throw VIERunException("Engine not in SETTINGS_LOADED status.", engineStatus);
    }
}

void VIEngine::initialiseVulkanLibraries() {
    if (engineStatus == VIEStatus::GLFW_LOADED) {
        // Creating an instance for GPU drivers and application connection
        // 1) Application info
        mVulkanLibraries.applicationInfo.pApplicationName = settings.getEngineName().c_str();
        mVulkanLibraries.applicationInfo.pEngineName = settings.getEngineName().c_str();
        // TODO let user choose which application version is
        mVulkanLibraries.applicationInfo.applicationVersion = VK_MAKE_VERSION(settings.getEngineMajorVersion(),
                                                                              settings.getEngineMinorVersion(),
                                                                              settings.getEnginePatchVersion());
        mVulkanLibraries.applicationInfo.engineVersion = VK_MAKE_VERSION(settings.getEngineMajorVersion(),
                                                                         settings.getEngineMinorVersion(),
                                                                         settings.getEnginePatchVersion());
        mVulkanLibraries.applicationInfo.apiVersion = VK_API_VERSION_1_2;

        // 2) Instance info for extensions to integrate global extensions and validation layers
        // TODO implement additional debug
        mVulkanLibraries.engineCreationInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        mVulkanLibraries.engineCreationInfo.pApplicationInfo = &mVulkanLibraries.applicationInfo;
        mVulkanLibraries.engineCreationInfo.enabledExtensionCount = mNativeWindow.glfwExtensionCount;
        mVulkanLibraries.engineCreationInfo.ppEnabledExtensionNames = mNativeWindow.glfwExtensions;

        // 2.1) Adding validation layers
        if (!settings.areValidationLayersEmpty()) {
            // Getting number of available layers
            uint32_t availableInSystemLayersNum = 0;
            vkEnumerateInstanceLayerProperties(&availableInSystemLayersNum, nullptr);

            // Getting all layers, now known the number of layers
            std::vector<VkLayerProperties> availableInSystemLayers(availableInSystemLayersNum);
            vkEnumerateInstanceLayerProperties(&availableInSystemLayersNum, availableInSystemLayers.data());

            settings.eraseValidationLayersIf([&availableInSystemLayers](const char*& requestedLayer){
                std::string requestedLayerStr(requestedLayer);
                auto foundLayer = std::ranges::find_if(availableInSystemLayers,
                                                       [&requestedLayerStr](const VkLayerProperties& properties) {
                                                           return requestedLayerStr == properties.layerName;
                                                       });

                return foundLayer == availableInSystemLayers.end();
            });

            if (settings.areValidationLayersEmpty()) {
                mVulkanLibraries.engineCreationInfo.enabledLayerCount = 0;
            } else {
                mVulkanLibraries.engineCreationInfo.enabledLayerCount = static_cast<uint32_t>(settings.getValidationLayersSize());
                mVulkanLibraries.engineCreationInfo.ppEnabledLayerNames = settings.getValidationLayersData();
            }
        } else {
            // No validation layers imported
            mVulkanLibraries.engineCreationInfo.enabledLayerCount = 0;
        }

        if (vkCreateInstance(&mVulkanLibraries.engineCreationInfo, nullptr, &mVulkanLibraries.vkInstance) != VK_SUCCESS) {
            throw VIERunException("Vulkan instance not created...", engineStatus);
        }

        engineStatus = VIEStatus::VULKAN_INSTANCE_CREATED;
    } else {
        throw VIERunException("Engine not in GLFW_LOADED status.", engineStatus);
    }
}

void VIEngine::prepareMainPhysicalDevices() {
    if (engineStatus == VIEStatus::VULKAN_SURFACE_CREATED) {
        // Looking for devices
        uint32_t devicesCount = 0;
        vkEnumeratePhysicalDevices(mVulkanLibraries.vkInstance, &devicesCount, nullptr);

        // Gathering devices info
        if (devicesCount == 0) {
            throw VIERunException("No physical devices found for Vulkan rendering...", engineStatus);
        }

        std::vector<VkPhysicalDevice> availableDevices(devicesCount);
        vkEnumeratePhysicalDevices(mVulkanLibraries.vkInstance, &devicesCount, availableDevices.data());

        // Boolean for checking if a device has been found
        bool areDevicesSet = false;

        // Selecting and sorting devices
        // For one device, the selection is at the beginning of availableDevices vector
        for (VkPhysicalDevice& device : availableDevices) {
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

            if (isDeviceCompatibleWithExtensions && isDeviceCompatibleWithQueueFamily && isSurfaceSwapChainBasicSupportAvailable) {
                mPhysicalDevice.vkPhysicalDevice = device;
                mPhysicalDevice.vkPhysicalDeviceSelectedQueueFamily = mainDeviceSelectedQueueFamily;
                mPhysicalDevice.vkPhysicalDeviceSelectedPresentFamily = mainDeviceSelectedPresentFamily;
                mPhysicalDevice.surfaceAvailableFormats = std::move(surfaceAvailableFormats);
                mPhysicalDevice.surfacePresentationModes = std::move(surfacePresentationModes);

                areDevicesSet = true;
            }
        }

        if (areDevicesSet) {
            engineStatus = VIEStatus::VULKAN_PHYSICAL_DEVICES_PREPARED;
        } else {
            throw VIERunException("Error looking for physical device...", engineStatus);
        }
    } else {
        throw VIERunException("Engine not in VULKAN_SURFACE_CREATED status.", engineStatus);
    }
}

// TODO make function handle all devices in different moments (via references)
void VIEngine::createLogicDevice() {
    if (engineStatus == VIEStatus::VULKAN_PHYSICAL_DEVICES_PREPARED) {
        // TODO check if the additional "presentQueue" is necessary for later use
        // Preparing command queue family for the main device
        mLogicDevice.vkDeviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        mLogicDevice.vkDeviceQueueCreateInfo.queueFamilyIndex = mPhysicalDevice.vkPhysicalDeviceSelectedQueueFamily.value();
        mLogicDevice.vkDeviceQueueCreateInfo.queueCount = 1;

        // Setting queue priority (array)
        mLogicDevice.vkDeviceQueueCreateInfo.pQueuePriorities = &mainQueueFamilyPriority;

        // Preparing logical device creation procedures
        mLogicDevice.vkDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        mLogicDevice.vkDeviceCreateInfo.pQueueCreateInfos = &mLogicDevice.vkDeviceQueueCreateInfo;
        mLogicDevice.vkDeviceCreateInfo.queueCreateInfoCount = 1;

        // Setting device features
        mLogicDevice.vkDeviceCreateInfo.pEnabledFeatures = &mPhysicalDevice.vkPhysicalDeviceFeatures;

        // Setting extensions to logic device
        mLogicDevice.vkDeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(settings.getDeviceExtensionsSize());
        mLogicDevice.vkDeviceCreateInfo.ppEnabledExtensionNames = settings.getDeviceExtensionsData();

        // Setting validation layers to logic device
        if (settings.areValidationLayersEmpty()) {
            mLogicDevice.vkDeviceCreateInfo.enabledLayerCount = 0;
        } else {
            mLogicDevice.vkDeviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(settings.getValidationLayersSize());
            mLogicDevice.vkDeviceCreateInfo.ppEnabledLayerNames = settings.getValidationLayersData();
        }

        if (vkCreateDevice(mPhysicalDevice.vkPhysicalDevice, &mLogicDevice.vkDeviceCreateInfo, nullptr,
                           &mLogicDevice.vkDevice) != VK_SUCCESS) {
            throw VIERunException("Vulkan logic device not created...", engineStatus);
        }

        // Obtaining graphics queue family from logic device via stored index
        vkGetDeviceQueue(mLogicDevice.vkDevice, mPhysicalDevice.vkPhysicalDeviceSelectedQueueFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(mLogicDevice.vkDevice, mPhysicalDevice.vkPhysicalDeviceSelectedPresentFamily.value(), 0, &presentQueue);

        engineStatus = VIEStatus::VULKAN_DEVICE_CREATED;
    } else {
        throw VIERunException("Engine not in VULKAN_PHYSICAL_DEVICES_PREPARED status.", engineStatus);
    }
}

void VIEngine::prepareWindowSurface() {
    if (engineStatus >= VIEStatus::VULKAN_INSTANCE_CREATED) {
#if _WIN64
        // Creating Vulkan surface based on WindowsNT native bindings
        VkWin32SurfaceCreateInfoKHR ntWindowSurfaceCreationInfo{};
        ntWindowSurfaceCreationInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        // Requesting a "Handle to a window" native Windows object
        ntWindowSurfaceCreationInfo.hwnd = glfwGetWin32Window(mNativeWindow.glfwWindow);
        // Providing a native NT instance
        ntWindowSurfaceCreationInfo.hinstance = GetModuleHandle(nullptr);

        if (vkCreateWin32SurfaceKHR(mVulkanLibraries.vkInstance, &ntWindowSurfaceCreationInfo, nullptr,
                                    &mSurface.surface) != VK_SUCCESS) {
            throw VIERunException("Cannot create native NT window surface to bind to Vulkan...", engineStatus);
        }
#elif __linux__
        // Creating Vulkan surface based on Wayland native bindings
        // TODO see if in Windows this creation type is compatible
        if (glfwCreateWindowSurface(mVulkanLibraries.vkInstance, mNativeWindow.glfwWindow, nullptr, &mSurface.surface)) {
            throw VIERunException("Error creating surface in Wayland environment...", VIESettings::engineStatus);
        }
#endif

        engineStatus = VIEStatus::VULKAN_SURFACE_CREATED;
    } else {
        throw VIERunException("Engine not in VULKAN_INSTANCE_CREATED status.", engineStatus);
    }
}

void VIEngine::prepareSwapChain() {
    if (engineStatus == VIEStatus::VULKAN_DEVICE_CREATED) {
        // Selecting surface format (channels and color space)
        auto foundSurfaceFormat(settings.findSurfaceFormat(mPhysicalDevice.surfaceAvailableFormats));

        mSurface.chosenSurfaceFormat = (foundSurfaceFormat != mPhysicalDevice.surfaceAvailableFormats.end()) ?
                                       *foundSurfaceFormat : mPhysicalDevice.surfaceAvailableFormats.at(0);

        // Selecting presentation mode by a swap chain (describing how and when images are represented on screen)
        auto foundSurfacePresentation = std::ranges::find(mPhysicalDevice.surfacePresentationModes, settings.getRefreshMode());
        mSurface.chosenSurfacePresentationMode = (foundSurfacePresentation != mPhysicalDevice.surfacePresentationModes.end()) ?
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

        // VIESettings how swap chain will be created
        mSwapChain.swapChainCreationInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        mSwapChain.swapChainCreationInfo.surface = mSurface.surface;
        mSwapChain.swapChainCreationInfo.minImageCount = swapChainImagesCount;
        mSwapChain.swapChainCreationInfo.imageFormat = mSurface.chosenSurfaceFormat.format;
        mSwapChain.swapChainCreationInfo.imageColorSpace = mSurface.chosenSurfaceFormat.colorSpace;
        mSwapChain.swapChainCreationInfo.imageExtent = mSwapChain.chosenSwapExtent;
        mSwapChain.swapChainCreationInfo.imageArrayLayers = 1;
        mSwapChain.swapChainCreationInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        // mSwapChain.swapChainCreationInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        // Choosing frame handling mode by swap chain
        std::array<uint32_t, 2> queueIndices({mPhysicalDevice.vkPhysicalDeviceSelectedQueueFamily.value(),
                                              mPhysicalDevice.vkPhysicalDeviceSelectedPresentFamily.value()});

        if (mPhysicalDevice.vkPhysicalDeviceSelectedQueueFamily != mPhysicalDevice.vkPhysicalDeviceSelectedPresentFamily) {
            mSwapChain.swapChainCreationInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            mSwapChain.swapChainCreationInfo.queueFamilyIndexCount = 2;
            mSwapChain.swapChainCreationInfo.pQueueFamilyIndices = queueIndices.data();
        } else {
            mSwapChain.swapChainCreationInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        // Frame transformation on surface swap chain
        mSwapChain.swapChainCreationInfo.preTransform = mSurface.surfaceCapabilities.currentTransform;
        mSwapChain.swapChainCreationInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        mSwapChain.swapChainCreationInfo.presentMode = mSurface.chosenSurfacePresentationMode;
        mSwapChain.swapChainCreationInfo.clipped = VK_TRUE;

        mSwapChain.swapChainCreationInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(mLogicDevice.vkDevice, &mSwapChain.swapChainCreationInfo, nullptr,
                                 &mSwapChain.swapChain)) {
            throw VIERunException("Cannot create swap chain for main device!", engineStatus);
        }

        vkGetSwapchainImagesKHR(mLogicDevice.vkDevice, mSwapChain.swapChain, &swapChainImagesCount, nullptr);
        mSwapChain.swapChainImages.resize(swapChainImagesCount);
        vkGetSwapchainImagesKHR(mLogicDevice.vkDevice, mSwapChain.swapChain, &swapChainImagesCount,
                                mSwapChain.swapChainImages.data());

        engineStatus = VIEStatus::VULKAN_SWAP_CHAIN_CREATED;

        // TODO https://vulkan-tutorial.com/en/Drawing_a_triangle/Presentation/Swap_chain get swap chain images?
        // TODO https://vulkan-tutorial.com/en/Drawing_a_triangle/Presentation/Image_views get textures for other parts? example shadow mapping?
    } else {
        throw VIERunException("Engine not in VULKAN_PHYSICAL_DEVICES_PREPARED status.", engineStatus);
    }
}

void VIEngine::cleanEngine() const {
    if (engineStatus >= VIEStatus::VULKAN_SWAP_CHAIN_CREATED) {
        vkDestroySwapchainKHR(mLogicDevice.vkDevice, mSwapChain.swapChain, nullptr);
    }

    if (engineStatus >= VIEStatus::VULKAN_INSTANCE_CREATED) {
        vkDestroySurfaceKHR(mVulkanLibraries.vkInstance, mSurface.surface, nullptr);
        vkDestroyDevice(mLogicDevice.vkDevice, nullptr);
        vkDestroyInstance(mVulkanLibraries.vkInstance, nullptr);
    }

    if (engineStatus >= VIEStatus::GLFW_LOADED) {
        glfwDestroyWindow(mNativeWindow.glfwWindow);
        glfwTerminate();
    }
}

void VIEngine::prepareEngine() {
    try {
        initialiseGLFW();
        initialiseVulkanLibraries();
        prepareWindowSurface();
        prepareMainPhysicalDevices();
        createLogicDevice();
        prepareSwapChain();
    } catch (const VIERunException& e) {
        std::cout << fmt::format("VIERunException::what(): {}\nCleaning and closing engine.\n", e.errorMessage());
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    // TODO import models (using lambdas to describe how to convert personal vector structure to internal drawing struct)
    //  (VIEModule)

    // TODO set a runEngine function (with lambdas for describing some parts (the order of drawing, how to do it, with
    //  a list of functions???)
}
