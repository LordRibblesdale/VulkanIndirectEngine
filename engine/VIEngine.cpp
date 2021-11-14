/* Created by LordRibblesdale on 16/10/2021.
 * MIT License
 */

#include "VIEngine.hpp"
#include "../system/Settings.hpp"

#define FMT_HEADER_ONLY
#include <fmt/format.h>

void VIEngine::initialiseGLFW() {
    // Requiring engine to have at least loaded settings
    if (Settings::engineStatus == VIEStatus::SETTINGS_LOADED) {
        // GLFW initialisation
        if (!glfwInit()) {
            throw std::runtime_error("GLFW not initialised...");
        }

        // Hinting GLFW to not load APIs (Vulkan APIs not defined in GLFW)
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        // Initialising window
        mainWindow = glfwCreateWindow(static_cast<int>(Settings::xRes), static_cast<int>(Settings::yRes),
                                      Settings::engineProgramName.c_str(), nullptr, nullptr);

        if (!mainWindow) {
            throw std::runtime_error("GLFW window not initialised...");
        }

        // Getting extensions from GLFW for Vulkan implementation
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        Settings::engineStatus = VIEStatus::GLFW_LOADED;
    }
}

void VIEngine::initialiseVulkanLibraries() {
    if (Settings::engineStatus == VIEStatus::GLFW_LOADED) {
        // Creating an instance for GPU drivers and application connection
        // 1) Application info
        applicationInfo.pApplicationName = Settings::engineName.c_str();
        applicationInfo.pEngineName = Settings::engineName.c_str();
        applicationInfo.applicationVersion = VK_MAKE_VERSION(Settings::engineMajorVersion, Settings::engineMinorVersion,
                                                             Settings::enginePatchVersion);
        applicationInfo.engineVersion = VK_MAKE_VERSION(Settings::engineMajorVersion, Settings::engineMinorVersion,
                                                        Settings::enginePatchVersion);
        applicationInfo.apiVersion = VK_API_VERSION_1_2;

        // 2) Instance info for extensions to integrate global extensions and validation layers
        engineCreationInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        // TODO implement additional debug
        engineCreationInfo.pApplicationInfo = &applicationInfo;
        engineCreationInfo.enabledExtensionCount = glfwExtensionCount;
        engineCreationInfo.ppEnabledExtensionNames = glfwExtensions;

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
                engineCreationInfo.enabledLayerCount = 0;
            } else {
                engineCreationInfo.enabledLayerCount = static_cast<unsigned int>(Settings::validationLayers.size());
                engineCreationInfo.ppEnabledLayerNames = Settings::validationLayers.data();
            }
        } else {
            // No validation layers imported
            engineCreationInfo.enabledLayerCount = 0;
        }

        if (VkResult creationResult = vkCreateInstance(&engineCreationInfo, nullptr, &mainInstance);
                creationResult != VK_SUCCESS) {
            throw std::runtime_error("Vulkan instance not created...");
        }

        Settings::engineStatus = VIEStatus::VULKAN_INSTANCE_CREATED;
    }
}

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

// TODO set as "prepareMainPhysicalDevice"
void VIEngine::preparePhysicalDevices() {
    if (Settings::engineStatus == VIEStatus::VULKAN_SURFACE_CREATED) {
        // Looking for devices
        unsigned int devicesCount = 0;
        vkEnumeratePhysicalDevices(mainInstance, &devicesCount, nullptr);

        // Gathering devices info
        if (devicesCount == 0) {
            throw std::runtime_error("No physical devices found for Vulkan rendering...");
        }

        std::vector<VkPhysicalDevice> availableDevices(devicesCount);
        vkEnumeratePhysicalDevices(mainInstance, &devicesCount, availableDevices.data());

        // Boolean for checking if a device has been found
        bool areDevicesSet = false;

        // Selecting and sorting devices
        // For one device, the selection is at the beginning of availableDevices vector
        for (VkPhysicalDevice& device : availableDevices) {
            if (Settings::checkPreferredGPUProperties && Settings::preferredDeviceSelectionFunction &&
                !Settings::preferredDeviceSelectionFunction(device)) {
                continue;
            }

            bool isDeviceCompatibleWithExtensions = checkDeviceExtensionSupport(device);

            bool isDeviceCompatibleWithQueueFamily =
                    checkQueueFamilyCompatibilityWithDevice(device, surface, mainDeviceSelectedQueueFamily,
                                                            mainDeviceSelectedPresentFamily);

            bool isSurfaceSwapChainBasicSupportAvailable =
                    checkSurfaceCapabilitiesFromDevice(device, surface, surfaceCapabilities,
                                                       surfaceAvailableFormats, surfacePresentationModes);

            if (isDeviceCompatibleWithExtensions && isDeviceCompatibleWithQueueFamily
                && isSurfaceSwapChainBasicSupportAvailable) {
                mainPhysicalDevice = device;
                areDevicesSet = true;
            }
        }

        if (areDevicesSet) {
            Settings::engineStatus = VIEStatus::VULKAN_PHYSICAL_DEVICES_PREPARED;
        } else {
            throw std::runtime_error("Error looking for physical device...");
        }
    }
}

