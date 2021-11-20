/* Created by LordRibblesdale on 01/08/2021.
 * MIT License
 */

#pragma once

#include <string>
#include "../../engine/VIESettings.hpp"

class XMLHandler {
public:
    static void loadXMLSettings(const std::string& path, VIESettings& settings);
};
