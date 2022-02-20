/* Created by LordRibblesdale on 16/02/2022.
 * MIT License
 */

#pragma once

#include <list>

#include "structs/VIEMesh.hpp"
#include "structs/transform/VIETransform.hpp"

class VIEModel : public std::list<VIEMesh>, public VIELocalTransform, public VIEGlobalTransform {

};
