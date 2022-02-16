/* Created by LordRibblesdale on 15/02/2022.
 * MIT License
 */

#include "engine/VIESettings.hpp"

#include <fstream>
#include <filesystem>
#include <pugixml.hpp>

void VIESettings::setDefaultValues() {
    applicationName = kDefaultName;
    applicationVersion = kDefaultVersion;

    startingXRes = kDefaultXRes;
    startingYRes = kDefaultYRes;
}

VIESettings::VIESettings(const std::string &configLocation) {
    std::ifstream file(configLocation);

    if (!file.is_open()) {
        std::cout << "<ERROR> loadXMLSettings: file not opened." << std::endl;
        setDefaultValues();
        return;
    }

    pugi::xml_document xmlDocument;

    if (pugi::xml_parse_result result(xmlDocument.load_file(configLocation.c_str())); !result) {
        std::cout << "<ERROR> result loadXMLSettings: XML file not loaded.";
        setDefaultValues();
        return;
    }

    pugi::xml_node root(xmlDocument.child("Settings"));

    pugi::xml_node current(root.child("Program"));
    applicationName = current.attribute("name").value();
    applicationVersion = VK_MAKE_API_VERSION(0,
                                             current.attribute("majorVersion").as_uint(),
                                             current.attribute("minorVersion").as_uint(),
                                             current.attribute("patchVersion").as_uint());

    current = root.child("Resolution");
    startingXRes = current.attribute("width").as_uint();
    startingYRes = current.attribute("height").as_uint();

    current = root.child("Locale");
    try {
        languageResource = std::make_unique<LanguageResource>(current.attribute("directory").value(),
                                                              current.attribute("language").value(),
                                                              current.attribute("country").value());
    } catch (const LangException &exception) {
        languageResource = std::make_unique<LanguageResource>("languages", "en", "US");
    }

    current = root.child("Framerate");
    if (double frameRate = current.attribute("limit").as_double(); frameRate != 0) {
        frameTime = 1. / frameRate;
    }

    if (std::string syncType(current.attribute("syncType").value()); syncType != "none") {
        if (syncType == "vsync") {
            preferredPresentMode = VK_PRESENT_MODE_FIFO_KHR;
        } else if (syncType == "relaxed_vsync") {
            preferredPresentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
        } else if (syncType == "triple_buffering") {
            preferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }

    current = root.child("Requirements");
    if (std::string gpuType(current.attribute("gpuType").value()); gpuType == "integrate") {
        selectedDeviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
    } else if (gpuType == "virtual") {
        selectedDeviceType = VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU;
    }

    isPreferableDevice = [this](const VkPhysicalDevice& device) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        return deviceProperties.deviceType == selectedDeviceType && deviceFeatures.multiDrawIndirect &&
               deviceFeatures.multiViewport;
    };

    current = root.child("Shaders");
    std::filesystem::path directory(current.attribute("directory").value());
    vertexShaderLocation = (directory / current.attribute("vertex").value()).string();
    fragmentShaderLocation = (directory / current.attribute("fragment").value()).string();

    current = root.child("Debug");
    enableMessageCallback = current.attribute("message").as_bool();

    for (const pugi::xml_node &layer: current.children("Vlayer")) {
        validationLayers.emplace_back(layer.value());
    }

    file.close();
}