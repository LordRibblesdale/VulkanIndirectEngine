/* Created by LordRibblesdale on 11/14/21.
 * MIT License
 */

#pragma once

#include "VIEStatus.hpp"

class VIERunException : public std::exception {
private:
    std::string message;

public:
    explicit VIERunException(std::string&& message, VIEStatus currentStatus)
            : message(fmt::format("{}. Engine status: {}", message, fromVIEStatusToString(currentStatus))) {}
    VIERunException(VIEStatus expectedStatus, VIEStatus currentStatus)
            : message(fmt::format("Engine not in {} status. Engine status: {}.", fromVIEStatusToString(expectedStatus),
                      fromVIEStatusToString(currentStatus))) {}

    const std::string& errorMessage() const {
        return message;
    }
};