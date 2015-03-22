/*
 * shot_local_estimator.h
 *
 *  Created on: Mar 24, 2012
 *      Author: aitor
 */

#ifndef FAAT_PCL_REC_FRAMEWORK_SHOT_LOCAL_ESTIMATOR_OMP_H_
#define FAAT_PCL_REC_FRAMEWORK_SHOT_LOCAL_ESTIMATOR_OMP_H_

#include "local_estimator.h"
#include "v4r/ORUtils/faat_3d_rec_framework_defines.h"
#include "normal_estimator.h"
#include <pcl/features/shot_omp.h>
#include <pcl/io/pcd_io.h>

namespace faat_pcl
{
  namespace rec_3d_framework
  {
    template<typename PointInT, typename FeatureT>
      class FAAT_3D_FRAMEWORK_API SHOTLocalEstimationOMP : public LocalEstimator<PointInT, FeatureT>
      {

        typedef typename pcl::PointCloud<PointInT>::Ptr PointInTPtr;
        typedef typename pcl::PointCloud<FeatureT>::Ptr FeatureTPtr;
        typedef pcl::PointCloud<pcl::PointXYZ> KeypointCloud;

        using LocalEstimator<PointInT, FeatureT>::support_radius_;
        using LocalEstimator<PointInT, FeatureT>::normal_estimator_;
        using LocalEstimator<PointInT, FeatureT>::keypoint_extractor_;
        using LocalEstimator<PointInT, FeatureT>::neighborhood_indices_;
        using LocalEstimator<PointInT, FeatureT>::neighborhood_dist_;
        using LocalEstimator<PointInT, FeatureT>::adaptative_MLS_;
        using LocalEstimator<PointInT, FeatureT>::normals_;
        using LocalEstimator<PointInT, FeatureT>::keypoint_indices_;

        pcl::PointIndices indices_;

      public:

		//~SHOTLocalEstimationOMP () {};

        void
        setIndices (const pcl::PointIndices & p_indices)
        {
          indices_ = p_indices;
        }

        size_t getFeatureType() const
        {
            return SHOT;
        }

        void
        setIndices(const std::vector<int> & p_indices)
        {
          indices_.indices = p_indices;
        }

        bool acceptsIndices() const
        {
          return true;
        }

        bool
        estimate (const PointInTPtr & in, PointInTPtr & processed, PointInTPtr & keypoints, FeatureTPtr & signatures)
        {
          if (!normal_estimator_)
          {
            PCL_ERROR("SHOTLocalEstimationOMP :: This feature needs normals... please provide a normal estimator\n");
            return false;
          }

          if (keypoint_extractor_.size() == 0)
          {
            PCL_ERROR("SHOTLocalEstimationOMP :: This feature needs a keypoint extractor... please provide one\n");
            return false;
          }

          PointInTPtr in_cloud(new pcl::PointCloud<PointInT>);
          if(indices_.indices.size() == 0)
          {
              in_cloud = in;
          }
          else
          {
              PCL_WARN("shot local estimator ---> indices were set!\n");
              pcl::copyPointCloud(*in, indices_, *in_cloud);
          }

          //pcl::PointCloud<pcl::Normal>::Ptr normals (new pcl::PointCloud<pcl::Normal>);
          pcl::MovingLeastSquares<PointInT, PointInT> mls;
          if (adaptative_MLS_)
          {
            typename pcl::search::KdTree<PointInT>::Ptr tree;
            Eigen::Vector4f centroid_cluster;
            pcl::compute3DCentroid (*in_cloud, centroid_cluster);
            float dist_to_sensor = centroid_cluster.norm ();
            float sigma = dist_to_sensor * 0.01f;
            mls.setSearchMethod (tree);
            mls.setSearchRadius (sigma);
            mls.setUpsamplingMethod (mls.SAMPLE_LOCAL_PLANE);
            mls.setUpsamplingRadius (0.002);
            mls.setUpsamplingStepSize (0.001);
          }

          normals_.reset (new pcl::PointCloud<pcl::Normal>);
          {
            pcl::ScopeTime t ("Compute normals");
            normal_estimator_->estimate (in_cloud, processed, normals_);
          }

          if (adaptative_MLS_)
          {
            mls.setInputCloud (processed);

            PointInTPtr filtered (new pcl::PointCloud<PointInT>);
            mls.process (*filtered);

            processed.reset (new pcl::PointCloud<PointInT>);
            normals_.reset (new pcl::PointCloud<pcl::Normal>);
            {
              pcl::ScopeTime t ("Compute normals after MLS");
              filtered->is_dense = false;
              normal_estimator_->estimate (filtered, processed, normals_);
            }
          }

          this->computeKeypoints(processed, keypoints, normals_);
          std::cout << " " << normals_->points.size() << " " << processed->points.size() << std::endl;

          if (keypoints->points.size () == 0)
          {
            PCL_WARN("SHOTLocalEstimationOMP :: No keypoints were found\n");
            return false;
          }

          //compute signatures
          typedef typename pcl::SHOTEstimationOMP<PointInT, pcl::Normal, pcl::SHOT352> SHOTEstimator;
          typename pcl::search::KdTree<PointInT>::Ptr tree (new pcl::search::KdTree<PointInT>);
          tree->setInputCloud (processed);

          pcl::PointCloud<pcl::SHOT352>::Ptr shots (new pcl::PointCloud<pcl::SHOT352>);
          SHOTEstimator shot_estimate;
          shot_estimate.setNumberOfThreads (8);
          shot_estimate.setSearchMethod (tree);
          shot_estimate.setInputCloud (keypoints);
          shot_estimate.setSearchSurface(processed);
          shot_estimate.setInputNormals (normals_);
          shot_estimate.setRadiusSearch (support_radius_);

          {
            pcl::ScopeTime t ("Compute SHOT");
            shot_estimate.compute (*shots);
          }

          signatures->resize (shots->points.size ());
          signatures->width = static_cast<int> (shots->points.size ());
          signatures->height = 1;

          int size_feat = sizeof(signatures->points[0].histogram) / sizeof(float);

          int good = 0;
          for (size_t k = 0; k < shots->points.size (); k++)
          {

            int NaNs = 0;
            for (int i = 0; i < size_feat; i++)
            {
              if (!pcl_isfinite(shots->points[k].descriptor[i]))
                NaNs++;
            }

            if (NaNs == 0)
            {
              for (int i = 0; i < size_feat; i++)
              {
                signatures->points[good].histogram[i] = shots->points[k].descriptor[i];
              }

              keypoints->points[good] = keypoints->points[k];
              keypoint_indices_.indices[good] = keypoint_indices_.indices[k];

              good++;
            }
          }

          keypoint_indices_.indices.resize(good);
          keypoints->points.resize(good);
          keypoints->width = good;
          signatures->resize (good);
          signatures->width = good;

          if(signatures->size() != keypoints->points.size())
          {
              PCL_ERROR("Number of signatures different that number of keypoints %d %d\n", (int)signatures->size(), (int)keypoints->points.size());
          }
          indices_.indices.clear ();
          return true;

        }

        bool
        needNormals ()
        {
          return true;
        }
      };
  }
}

#endif /* REC_FRAMEWORK_SHOT_LOCAL_ESTIMATOR_OMP_H_ */
