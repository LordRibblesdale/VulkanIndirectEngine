/* Created by LordRibblesdale on 16/10/2021.
 * MIT License
 */

#include "VIEngine.hpp"
#include "../system/Settings.hpp"
#include "VIERunException.hpp"

#define FMT_HEADER_ONLY
#include <fmt/format.h>

bool VIEngine::checkDeviceExtensionSupport(const VkPhysicalDevice& device) {
    unsigned int extensionCount;
    // Getting number of supported extensions
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    // Getting supported extensions
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    bool areExtensionsCompatible =
            std::ranges::all_of(Settings::deviceExtensions,
                                [&availableExtensions](const char*& extension) {
                                    std::string extensionStr(extension);
                                    auto foundExtension =
                                            std::ranges::find_if(availableExtensions,
                                                                 [&extensionStr](const VkExtensionProperties& extensionProperties) {
                                                                     return extensionStr == extensionProperties.extensionName;
                                                                 });

                                    return foundExtension != availableExtensions.end();
                                });

    return areExtensionsCompatible;
}

bool VIEngine::checkQueueFamilyCompatibilityWithDevice(const VkPhysicalDevice &device, VkSurfaceKHR &surface,
                                                       std::optional<unsigned int>& queueFamilyIndex,
                                                       std::optional<unsigned int>& presentQueueFamilyIndex) {
    unsigned int queueFamilyCount = 0;
    // Getting number of supported queue families
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> deviceQueueFamilies(queueFamilyCount);
    // Getting supported queue families
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, deviceQueueFamilies.data());

    unsigned int foundQueueFamilyIndex = -1;

    // Checking if there is the interested queue family with requested flags and with surface support
    auto foundQueueFamily =
            std::ranges::find_if(deviceQueueFamilies,
                                 [&foundQueueFamilyIndex, &presentQueueFamilyIndex, &device, &surface]
                                         (const VkQueueFamilyProperties& queueFamilyProperties) {
                                     std::function<bool(const VkQueueFlagBits&)> flagsContainsFunction(
                                             [&queueFamilyProperties](const VkQueueFlagBits& flagBits) {
                                                 return queueFamilyProperties.queueFlags & flagBits;
                                             });
                                     // Keeping track of queue family index
                                     ++foundQueueFamilyIndex;

                                     bool areAllBitsSupported =
                                             std::ranges::all_of(Settings::defaultFlags, flagsContainsFunction) &&
                                             std::ranges::all_of(Settings::preferredFlagBits, flagsContainsFunction);

                                     VkBool32 isSurfaceSupported = false;
                                     vkGetPhysicalDeviceSurfaceSupportKHR(device, foundQueueFamilyIndex, surface,
                                                                          &isSurfaceSupported);

                                     if (isSurfaceSupported) {
                                         presentQueueFamilyIndex = foundQueueFamilyIndex;
                                     }

                                     return areAllBitsSupported && isSurfaceSupported;
                                 });

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

    unsigned int formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        surfaceAvailableFormats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                             surfaceAvailableFormats.data());
    }

    unsigned int presentModeCount;
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
    if (Settings::engineStatus == VIEStatus::SETTINGS_LOADED) {
        // GLFW initialisation
        if (!glfwInit()) {
            throw VIERunException("GLFW not initialised...", Settings::engineStatus);
        }

        // Hinting GLFW to not load APIs (Vulkan APIs not defined in GLFW)
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        // Initialising window
        mNativeWindow.mainWindow = glfwCreateWindow(static_cast<int>(Settings::xRes), static_cast<int>(Settings::yRes),
                                                    Settings::engineProgramName.c_str(), nullptr, nullptr);

        if (!mNativeWindow.mainWindow) {
            throw VIERunException("GLFW window not initialised...", Settings::engineStatus);
        }

        // Getting extensions from GLFW for Vulkan implementation
        mNativeWindow.glfwExtensions = glfwGetRequiredInstanceExtensions(&mNativeWindow.glfwExtensionCount);

        Settings::engineStatus = VIEStatus::GLFW_LOADED;
    } else {
        throw VIERunException("Engine not in SETTINGS_LOADED status.", Settings::engineStatus);
    }
}

