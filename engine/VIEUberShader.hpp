/* Created by LordRibblesdale on 11/20/21.
 * MIT License
 */

#pragma once

#include <shaderc/shaderc.hpp>
#include <fstream>

#include "VIEStatus.hpp"
#include "VIERunException.hpp"

class VIEUberShader {
    VIEStatus* currentStatus;
    std::vector<uint32_t> currentVertexShader;
    std::vector<uint32_t> currentFragmentShader;

    void compileSPIRVVertexShader(const std::string& shaderModule) {
        shaderc::Compiler compiler{};

        if (compiler.IsValid()) {
            shaderc::CompileOptions options{};
            shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(shaderModule,
                                                                             shaderc_glsl_vertex_shader,
                                                                             "vs", options);

            if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
                throw VIERunException("Error compiling vertex shader.", *currentStatus);
            }

            currentVertexShader.assign(result.cbegin(), result.cend());
        } else {
            throw VIERunException("Error creating Google shaderc (not valid).", *currentStatus);
        }
    }

    void compileSPIRVFragmentShader(const std::string& shaderModule) {
        shaderc::Compiler compiler{};

        if (compiler.IsValid()) {
            shaderc::CompileOptions options{};
            shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(shaderModule,
                                                                             shaderc_glsl_fragment_shader,
                                                                             "fs", options);

            if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
                throw VIERunException("Error compiling fragment shader.", *currentStatus);
            }

            currentFragmentShader.assign(result.cbegin(), result.cend());
        } else {
            throw VIERunException("Error creating Google shaderc (not valid).", *currentStatus);
        }
    }

    VkShaderModule createShaderModuleFromSPIRV(VkDevice& logicDevice, const std::vector<uint32_t>& spirvCode) const {
        VkShaderModuleCreateInfo vkShaderModuleCreateInfo{};
        vkShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vkShaderModuleCreateInfo.codeSize = spirvCode.size();
        vkShaderModuleCreateInfo.pCode = spirvCode.data();

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(logicDevice, &vkShaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw VIERunException("Error creating VkShaderModule from SPIR-V code", *currentStatus);
        }

        return shaderModule;
    }

public:
    VIEUberShader() = delete;
    VIEUberShader(VIEStatus* currentStatus, const std::string& vertexShaderLocation,
                  const std::string& fragmentShaderLocation) : currentStatus(currentStatus) {
        std::ifstream vertexShaderFile(vertexShaderLocation);
        std::ifstream fragmentShaderFile(fragmentShaderLocation);

        if (!vertexShaderFile.is_open()) {
            throw VIERunException(fmt::format("Error: cannot open vertex shader file {}", vertexShaderLocation),
                                  *currentStatus);
        }

        if (!fragmentShaderFile.is_open()) {
            throw VIERunException(fmt::format("Error: cannot open fragment shader file {}", fragmentShaderLocation),
                                  *currentStatus);
        }

        std::string vertexShader;
        std::string fragmentShader;

        for (std::string line; std::getline(vertexShaderFile, line); ) {
            vertexShader.append(line).append("\n");
        }

        for (std::string line; std::getline(fragmentShaderFile, line); ) {
            fragmentShader.append(line).append("\n");
        }

        compileSPIRVVertexShader(vertexShader);
        compileSPIRVFragmentShader(fragmentShader);
    }
    VIEUberShader(const VIEUberShader&) = delete;
    VIEUberShader(VIEUberShader&&) = default;
    ~VIEUberShader() = default;

    VkShaderModule createVertexModuleFromSPIRV(VkDevice& logicDevice) const {
        return createShaderModuleFromSPIRV(logicDevice, currentVertexShader);
    }

    VkShaderModule createFragmentModuleFromSPIRV(VkDevice& logicDevice) const {
        return createShaderModuleFromSPIRV(logicDevice, currentFragmentShader);
    }
};