/* Created by LordRibblesdale on 19/10/2021.
 * MIT License
 */

#pragma once

#include <string>

#include <vector>
#include <functional>
#include <vulkan/vulkan.h>

#define FMT_HEADER_ONLY
#include <fmt/format.h>

#include "VIEStatus.hpp"
#include "LanguageResource.hpp"

/**
 * @brief VIESettings structure for data access around the engine
 */
struct VIESettings {
private:
    void setDefaultValues();

    static const uint16_t kDefaultXRes{1366};
    static const uint16_t kDefaultYRes{768};

    inline static const char* kDefaultName{"VIEProgram"};
    static const uint32_t kDefaultVersion{VK_MAKE_API_VERSION(0, 0, 0, 0)};
public:
    const std::vector<const char*> kDeviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    const VkFormat kDefaultFormat{VK_FORMAT_B8G8R8A8_SRGB};
    const VkColorSpaceKHR kDefaultColorSpace{VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    const std::vector<VkQueueFlagBits> kDefaultQueueFlags{VK_QUEUE_GRAPHICS_BIT};

    const std::string kEngineName{"VulkanIndirectEngine"};
    const uint32_t kEngineVersion{VK_MAKE_API_VERSION(0, 1, 0, 0)};

    const uint8_t kMaxFramesInFlight{2};

    // -----------------------------------------------------------------------------------------------------------------

    std::string applicationName{};
    uint32_t applicationVersion{};

    uint32_t startingXRes{};
    uint32_t startingYRes{};

    double frameTime;

    std::string vertexShaderLocation{};
    std::string fragmentShaderLocation{};

    VkPhysicalDeviceType selectedDeviceType{VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU};
    VkPresentModeKHR preferredPresentMode{VK_PRESENT_MODE_FIFO_KHR};

    bool enableMessageCallback = false;
    bool pauseOnMinimized = false;

    std::vector<const char*> validationLayers;
    std::vector<VkQueueFlagBits> preferredFlagBits;

    std::function<bool(const VkPhysicalDevice &)> isPreferableDevice{};

    std::unique_ptr<LanguageResource> languageResource;

    // -----------------------------------------------------------------------------------------------------------------

    VIESettings() = delete;
    VIESettings(const std::string &configLocation);
    VIESettings(const VIESettings &) = delete;
    VIESettings(VIESettings &&) = default;
    ~VIESettings() = default;
};
