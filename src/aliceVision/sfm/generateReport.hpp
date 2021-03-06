// This file is part of the AliceVision project.
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <aliceVision/sfm/sfmDataIO.hpp>

namespace aliceVision {
namespace sfm {

bool Generate_SfM_Report(const SfMData & sfm_data, const std::string & htmlFilename);


} // namespace sfm
} // namespace aliceVision