void VIEngine::initialiseVulkanLibraries() {
    if (Settings::engineStatus == VIEStatus::GLFW_LOADED) {
        // Creating an instance for GPU drivers and application connection
        // 1) Application info
        mVulkanLibraries.applicationInfo.pApplicationName = Settings::engineName.c_str();
        mVulkanLibraries.applicationInfo.pEngineName = Settings::engineName.c_str();
        mVulkanLibraries.applicationInfo.applicationVersion = VK_MAKE_VERSION(Settings::engineMajorVersion,
                                                                              Settings::engineMinorVersion,
                                                                              Settings::enginePatchVersion);
        mVulkanLibraries.applicationInfo.engineVersion = VK_MAKE_VERSION(Settings::engineMajorVersion,
                                                                         Settings::engineMinorVersion,
                                                                         Settings::enginePatchVersion);
        mVulkanLibraries.applicationInfo.apiVersion = VK_API_VERSION_1_2;

        // 2) Instance info for extensions to integrate global extensions and validation layers
        // TODO implement additional debug
        mVulkanLibraries.engineCreationInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        mVulkanLibraries.engineCreationInfo.pApplicationInfo = &mVulkanLibraries.applicationInfo;
        mVulkanLibraries.engineCreationInfo.enabledExtensionCount = mNativeWindow.glfwExtensionCount;
        mVulkanLibraries.engineCreationInfo.ppEnabledExtensionNames = mNativeWindow.glfwExtensions;

        // 2.1) Adding validation layers
        if (!Settings::validationLayers.empty()) {
            // Getting number of available layers
            unsigned int availableInSystemLayersNum = 0;
            vkEnumerateInstanceLayerProperties(&availableInSystemLayersNum, nullptr);

            // Getting all layers, now known the number of layers
            std::vector<VkLayerProperties> availableInSystemLayers(availableInSystemLayersNum);
            vkEnumerateInstanceLayerProperties(&availableInSystemLayersNum, availableInSystemLayers.data());

            std::erase_if(Settings::validationLayers, [&availableInSystemLayers](const char*& requestedLayer){
                std::string requestedLayerStr(requestedLayer);
                auto foundLayer = std::ranges::find_if(availableInSystemLayers,
                                                       [&requestedLayerStr](const VkLayerProperties& properties) {
                                                           return requestedLayerStr == properties.layerName;
                                                       });

                return foundLayer == availableInSystemLayers.end();
            });

            if (Settings::validationLayers.empty()) {
                mVulkanLibraries.engineCreationInfo.enabledLayerCount = 0;
            } else {
                mVulkanLibraries.engineCreationInfo.enabledLayerCount = static_cast<unsigned int>(Settings::validationLayers.size());
                mVulkanLibraries.engineCreationInfo.ppEnabledLayerNames = Settings::validationLayers.data();
            }
        } else {
            // No validation layers imported
            mVulkanLibraries.engineCreationInfo.enabledLayerCount = 0;
        }

        if (vkCreateInstance(&mVulkanLibraries.engineCreationInfo, nullptr, &mVulkanLibraries.mainInstance) != VK_SUCCESS) {
            throw VIERunException("Vulkan instance not created...", Settings::engineStatus);
        }

        Settings::engineStatus = VIEStatus::VULKAN_INSTANCE_CREATED;
    } else {
        throw VIERunException("Engine not in GLFW_LOADED status.", Settings::engineStatus);
    }
}

