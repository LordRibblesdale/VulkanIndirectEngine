/* Created by LordRibblesdale on 16/10/2021.
 * MIT License
 */

#include "VIEngine.hpp"
#include "../system/Settings.hpp"

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

        Settings::engineStatus = GLFW_LOADED;
    } else {
        // TODO handle cases of multiple calls
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
                auto foundLayer = std::find_if(availableInSystemLayers.begin(), availableInSystemLayers.end(),
                                         [&requestedLayerStr](VkLayerProperties& properties) {
                                             return requestedLayerStr == properties.layerName;
                                         });

                return foundLayer == availableInSystemLayers.end();
            });

            if (Settings::validationLayers.empty()) {
                engineCreationInfo.enabledLayerCount = 0;
            } else {
                engineCreationInfo.enabledLayerCount = Settings::validationLayers.size();
                engineCreationInfo.ppEnabledLayerNames = Settings::validationLayers.data();
            }
        } else {
            // No validation layers imported
            engineCreationInfo.enabledLayerCount = 0;
        }

        VkResult creationResult = vkCreateInstance(&engineCreationInfo, nullptr, &mainInstance);
        if (creationResult != VK_SUCCESS) {
            throw std::runtime_error("Vulkan instance not created...");
        }

        Settings::engineStatus = VULKAN_INSTANCE_CREATED;
    } else {
        // TODO handle cases of multiple calls
    }
}

bool VIEngine::checkDeviceExtensionSupport(const VkPhysicalDevice& device) {
    unsigned int extensionCount;
    // Getting number of supported extensions
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    // Getting supported extensions
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    bool areAllExtensionsCompatible =
            std::ranges::all_of(Settings::deviceExtensions.begin(),
                                Settings::deviceExtensions.end(),
                                [&availableExtensions](const char*& extension) {
                                    std::string extensionStr(extension);
                                    auto foundExtension =
                                            std::find_if(availableExtensions.begin(),
                                                         availableExtensions.end(),
                                                         [&extensionStr](VkExtensionProperties& extensionProperties) {
                                                             return extensionStr == extensionProperties.extensionName;
                                                         });

                                    return foundExtension != availableExtensions.end();
                                });

    return areAllExtensionsCompatible;
}

bool VIEngine::checkQueueFamilyCompatibilityWithDevice(const VkPhysicalDevice &device, VkSurfaceKHR &surface,
                                                       std::optional<unsigned int>& queueFamilyIndex) {
    unsigned int queueFamilyCount = 0;
    // Getting number of supported queue families
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> deviceQueueFamilies(queueFamilyCount);
    // Getting supported queue families
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, deviceQueueFamilies.data());

    unsigned int foundQueueFamilyIndex = -1;
    auto foundQueueFamily =
            // Checking if there is the interested queue family with requested flags and with surface support
            std::find_if(deviceQueueFamilies.begin(), deviceQueueFamilies.end(),
                         [&foundQueueFamilyIndex, &device, &surface] (VkQueueFamilyProperties& queueFamilyProperties) {
                             // Keeping track of queue family index
                             ++foundQueueFamilyIndex;
                             
                             bool areAllBitsSupported =
                                     std::ranges::all_of(Settings::preferredFlagBits.begin(),
                                                         Settings::preferredFlagBits.end(),
                                                         [&queueFamilyProperties](VkQueueFlagBits& flagBits) {
                                                             return queueFamilyProperties.queueFlags & flagBits;
                                                         });

                             VkBool32 isSurfaceSupported = false;
                             vkGetPhysicalDeviceSurfaceSupportKHR(device, foundQueueFamilyIndex, surface, &isSurfaceSupported);

                             return areAllBitsSupported && isSurfaceSupported;
                         });

    if (foundQueueFamilyIndex != queueFamilyCount) {
        queueFamilyIndex = foundQueueFamilyIndex;
    }
    return foundQueueFamily != deviceQueueFamilies.end();
}

