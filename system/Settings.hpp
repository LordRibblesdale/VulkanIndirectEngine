/* Created by LordRibblesdale on 19/10/2021.
 * MIT License
 */

#pragma once

#include <string>

#define FMT_HEADER_ONLY
#include <fmt/format.h>

#include "../engine/VIEngineStatus.hpp"

struct Settings {
    inline static std::string engineName;
    inline static std::string engineProgramName;
    inline static unsigned int engineMajorVersion;
    inline static unsigned int engineMinorVersion;
    inline static unsigned int enginePatchVersion;

    inline static unsigned int xRes;
    inline static unsigned int yRes;

    inline static VIEngineStatus engineStatus;
    inline static std::vector<const char*> validationLayers;
};