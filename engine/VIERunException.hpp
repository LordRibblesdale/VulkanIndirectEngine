/* Created by LordRibblesdale on 11/14/21.
 * MIT License
 */

#pragma once

#include "VIEStatus.hpp"

class VIERunException {
private:
    std::string message;
    VIEStatus engineStatus;

public:
    VIERunException(std::string_view message, VIEStatus status) : message(message), engineStatus(status) {}

    std::string what() const {
        return message + " " + fromVIEStatusToString(engineStatus);
    }
};