// TODO make function handle all devices in different moments (via references)
void VIEngine::createLogicDevice() {
    if (Settings::engineStatus == VIEStatus::VULKAN_PHYSICAL_DEVICES_PREPARED) {
        // TODO check if the additional "presentQueue" is necessary for later use
        // Preparing command queue family for the main device
        mainDeviceQueueCreationInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        mainDeviceQueueCreationInfo.queueFamilyIndex = mainDeviceSelectedQueueFamily.value();
        mainDeviceQueueCreationInfo.queueCount = 1;

        // Setting queue priority (array)
        mainDeviceQueueCreationInfo.pQueuePriorities = &mainQueueFamilyPriority;

        // Preparing logical device creation procedures
        mainDeviceCreationInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        mainDeviceCreationInfo.pQueueCreateInfos = &mainDeviceQueueCreationInfo;
        mainDeviceCreationInfo.queueCreateInfoCount = 1;

        // Setting device features
        mainDeviceCreationInfo.pEnabledFeatures = &mainPhysicalDeviceFeatures;

        // Setting extensions to logic device
        mainDeviceCreationInfo.enabledExtensionCount = static_cast<unsigned int>(Settings::deviceExtensions.size());
        mainDeviceCreationInfo.ppEnabledExtensionNames = Settings::deviceExtensions.data();

        // Setting validation layers to logic device
        if (Settings::validationLayers.empty()) {
            mainDeviceCreationInfo.enabledLayerCount = 0;
        } else {
            mainDeviceCreationInfo.enabledLayerCount = static_cast<unsigned int>(Settings::validationLayers.size());
            mainDeviceCreationInfo.ppEnabledLayerNames = Settings::validationLayers.data();
        }

        if (VkResult creationResult = vkCreateDevice(mainPhysicalDevice, &mainDeviceCreationInfo, nullptr, &mainDevice);
                creationResult != VK_SUCCESS) {
            throw std::runtime_error("Vulkan logic device not created...");
        }

        // Obtaining graphics queue family from logic device via stored index
        vkGetDeviceQueue(mainDevice, mainDeviceSelectedQueueFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(mainDevice, mainDeviceSelectedPresentFamily.value(), 0, &presentQueue);

        Settings::engineStatus = VIEStatus::VULKAN_DEVICE_CREATED;
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
            throw std::runtime_error("Cannot create native NT window surface to bind to Vulkan...");
        }
#elif __linux__
        // Creating Vulkan surface based on Wayland native bindings
        // TODO see if in Windows this creation type is compatible
        if (glfwCreateWindowSurface(mainInstance, mainWindow, nullptr, &surface)) {
            throw std::runtime_error("Error creating surface in Wayland environment...");
        }
#endif

        Settings::engineStatus = VIEStatus::VULKAN_SURFACE_CREATED;
    }
}

