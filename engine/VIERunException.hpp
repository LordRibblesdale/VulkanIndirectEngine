/* Created by LordRibblesdale on 11/14/21.
 * MIT License
 */

#pragma once

#include "VIEStatus.hpp"

class VIERunException : public std::exception {
private:
    std::string message;
    VIEStatus engineStatus;

public:
    VIERunException(std::string_view message, VIEStatus status) : message(message), engineStatus(status) {}

    std::string errorMessage() const {
        return message + " " + fromVIEStatusToString(engineStatus);
    }
};