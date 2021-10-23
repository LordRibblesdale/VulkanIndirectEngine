/* Created by LordRibblesdale on 01/08/2021.
 * MIT License
 */

#pragma once

#include <string>
#include "../system/Settings.hpp"

class XMLHandler {
public:
    static void loadXMLSettings(const std::string& path, Settings& settings);
};
