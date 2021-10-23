/* Created by LordRibblesdale on 16/10/2021.
 * MIT License
 */

#include <iostream>
#include "VIEngine.hpp"
#include "../system/Settings.hpp"

void VIEngine::initialiseGLFW() {
    // Requiring engine to have at least loaded settings
    if (Settings::engineStatus == VIEngineStatus::SETTINGS_LOADED) {
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
    }
}

void VIEngine::initialiseVulkanLibraries() {
    if (Settings::engineStatus == GLFW_LOADED) {
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
                auto item = std::find_if(availableInSystemLayers.begin(), availableInSystemLayers.end(),
                                         [&requestedLayer](VkLayerProperties& properties) {
                                             return properties.layerName == requestedLayer;
                                         });

                return item == availableInSystemLayers.end();
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
    }
}

template<typename ... VkQueueFlagBits>
bool VIEngine::isDeviceCompliantToQueueFamilies(const VkPhysicalDevice &device,
                                                std::optional<unsigned int>& queueFamilyIndex,
                                                VkQueueFlagBits&&... flags) {
    unsigned int queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> deviceQueueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, deviceQueueFamilies.data());

    std::vector<::VkQueueFlagBits> flagList(flags...);
    unsigned int foundQueueFamilyIndex = -1;
    auto item = std::find_if(deviceQueueFamilies.begin(), deviceQueueFamilies.end(),
                             [&flagList, &foundQueueFamilyIndex](VkQueueFamilyProperties& queueFamilyProperties) {
                                 ++foundQueueFamilyIndex;
                                 return std::ranges::all_of(flagList.begin(), flagList.end(),
                                                            [&queueFamilyProperties](::VkQueueFlagBits& flagBits) {
                                                                return queueFamilyProperties.queueFlags & flagBits;
                                                            });
                             });

    if (foundQueueFamilyIndex != queueFamilyCount) {
        queueFamilyIndex = foundQueueFamilyIndex;
    }
    return item != deviceQueueFamilies.end();
}

void VIEngine::preparePhysicalDevices() {
    if (Settings::engineStatus == VIEngineStatus::VULKAN_INSTANCE_CREATED) {
        // Looking for devices
        unsigned int devicesCount = 0;
        vkEnumeratePhysicalDevices(mainInstance, &devicesCount, nullptr);

        // Gathering devices info
        if (devicesCount == 0) {
            throw std::runtime_error("No physical devices found for Vulkan rendering...");
        }

        std::vector<VkPhysicalDevice> availableDevices;
        vkEnumeratePhysicalDevices(mainInstance, &devicesCount, availableDevices.data());

        // Selecting and sorting devices
        bool areDevicesSet = false;

        if (devicesCount == 1) {
            if (isDeviceCompliantToQueueFamilies(availableDevices.at(0), mainDeviceSelectedQueueFamily)) {
                mainPhysicalDevice = availableDevices.at(0);
                areDevicesSet = true;
            } else {
                areDevicesSet = false;
            }
        } else if (devicesCount > 1) {
            // If more than one, we will look for the optimal one as first device, if requested
            while (!areDevicesSet && !availableDevices.empty()) {
                std::vector<VkPhysicalDevice>::iterator item;
                if (Settings::checkPreferredGPUProperties) {
                    item = std::find_if(availableDevices.begin(), availableDevices.end(),
                                        [](VkPhysicalDevice& device) {
                                            VkPhysicalDeviceProperties deviceProperties;
                                            vkGetPhysicalDeviceProperties(device, &deviceProperties);

                                            return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
                                        });
                } else {
                    item = availableDevices.begin();
                }

                if (isDeviceCompliantToQueueFamilies(*item, mainDeviceSelectedQueueFamily)) {
                    mainPhysicalDevice = *item;
                    areDevicesSet = true;
                }

                availableDevices.erase(item);
            }
        }

        if (areDevicesSet) {
            Settings::engineStatus = VIEngineStatus::VULKAN_PHYSICAL_DEVICES_PREPARED;
        } else {
            throw std::runtime_error("Error gathering devices into temporary array...");
        }
    }
}

void VIEngine::createLogicDevice() {

}

void VIEngine::cleanEngine() {
    if (Settings::engineStatus >= VIEngineStatus::GLFW_LOADED) {
        glfwDestroyWindow(mainWindow);
        glfwTerminate();

        if (Settings::engineStatus >= VIEngineStatus::VULKAN_INSTANCE_CREATED) {
            vkDestroyInstance(mainInstance, nullptr);
        }
    }
}

void VIEngine::runEngine() {
    initialiseGLFW();
    initialiseVulkanLibraries();
    preparePhysicalDevices();

    cleanEngine();
}