// TODO set as "prepareMainPhysicalDevice"
void VIEngine::preparePhysicalDevices() {
    if (Settings::engineStatus == VIEStatus::VULKAN_INSTANCE_CREATED) {
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
            if (Settings::checkPreferredGPUProperties && Settings::preferredDeviceSelectionFunction) {
                if (!Settings::preferredDeviceSelectionFunction(device)) {
                    continue;
                }
            }
            
            bool isDeviceCompatibleWithExtensions = checkDeviceExtensionSupport(device);
            
            bool isDeviceCompatibleWithQueueFamily = checkQueueFamilyCompatibilityWithDevice(device, surface,
                                                                                             mainDeviceSelectedQueueFamily);

            if (isDeviceCompatibleWithExtensions && isDeviceCompatibleWithQueueFamily) {
                mainPhysicalDevice = device;
                areDevicesSet = true;
            }
        }

        if (areDevicesSet) {
            Settings::engineStatus = VIEStatus::VULKAN_PHYSICAL_DEVICES_PREPARED;
        } else {
            throw std::runtime_error("Error looking for physical device...");
        }
    } else {
        // TODO handle cases of multiple calls
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
        mainDeviceCreationInfo.enabledExtensionCount = Settings::deviceExtensions.size();
        mainDeviceCreationInfo.ppEnabledExtensionNames = Settings::deviceExtensions.data();

        // Setting validation layers to logic device
        if (Settings::validationLayers.empty()) {
            mainDeviceCreationInfo.enabledLayerCount = 0;
        } else {
            mainDeviceCreationInfo.enabledLayerCount = Settings::validationLayers.size();
            mainDeviceCreationInfo.ppEnabledLayerNames = Settings::validationLayers.data();
        }

        VkResult creationResult = vkCreateDevice(mainPhysicalDevice, &mainDeviceCreationInfo, nullptr, &mainDevice);
        if (creationResult != VK_SUCCESS) {
            throw std::runtime_error("Vulkan logic device not created...");
        }

        // Obtaining graphics queue family from logic device via stored index
        vkGetDeviceQueue(mainDevice, mainDeviceSelectedQueueFamily.value(), 0, &graphicsQueue);

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

        VkResult result = vkCreateWin32SurfaceKHR(mainInstance, &ntWindowSurfaceCreationInfo, nullptr, &surface);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Cannot create native NT window surface to bind to Vulkan...");
        }
#elif __linux__
        VkWaylandSurfaceCreateInfoKHR waylandWindowSurfaceCreationInfo{};
#endif
    } else {
        // TODO handle cases of multiple calls
    }
}

void VIEngine::cleanEngine() {
    if (Settings::engineStatus >= VIEStatus::GLFW_LOADED) {
        glfwDestroyWindow(mainWindow);
        glfwTerminate();

        if (Settings::engineStatus >= VIEStatus::VULKAN_INSTANCE_CREATED) {
            vkDestroySurfaceKHR(mainInstance, surface, nullptr);
            vkDestroyDevice(mainDevice, nullptr);
            vkDestroyInstance(mainInstance, nullptr);
        }
    }
}

void VIEngine::runEngine() {
    if (std::find(Settings::preferredFlagBits.begin(), Settings::preferredFlagBits.end(), VK_QUEUE_GRAPHICS_BIT)
            == Settings::preferredFlagBits.end()) {
        Settings::preferredFlagBits.emplace_back(VK_QUEUE_GRAPHICS_BIT);
    }

    if (Settings::checkPreferredGPUProperties) {
        if (!Settings::preferredDeviceSelectionFunction) {
            std::cout << "\"checkPreferredGPUProperties\" is flagged and no preferred GPU selection function has been"
                         " defined! Skipping check...";
        }
    }

    initialiseGLFW();
    initialiseVulkanLibraries();
    prepareWindowSurface();
    preparePhysicalDevices();
    createLogicDevice();

    cleanEngine();
}
