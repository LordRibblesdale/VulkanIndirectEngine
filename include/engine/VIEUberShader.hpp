/* Created by LordRibblesdale on 11/20/21.
 * MIT License
 */

#pragma once

#include <shaderc/shaderc.hpp>
#include <fstream>

#include "VIEStatus.hpp"

class VIEUberShader {
    std::vector<uint32_t> currentVertexShader;
    std::vector<uint32_t> currentFragmentShader;

    bool compileSPIRVVertexShader(const std::string &shaderModule) {
        if (shaderc::Compiler compiler{}; compiler.IsValid()) {
            shaderc::CompileOptions options{};
            shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(shaderModule,
                                                                             shaderc_glsl_vertex_shader,
                                                                             "vs", options);

            if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
                std::cout << "Error compiling vertex shader." << std::endl;
                return false;
            }

            currentVertexShader.assign(result.cbegin(), result.cend());
        } else {
            std::cout << "Error creating Google shaderc (not valid)." << std::endl;
            return false;
        }

        return true;
    }

    bool compileSPIRVFragmentShader(const std::string &shaderModule) {
        if (shaderc::Compiler compiler{}; compiler.IsValid()) {
            shaderc::CompileOptions options{};
            shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(shaderModule,
                                                                             shaderc_glsl_fragment_shader,
                                                                             "fs", options);

            if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
                std::cout << "Error compiling fragment shader." << std::endl;
                return false;
            }

            currentFragmentShader.assign(result.cbegin(), result.cend());
        } else {
            std::cout << "Error creating Google shaderc (not valid)." << std::endl;
            return false;
        }

        return true;
    }

    VkShaderModule createShaderModuleFromSPIRV(VkDevice &logicDevice, const std::vector<uint32_t> &spirvCode) const {
        VkShaderModuleCreateInfo vkShaderModuleCreateInfo{};
        vkShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vkShaderModuleCreateInfo.codeSize = spirvCode.size() * sizeof(uint32_t);
        vkShaderModuleCreateInfo.pCode = spirvCode.data();

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(logicDevice, &vkShaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            std::cout << "Error creating VkShaderModule from SPIR-V code" << std::endl;
            return nullptr;
        }

        return shaderModule;
    }

public:
    VIEUberShader() = delete;

    VIEUberShader(const std::string &vertexShaderLocation, const std::string &fragmentShaderLocation) {
        std::ifstream vertexShaderFile(vertexShaderLocation);
        std::ifstream fragmentShaderFile(fragmentShaderLocation);

        if (vertexShaderFile.is_open()) {
            std::string vertexShader;

            for (std::string line; std::getline(vertexShaderFile, line);) {
                vertexShader.append(line).append("\n");
            }

            compileSPIRVVertexShader(vertexShader);
        } else {
            std::cout << "Error: cannot open vertex shader file {}" << std::endl;
        }

        if (fragmentShaderFile.is_open()) {
            std::string fragmentShader;

            for (std::string line; std::getline(fragmentShaderFile, line);) {
                fragmentShader.append(line).append("\n");
            }

            compileSPIRVFragmentShader(fragmentShader);
        } else {
            std::cout << "Error: cannot open fragment shader file {}" << std::endl;
        }
    }

    VIEUberShader(const VIEUberShader &) = delete;

    VIEUberShader(VIEUberShader &&) = default;

    ~VIEUberShader() = default;

    VkShaderModule createVertexModuleFromSPIRV(VkDevice &logicDevice) const {
        return createShaderModuleFromSPIRV(logicDevice, currentVertexShader);
    }

    VkShaderModule createFragmentModuleFromSPIRV(VkDevice &logicDevice) const {
        return createShaderModuleFromSPIRV(logicDevice, currentFragmentShader);
    }
};