void VIEngine::prepareSwapChain() {
    if (Settings::engineStatus >= VIEStatus::VULKAN_PHYSICAL_DEVICES_PREPARED) {
        // Selecting surface format (channels and color space)
        auto foundSurfaceFormat =
                std::ranges::find_if(surfaceAvailableFormats,
                                     [](const VkSurfaceFormatKHR& surfaceFormat) {
                                         return (std::ranges::find(Settings::defaultFormats, surfaceFormat.format) !=
                                                 Settings::defaultFormats.end()) &&
                                                 surfaceFormat.colorSpace == Settings::defaultColorSpace;
                                     });


        chosenSurfaceFormat = (foundSurfaceFormat != surfaceAvailableFormats.end()) ? *foundSurfaceFormat
                                                                                    : surfaceAvailableFormats.at(0);

        // Selecting presentation mode by a swap chain (describing how and when images are represented on screen)
        auto foundSurfacePresentation = std::ranges::find(surfacePresentationModes, Settings::refreshMode);
        chosenSurfacePresentationMode = (foundSurfacePresentation != surfacePresentationModes.end()) ?
                                        *foundSurfacePresentation : VK_PRESENT_MODE_FIFO_KHR;

        // Selecting swap extent (resolution of the swap chain images in pixels)
        if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            chosenSwapExtent = surfaceCapabilities.currentExtent;
        } else {
            int width;
            int height;
            glfwGetFramebufferSize(mainWindow, &width, &height);

            chosenSwapExtent.width = std::clamp(static_cast<uint32_t>(width),
                                                surfaceCapabilities.minImageExtent.width,
                                                surfaceCapabilities.maxImageExtent.width);
            chosenSwapExtent.height = std::clamp(static_cast<uint32_t>(height),
                                                 surfaceCapabilities.minImageExtent.height,
                                                 surfaceCapabilities.maxImageExtent.height);
        }

        // Setting the number of images that the swap chain needs to create, depending on a necessary minimum and maximum
        unsigned int swapChainImagesCount = std::min(surfaceCapabilities.minImageCount + 1,
                                                     surfaceCapabilities.maxImageCount);

        // Settings how swap chain will be created
        swapChainCreationInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapChainCreationInfo.surface = surface;
        swapChainCreationInfo.minImageCount = swapChainImagesCount;
        swapChainCreationInfo.imageFormat = chosenSurfaceFormat.format;
        swapChainCreationInfo.imageColorSpace = chosenSurfaceFormat.colorSpace;
        swapChainCreationInfo.imageExtent = chosenSwapExtent;
        swapChainCreationInfo.imageArrayLayers = 1;
        swapChainCreationInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        // swapChainCreationInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        // Choosing frame handling mode by swap chain
        std::array<unsigned int, 2> queueIndices({mainDeviceSelectedQueueFamily.value(),
                                                  mainDeviceSelectedPresentFamily.value()});

        if (mainDeviceSelectedQueueFamily != mainDeviceSelectedPresentFamily) {
            swapChainCreationInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapChainCreationInfo.queueFamilyIndexCount = 2;
            swapChainCreationInfo.pQueueFamilyIndices = queueIndices.data();
        } else {
            swapChainCreationInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        // Frame transform on surface swap chain
        swapChainCreationInfo.preTransform = surfaceCapabilities.currentTransform;
        swapChainCreationInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        swapChainCreationInfo.presentMode = chosenSurfacePresentationMode;
        swapChainCreationInfo.clipped = VK_TRUE;

        swapChainCreationInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(mainDevice, &swapChainCreationInfo, nullptr, &mainSwapChain)) {
            throw std::runtime_error("Cannot create swap chain for main device!");
        }

        Settings::engineStatus = VIEStatus::VULKAN_SWAP_CHAIN_CREATED;

        // TODO https://vulkan-tutorial.com/en/Drawing_a_triangle/Presentation/Swap_chain get swap chain images?
        // TODO https://vulkan-tutorial.com/en/Drawing_a_triangle/Presentation/Image_views get textures for other parts? example shadow mapping?
    }
}

void VIEngine::cleanEngine() {
    if (Settings::engineStatus >= VIEStatus::GLFW_LOADED) {
        glfwDestroyWindow(mainWindow);
        glfwTerminate();

        if (Settings::engineStatus >= VIEStatus::VULKAN_INSTANCE_CREATED) {
            if (Settings::engineStatus >= VIEStatus::VULKAN_SWAP_CHAIN_CREATED) {
                vkDestroySwapchainKHR(mainDevice, mainSwapChain, nullptr);
            }

            vkDestroySurfaceKHR(mainInstance, surface, nullptr);
            vkDestroyDevice(mainDevice, nullptr);
            vkDestroyInstance(mainInstance, nullptr);
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
        preparePhysicalDevices();
        createLogicDevice();
        prepareSwapChain();
    } catch (std::runtime_error& e) {
        // TODO create "why it crashed due to this error", extending std::runtime_error and gathering engine status
        std::cout << fmt::format("std::runtime_error::what(): {}\nCleaning and closing engine.", e.what());
    }


    // TODO import models (using lambdas to describe how to convert personal vector structure to internal drawing struct)
    //  (VIEModule)

    // TODO set a runEngine function (with lambdas for describing some parts (the order of drawing, how to do it, with
    //  a list of functions???)

    // TODO move cleanEngine to a program closing callback
    cleanEngine();
}
