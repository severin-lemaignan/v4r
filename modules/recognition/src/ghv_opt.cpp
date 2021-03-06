/*
 * Software License Agreement (BSD License)
 *
 *  Point Cloud Library (PCL) - www.pointclouds.org
 *  Copyright (c) 2010-2011, Willow Garage, Inc.
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <pcl/point_types.h>
#include <pcl/impl/instantiate.hpp>
#include "v4r/recognition/ghv.h"
#include "v4r/recognition/impl/ghv_opt.hpp"

//PCL_INSTANTIATE_PRODUCT(faatGoHV, ((pcl::PointXYZ))((pcl::PointXYZ)))
//PCL_INSTANTIATE_PRODUCT(faatGoHV, ((pcl::PointXYZRGB))((pcl::PointXYZRGB)))
//PCL_INSTANTIATE_PRODUCT(faatGoHV, ((pcl::PointXYZRGBA))((pcl::PointXYZRGBA)))

//PCL_INSTANTIATE_PRODUCT(faatGoHV, ((pcl::PointXYZ))((pcl::PointXYZ))) template class v4r::GlobalHypothesesVerification<pcl::PointXYZ,pcl::PointXYZ>;
//template class FAAT_REC_API v4r::HVGOBinaryOptimizer<pcl::PointXYZ,pcl::PointXYZ>;
//template class FAAT_REC_API v4r::HVGOBinaryOptimizer<pcl::PointXYZRGB,pcl::PointXYZRGB>;

template class V4R_EXPORTS v4r::GHVmove_manager<pcl::PointXYZ,pcl::PointXYZ>;
template class V4R_EXPORTS v4r::GHVmove_manager<pcl::PointXYZRGB,pcl::PointXYZRGB>;
//template class FAAT_REC_API v4r::GlobalHypothesesVerification<pcl::PointXYZRGBA,pcl::PointXYZRGBA>;