void VIEngine::prepareMainPhysicalDevices() {
    if (Settings::engineStatus == VIEStatus::VULKAN_SURFACE_CREATED) {
        // Looking for devices
        unsigned int devicesCount = 0;
        vkEnumeratePhysicalDevices(mVulkanLibraries.mainInstance, &devicesCount, nullptr);

        // Gathering devices info
        if (devicesCount == 0) {
            throw VIERunException("No physical devices found for Vulkan rendering...", Settings::engineStatus);
        }

        std::vector<VkPhysicalDevice> availableDevices(devicesCount);
        vkEnumeratePhysicalDevices(mVulkanLibraries.mainInstance, &devicesCount, availableDevices.data());

        // Boolean for checking if a device has been found
        bool areDevicesSet = false;

        // Selecting and sorting devices
        // For one device, the selection is at the beginning of availableDevices vector
        for (VkPhysicalDevice& device : availableDevices) {
            if (Settings::checkPreferredGPUProperties && Settings::preferredDeviceSelectionFunction &&
                !Settings::preferredDeviceSelectionFunction(device)) {
                continue;
            }

            std::optional<unsigned int> mainDeviceSelectedQueueFamily;
            std::optional<unsigned int> mainDeviceSelectedPresentFamily;
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
                mPhysicalDevice.mainPhysicalDevice = device;
                mPhysicalDevice.mainDeviceSelectedQueueFamily = mainDeviceSelectedQueueFamily;
                mPhysicalDevice.mainDeviceSelectedPresentFamily = mainDeviceSelectedPresentFamily;
                mPhysicalDevice.surfaceAvailableFormats = std::move(surfaceAvailableFormats);
                mPhysicalDevice.surfacePresentationModes = std::move(surfacePresentationModes);

                areDevicesSet = true;
            }
        }

        if (areDevicesSet) {
            Settings::engineStatus = VIEStatus::VULKAN_PHYSICAL_DEVICES_PREPARED;
        } else {
            throw VIERunException("Error looking for physical device...", Settings::engineStatus);
        }
    } else {
        throw VIERunException("Engine not in VULKAN_SURFACE_CREATED status.", Settings::engineStatus);
    }
}

