#include <v4r/recognition/multi_pipeline_recognizer.h>
#include <pcl/registration/transformation_estimation_svd.h>
#include <v4r/common/normals.h>

namespace v4r
{

template<typename PointT>
bool
MultiRecognitionPipeline<PointT>::initialize(bool force_retrain)
{
    for(auto &r:recognizers_)
        r->initialize(force_retrain);

    if(param_.icp_iterations_ > 0 && param_.icp_type_ == 1)
    {
        for(size_t i=0; i < recognizers_.size(); i++)
            recognizers_[i]->getDataSource()->createVoxelGridAndDistanceTransform(param_.voxel_size_icp_);
    }

    return true;
}

template<typename PointT>
void
MultiRecognitionPipeline<PointT>::getPoseRefinement(
        const std::vector<ModelTPtr> &models,
        std::vector<Eigen::Matrix4f, Eigen::aligned_allocator<Eigen::Matrix4f> > &transforms)
{
    models_ = models;
    transforms_ = transforms;
    poseRefinement();
    transforms = transforms_; //is this neccessary?
}

template<typename PointT>
void
MultiRecognitionPipeline<PointT>::recognize()
{
    models_.clear();
    transforms_.clear();
    std::vector<int> input_icp_indices;
    obj_hypotheses_.clear();
    scene_keypoints_.reset(new pcl::PointCloud<PointT>);
    scene_kp_normals_.reset(new pcl::PointCloud<pcl::Normal>);

    for(const auto &r : recognizers_)
    {
        r->setInputCloud(scene_);

        if(r->requiresSegmentation()) // this might not work in the current state!!
        {
            if( r->acceptsNormals() )
            {
                if ( !scene_normals_ || scene_normals_->points.size() != scene_->points.size() )
                    computeNormals<PointT>(scene_, scene_normals_, param_.normal_computation_method_);

                r->setSceneNormals(scene_normals_);
            }

            for(size_t c=0; c < segmentation_indices_.size(); c++)
            {
                r->recognize();
                const std::vector<ModelTPtr> models_tmp = r->getModels ();
                const std::vector<Eigen::Matrix4f, Eigen::aligned_allocator<Eigen::Matrix4f> > transforms_tmp = r->getTransforms ();

                models_.insert(models_.end(), models_tmp.begin(), models_tmp.end());
                transforms_.insert(transforms_.end(), transforms_tmp.begin(), transforms_tmp.end());
                input_icp_indices.insert(input_icp_indices.end(), segmentation_indices_[c].indices.begin(), segmentation_indices_[c].indices.end());
            }
        }
        else
        {
            r->recognize();

            if(!r->getSaveHypothesesParam())
            {
                const std::vector<ModelTPtr> models = r->getModels ();
                const std::vector<Eigen::Matrix4f, Eigen::aligned_allocator<Eigen::Matrix4f> > transforms = r->getTransforms ();

                models_.insert(models_.end(), models.begin(), models.end());
                transforms_.insert(transforms_.end(), transforms.begin(), transforms.end());
            }
            else
            {
                typename std::map<std::string, ObjectHypothesis<PointT> > oh_tmp;
                r->getSavedHypotheses(oh_tmp);

                std::vector<int> kp_indices;
                typename pcl::PointCloud<PointT>::Ptr kp_tmp = r->getKeypointCloud();
                r->getKeypointIndices(kp_indices);

                for (auto &oh : oh_tmp) {
                    for (auto &corr : *(oh.second.model_scene_corresp_)) {  // add appropriate offset to correspondence index of the scene cloud
                        corr.index_match += scene_keypoints_->points.size();
                    }

                    auto it_mp_oh = obj_hypotheses_.find(oh.first);
                    if(it_mp_oh == obj_hypotheses_.end())   // no feature correspondences exist yet
                        obj_hypotheses_.insert(oh);//std::pair<std::string, ObjectHypothesis<PointT> >(id, it_tmp->second));
                    else
                        it_mp_oh->second.model_scene_corresp_->insert(  it_mp_oh->second.model_scene_corresp_->  end(),
                                                                               oh.second.model_scene_corresp_->begin(),
                                                                               oh.second.model_scene_corresp_->  end() );
                }

                *scene_keypoints_ += *kp_tmp;

                if(cg_algorithm_ && cg_algorithm_->getRequiresNormals()) {

                    if( !scene_normals_ || scene_normals_->points.size() != scene_->points.size())
                        computeNormals<PointT>(scene_, scene_normals_, param_.normal_computation_method_);

                    pcl::PointCloud<pcl::Normal> kp_normals;
                    pcl::copyPointCloud(*scene_normals_, kp_indices, kp_normals);
                    *scene_kp_normals_ += kp_normals;
                }
            }
        }
    }

    compress();

    if(cg_algorithm_ && !param_.save_hypotheses_)    // correspondence grouping is not done outside
    {
        correspondenceGrouping();

        if (param_.icp_iterations_ > 0 || hv_algorithm_) //Prepare scene and model clouds for the pose refinement step
            getDataSource()->voxelizeAllModels (param_.voxel_size_icp_);

        if ( param_.icp_iterations_ > 0 )
            poseRefinement();

        if ( hv_algorithm_ && models_.size() )
            hypothesisVerification();

        scene_keypoints_.reset();
        scene_kp_normals_.reset();
    }
}

template<typename PointT>
void MultiRecognitionPipeline<PointT>::correspondenceGrouping ()
{
    typename std::map<std::string, ObjectHypothesis<PointT> >::iterator it;
    for (it = obj_hypotheses_.begin (); it != obj_hypotheses_.end (); ++it)
    {
        ObjectHypothesis<PointT> &oh = it->second;

        if(oh.model_scene_corresp_->size() < 3)
            continue;

        std::vector < pcl::Correspondences > corresp_clusters;
        cg_algorithm_->setSceneCloud (scene_keypoints_);
        cg_algorithm_->setInputCloud (oh.model_->keypoints_);

        if(cg_algorithm_->getRequiresNormals())
            cg_algorithm_->setInputAndSceneNormals(oh.model_->kp_normals_, scene_kp_normals_);

        //we need to pass the keypoints_pointcloud and the specific object hypothesis
        cg_algorithm_->setModelSceneCorrespondences (oh.model_scene_corresp_);
        cg_algorithm_->cluster (corresp_clusters);

        std::vector<Eigen::Matrix4f, Eigen::aligned_allocator<Eigen::Matrix4f> > new_transforms (corresp_clusters.size());
        typename pcl::registration::TransformationEstimationSVD < PointT, PointT > t_est;

        for (size_t i = 0; i < corresp_clusters.size(); i++)
            t_est.estimateRigidTransformation (*oh.model_->keypoints_, *scene_keypoints_, corresp_clusters[i], new_transforms[i]);

        if(param_.merge_close_hypotheses_) {
            std::vector<Eigen::Matrix4f, Eigen::aligned_allocator<Eigen::Matrix4f> > merged_transforms (corresp_clusters.size());
            std::vector<bool> cluster_has_been_taken(corresp_clusters.size(), false);
            const double angle_thresh_rad = param_.merge_close_hypotheses_angle_ * M_PI / 180.f ;

            size_t kept=0;
            for (size_t i = 0; i < new_transforms.size(); i++) {

                if (cluster_has_been_taken[i])
                    continue;

                cluster_has_been_taken[i] = true;
                const Eigen::Vector3f centroid1 = new_transforms[i].block<3, 1> (0, 3);
                const Eigen::Matrix3f rot1 = new_transforms[i].block<3, 3> (0, 0);

                pcl::Correspondences merged_corrs = corresp_clusters[i];

                for(size_t j=i; j < new_transforms.size(); j++) {
                    const Eigen::Vector3f centroid2 = new_transforms[j].block<3, 1> (0, 3);
                    const Eigen::Matrix3f rot2 = new_transforms[j].block<3, 3> (0, 0);
                    const Eigen::Matrix3f rot_diff = rot2 * rot1.transpose();

                    double rotx = atan2(rot_diff(2,1), rot_diff(2,2));
                    double roty = atan2(-rot_diff(2,0), sqrt(rot_diff(2,1) * rot_diff(2,1) + rot_diff(2,2) * rot_diff(2,2)));
                    double rotz = atan2(rot_diff(1,0), rot_diff(0,0));
                    double dist = (centroid1 - centroid2).norm();

                    if ( (dist < param_.merge_close_hypotheses_dist_) && (rotx < angle_thresh_rad) && (roty < angle_thresh_rad) && (rotz < angle_thresh_rad) ) {
                        merged_corrs.insert( merged_corrs.end(), corresp_clusters[j].begin(), corresp_clusters[j].end() );
                        cluster_has_been_taken[j] = true;
                    }
                }

                t_est.estimateRigidTransformation (*oh.model_->keypoints_, *scene_keypoints_, merged_corrs, merged_transforms[kept]);
                kept++;
            }
            merged_transforms.resize(kept);
            new_transforms = merged_transforms;
        }

        std::cout << "Merged " << corresp_clusters.size() << " clusters into " << new_transforms.size() << " clusters. Total correspondences: " << oh.model_scene_corresp_->size () << " " << it->first << std::endl;

        //        oh.visualize(*scene_);

        size_t existing_hypotheses = models_.size();
        models_.resize( existing_hypotheses + new_transforms.size(), oh.model_  );
        transforms_.insert(transforms_.end(), new_transforms.begin(), new_transforms.end());
    }
}

template<typename PointT>
bool
MultiRecognitionPipeline<PointT>::isSegmentationRequired() const
{
    bool ret_value = false;
    for(size_t i=0; (i < recognizers_.size()) && !ret_value; i++)
        ret_value = recognizers_[i]->requiresSegmentation();

    return ret_value;
}

template<typename PointT>
typename boost::shared_ptr<Source<PointT> >
MultiRecognitionPipeline<PointT>::getDataSource () const
{
    //NOTE: Assuming source is the same or contains the same models for all recognizers...
    //Otherwise, we should create a combined data source so that all models are present

    return recognizers_[0]->getDataSource();
}

}
