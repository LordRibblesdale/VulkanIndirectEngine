/* Created by LordRibblesdale on 24/10/2021.
 * MIT License
 */

#include "engine/VIESettings.hpp"

void VIESettings::addDeviceExtension(const char *extension) {
    deviceExtensions.push_back(extension);
}

uint32_t VIESettings::getDefaultXRes() const {
    return kDefaultXRes;
}

uint32_t VIESettings::getDefaultYRes() const {
    return kDefaultYRes;
}