// TODO make function handle all devices in different moments (via references)
void VIEngine::createLogicDevice() {
    if (Settings::engineStatus == VIEStatus::VULKAN_PHYSICAL_DEVICES_PREPARED) {
        // TODO check if the additional "presentQueue" is necessary for later use
        // Preparing command queue family for the main device
        mLogicDevice.mainDeviceQueueCreationInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        mLogicDevice.mainDeviceQueueCreationInfo.queueFamilyIndex = mPhysicalDevice.mainDeviceSelectedQueueFamily.value();
        mLogicDevice.mainDeviceQueueCreationInfo.queueCount = 1;

        // Setting queue priority (array)
        mLogicDevice.mainDeviceQueueCreationInfo.pQueuePriorities = &mainQueueFamilyPriority;

        // Preparing logical device creation procedures
        mLogicDevice.mainDeviceCreationInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        mLogicDevice.mainDeviceCreationInfo.pQueueCreateInfos = &mLogicDevice.mainDeviceQueueCreationInfo;
        mLogicDevice.mainDeviceCreationInfo.queueCreateInfoCount = 1;

        // Setting device features
        mLogicDevice.mainDeviceCreationInfo.pEnabledFeatures = &mPhysicalDevice.mainPhysicalDeviceFeatures;

        // Setting extensions to logic device
        mLogicDevice.mainDeviceCreationInfo.enabledExtensionCount = static_cast<unsigned int>(Settings::deviceExtensions.size());
        mLogicDevice.mainDeviceCreationInfo.ppEnabledExtensionNames = Settings::deviceExtensions.data();

        // Setting validation layers to logic device
        if (Settings::validationLayers.empty()) {
            mLogicDevice.mainDeviceCreationInfo.enabledLayerCount = 0;
        } else {
            mLogicDevice.mainDeviceCreationInfo.enabledLayerCount = static_cast<unsigned int>(Settings::validationLayers.size());
            mLogicDevice.mainDeviceCreationInfo.ppEnabledLayerNames = Settings::validationLayers.data();
        }

        if (vkCreateDevice(mPhysicalDevice.mainPhysicalDevice, &mLogicDevice.mainDeviceCreationInfo, nullptr,
                           &mLogicDevice.mainDevice) != VK_SUCCESS) {
            throw VIERunException("Vulkan logic device not created...", Settings::engineStatus);
        }

        // Obtaining graphics queue family from logic device via stored index
        vkGetDeviceQueue(mLogicDevice.mainDevice, mPhysicalDevice.mainDeviceSelectedQueueFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(mLogicDevice.mainDevice, mPhysicalDevice.mainDeviceSelectedPresentFamily.value(), 0, &presentQueue);

        Settings::engineStatus = VIEStatus::VULKAN_DEVICE_CREATED;
    } else {
        throw VIERunException("Engine not in VULKAN_PHYSICAL_DEVICES_PREPARED status.", Settings::engineStatus);
    }
}

void VIEngine::prepareWindowSurface() {
    if (Settings::engineStatus >= VIEStatus::VULKAN_INSTANCE_CREATED) {
#if _WIN64
        // Creating Vulkan surface based on WindowsNT native bindings
        VkWin32SurfaceCreateInfoKHR ntWindowSurfaceCreationInfo{};
        ntWindowSurfaceCreationInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        // Requesting a "Handle to a window" native Windows object
        ntWindowSurfaceCreationInfo.hwnd = glfwGetWin32Window(mainWindow);
        // Providing a native NT instance
        ntWindowSurfaceCreationInfo.hinstance = GetModuleHandle(nullptr);

        if (VkResult result = vkCreateWin32SurfaceKHR(mainInstance, &ntWindowSurfaceCreationInfo, nullptr, &surface);
                result != VK_SUCCESS) {
            throw VIERunException("Cannot create native NT window surface to bind to Vulkan...", Settings::engineStatus);
        }
#elif __linux__
        // Creating Vulkan surface based on Wayland native bindings
        // TODO see if in Windows this creation type is compatible
        if (glfwCreateWindowSurface(mVulkanLibraries.mainInstance, mNativeWindow.mainWindow, nullptr, &mSurface.surface)) {
            throw VIERunException("Error creating surface in Wayland environment...", Settings::engineStatus);
        }
#endif

        Settings::engineStatus = VIEStatus::VULKAN_SURFACE_CREATED;
    } else {
        throw VIERunException("Engine not in VULKAN_INSTANCE_CREATED status.", Settings::engineStatus);
    }
}

void VIEngine::prepareSwapChain() {
    if (Settings::engineStatus == VIEStatus::VULKAN_DEVICE_CREATED) {
        // Selecting surface format (channels and color space)
        auto foundSurfaceFormat =
                std::ranges::find_if(mPhysicalDevice.surfaceAvailableFormats,
                                     [](const VkSurfaceFormatKHR& surfaceFormat) {
                                         return (std::ranges::find(Settings::defaultFormats, surfaceFormat.format) !=
                                                 Settings::defaultFormats.end()) &&
                                                surfaceFormat.colorSpace == Settings::defaultColorSpace;
                                     });


        mSurface.chosenSurfaceFormat = (foundSurfaceFormat != mPhysicalDevice.surfaceAvailableFormats.end()) ?
                                       *foundSurfaceFormat : mPhysicalDevice.surfaceAvailableFormats.at(0);

        // Selecting presentation mode by a swap chain (describing how and when images are represented on screen)
        auto foundSurfacePresentation = std::ranges::find(mPhysicalDevice.surfacePresentationModes, Settings::refreshMode);
        mSurface.chosenSurfacePresentationMode = (foundSurfacePresentation != mPhysicalDevice.surfacePresentationModes.end()) ?
                                                 *foundSurfacePresentation : VK_PRESENT_MODE_FIFO_KHR;

        // Selecting swap extent (resolution of the swap chain images in pixels)
        if (mSurface.surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            mSwapChain.chosenSwapExtent = mSurface.surfaceCapabilities.currentExtent;
        } else {
            int width;
            int height;
            glfwGetFramebufferSize(mNativeWindow.mainWindow, &width, &height);

            mSwapChain.chosenSwapExtent.width = std::clamp(static_cast<uint32_t>(width),
                                                           mSurface.surfaceCapabilities.minImageExtent.width,
                                                           mSurface.surfaceCapabilities.maxImageExtent.width);
            mSwapChain.chosenSwapExtent.height = std::clamp(static_cast<uint32_t>(height),
                                                            mSurface.surfaceCapabilities.minImageExtent.height,
                                                            mSurface.surfaceCapabilities.maxImageExtent.height);
        }

        // Setting the number of images that the swap chain needs to create, depending on a necessary minimum and maximum
        unsigned int swapChainImagesCount = std::min(mSurface.surfaceCapabilities.minImageCount + 1,
                                                     mSurface.surfaceCapabilities.maxImageCount);

        // Settings how swap chain will be created
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
        std::array<unsigned int, 2> queueIndices({mPhysicalDevice.mainDeviceSelectedQueueFamily.value(),
                                                  mPhysicalDevice.mainDeviceSelectedPresentFamily.value()});

        if (mPhysicalDevice.mainDeviceSelectedQueueFamily != mPhysicalDevice.mainDeviceSelectedPresentFamily) {
            mSwapChain.swapChainCreationInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            mSwapChain.swapChainCreationInfo.queueFamilyIndexCount = 2;
            mSwapChain.swapChainCreationInfo.pQueueFamilyIndices = queueIndices.data();
        } else {
            mSwapChain.swapChainCreationInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        // Frame transform on surface swap chain
        mSwapChain.swapChainCreationInfo.preTransform = mSurface.surfaceCapabilities.currentTransform;
        mSwapChain.swapChainCreationInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        mSwapChain.swapChainCreationInfo.presentMode = mSurface.chosenSurfacePresentationMode;
        mSwapChain.swapChainCreationInfo.clipped = VK_TRUE;

        mSwapChain.swapChainCreationInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(mLogicDevice.mainDevice, &mSwapChain.swapChainCreationInfo, nullptr,
                                 &mSwapChain.mainSwapChain)) {
            throw VIERunException("Cannot create swap chain for main device!", Settings::engineStatus);
        }

        Settings::engineStatus = VIEStatus::VULKAN_SWAP_CHAIN_CREATED;

        // TODO https://vulkan-tutorial.com/en/Drawing_a_triangle/Presentation/Swap_chain get swap chain images?
        // TODO https://vulkan-tutorial.com/en/Drawing_a_triangle/Presentation/Image_views get textures for other parts? example shadow mapping?
    } else {
        throw VIERunException("Engine not in VULKAN_PHYSICAL_DEVICES_PREPARED status.", Settings::engineStatus);
    }
}

void VIEngine::cleanEngine() {
    if (Settings::engineStatus >= VIEStatus::GLFW_LOADED) {
        glfwDestroyWindow(mNativeWindow.mainWindow);
        glfwTerminate();

        if (Settings::engineStatus >= VIEStatus::VULKAN_INSTANCE_CREATED) {
            if (Settings::engineStatus >= VIEStatus::VULKAN_SWAP_CHAIN_CREATED) {
                vkDestroySwapchainKHR(mLogicDevice.mainDevice, mSwapChain.mainSwapChain, nullptr);
            }

            vkDestroySurfaceKHR(mVulkanLibraries.mainInstance, mSurface.surface, nullptr);
            vkDestroyDevice(mLogicDevice.mainDevice, nullptr);
            vkDestroyInstance(mVulkanLibraries.mainInstance, nullptr);
        }
    }
}

void VIEngine::prepareEngine() {
    Settings::defaultFlags.emplace_back(VK_QUEUE_GRAPHICS_BIT);

    Settings::defaultFormats.emplace_back(VK_FORMAT_R8_SRGB);
    Settings::defaultFormats.emplace_back(VK_FORMAT_R8G8B8A8_SRGB);

    Settings::defaultColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    Settings::deviceExtensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    if (Settings::checkPreferredGPUProperties && !Settings::preferredDeviceSelectionFunction) {
        std::cout << "\"checkPreferredGPUProperties\" is flagged and no preferred GPU selection function has been"
                     " defined! Skipping check...";
    }

    try {
        initialiseGLFW();
        initialiseVulkanLibraries();
        prepareWindowSurface();
        prepareMainPhysicalDevices();
        createLogicDevice();
        prepareSwapChain();
    } catch (const VIERunException& e) {
        std::cout << fmt::format("VIERunException::what(): {}\nCleaning and closing engine.\n", e.what());
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }


    // TODO import models (using lambdas to describe how to convert personal vector structure to internal drawing struct)
    //  (VIEModule)

    // TODO set a runEngine function (with lambdas for describing some parts (the order of drawing, how to do it, with
    //  a list of functions???)

    // TODO move cleanEngine to a program closing callback
    cleanEngine();
}
