/* Created by LordRibblesdale on 20/02/2022.
 * MIT License
 */

#pragma once

#include "structs/transform/VIETranslation.hpp"
#include "structs/transform/VIERotation.hpp"
#include "structs/transform/VIEScale.hpp"

struct VIELocalTransform {
    VIETranslation localTranslation;
    VIEScale localScale;
    VIERotation localRotation;
};

struct VIEGlobalTransform {
    VIETranslation globalTranslation;
    VIEScale globalScale;
    VIERotation globalRotation;
};