#ifndef V4R_OBJECT_MODELLING_MODELVIEW_H__
#define V4R_OBJECT_MODELLING_MODELVIEW_H__

/*
 * Author: Thomas Faeulhammer
 * Date: 20th July 2015
 * License: MIT
 *
 * This class represents a single-view of the object model used for learning
 *
 * */
#include <pcl/common/common.h>
#include <v4r/core/macros.h>
#include <v4r/keypoints/ClusterNormalsToPlanes.h>

namespace v4r
{
    namespace object_modelling
    {
        class V4R_EXPORTS modelView
        {
        public:
            class SuperPlane : public ClusterNormalsToPlanes::Plane
            {
            public:
                std::vector<size_t> visible_indices;
                std::vector<size_t> object_indices;
                std::vector<size_t> within_chop_z_indices;
                bool is_filtered;
                SuperPlane() : ClusterNormalsToPlanes::Plane()
                {
                    is_filtered = false;
                }
            };

        public:
            typedef pcl::PointXYZRGB PointT;
            typedef pcl::Histogram<128> FeatureT;

            pcl::PointCloud<PointT>::Ptr  cloud_;
            pcl::PointCloud<pcl::Normal>::Ptr  normal_;
            pcl::PointCloud<PointT>::Ptr  transferred_cluster_;

            pcl::PointCloud<FeatureT>::Ptr  sift_signatures_;
            pcl::PointCloud<pcl::PointXYZRGBA>::Ptr  supervoxel_cloud_;
            pcl::PointCloud<pcl::PointXYZRGBA>::Ptr  supervoxel_cloud_organized_;

            std::vector<SuperPlane> planes_;

            std::vector< size_t > scene_points_;
            std::vector< size_t > sift_keypoint_indices_;

            std::vector< std::vector <bool> > obj_mask_step_;
            Eigen::Matrix4f camera_pose_;
            Eigen::Matrix4f tracking_pose_;
            bool tracking_pose_set_ = false;
            bool camera_pose_set_ = false;

            size_t id_; // might be redundant

            bool is_pre_labelled_;

            modelView()
            {
                cloud_.reset(new pcl::PointCloud<PointT>());
                normal_.reset(new pcl::PointCloud<pcl::Normal>());
                transferred_cluster_.reset(new pcl::PointCloud<PointT>());
                supervoxel_cloud_.reset(new pcl::PointCloud<pcl::PointXYZRGBA>());
                supervoxel_cloud_organized_.reset(new pcl::PointCloud<pcl::PointXYZRGBA>());
                sift_signatures_.reset (new pcl::PointCloud<FeatureT>());
                is_pre_labelled_ = false;
            }
        };
    }
}


#endif //V4R_OBJECT_MODELLING_MODELVIEW_H__