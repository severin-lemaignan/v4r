#include <v4r/features/sift_local_estimator.h>
#include <v4r/common/pcl_opencv.h>
#include <v4r/common/miscellaneous.h>

namespace v4r
{

template<typename PointInT, typename FeatureT>
bool
SIFTLocalEstimation<PointInT, FeatureT>::estimate(const PointInTPtr & in, FeatureTPtr & signatures)
{
    //fill keypoints with indices_, all points at indices_[i] should be valid
    std::vector<SiftGPU::SiftKeypoint> ks;
    if(indices_.empty())
    {
        PCL_ERROR("indices are empty\n");
        return false;
    }

    ks.resize(indices_.size());
    for(size_t i=0; i < indices_.size(); i++)
    {
        ks[i].y = (float)(indices_[i] / in->width);
        ks[i].x = (float)(indices_[i] % in->width);
        ks[i].s = 1.f;
        ks[i].o = 0.f;
    }

    std::cout << "Number of keypoints:" << ks.size() << std::endl;

    cv::Mat_ < cv::Vec3b > colorImage = ConvertPCLCloud2Image (*in);
    cv::Mat grayImage;
    cv::cvtColor (colorImage, grayImage, CV_BGR2GRAY);

    if (sift_->CreateContextGL () != SiftGPU::SIFTGPU_FULL_SUPPORTED)
        throw std::runtime_error ("PSiftGPU::PSiftGPU: No GL support!");

    sift_->VerifyContextGL();

    SiftGPU *pSift = (SiftGPU*)&(*sift_);
    pSift->SetKeypointList(ks.size(), &ks[0], 0);
    sift_->VerifyContextGL();

    cv::Mat descriptors;
    if(pSift->RunSIFT(grayImage.cols,grayImage.rows,grayImage.ptr<uchar>(0),GL_LUMINANCE,GL_UNSIGNED_BYTE))
    {
        int num = pSift->GetFeatureNum();
        if (num==(int)ks.size())
        {
            descriptors = cv::Mat(num,128,CV_32F);
            pSift->GetFeatureVector(NULL, descriptors.ptr<float>(0));
        }
        else std::cout<<"No SIFT found"<< std::endl;
    }

    signatures->resize (ks.size ());
    signatures->width = static_cast<int> (ks.size ());
    signatures->height = 1;
    for(size_t i=0; i < ks.size(); i++)
    {
        for (int k = 0; k < 128; k++)
            signatures->points[i].histogram[k] = descriptors.at<float>(i,k);
    }

    return true;
}

template<typename PointInT, typename FeatureT>
bool
SIFTLocalEstimation<PointInT, FeatureT>::estimate (const PointInTPtr & in, PointInTPtr & processed, PointInTPtr & keypoints, FeatureTPtr & signatures)
{
    if(!processed)
        processed.reset(new pcl::PointCloud<PointInT>);
    *processed = *in;

    std::vector<float> scale;
    return estimate(in, keypoints, signatures, scale);
}

template<typename PointInT, typename FeatureT>
bool
SIFTLocalEstimation<PointInT, FeatureT>::estimate (const PointInTPtr & in, PointInTPtr & keypoints, FeatureTPtr & signatures, std::vector<float> & scales)
{
    if(!keypoints)
        keypoints.reset(new pcl::PointCloud<PointInT>);

    cv::Mat colorImage = ConvertPCLCloud2Image(*in);
    cv::Mat grayImage;
    cv::cvtColor (colorImage, grayImage, CV_BGR2GRAY);

    cv::Mat descriptors;
    std::vector<SiftGPU::SiftKeypoint> ks;

    SiftGPU *pSift = (SiftGPU*)&(*sift_);

    if (sift_->CreateContextGL () != SiftGPU::SIFTGPU_FULL_SUPPORTED)
        throw std::runtime_error ("PSiftGPU::PSiftGPU: No GL support!");

    sift_->VerifyContextGL();
    if (sift_->RunSIFT (grayImage.cols, grayImage.rows, grayImage.ptr<uchar> (0), GL_LUMINANCE, GL_UNSIGNED_BYTE))
    {
        int num = sift_->GetFeatureNum ();
        if (num > 0)
        {
            ks.resize(num);
            descriptors = cv::Mat(num,128,CV_32F);
            pSift->GetFeatureVector(&ks[0], descriptors.ptr<float>(0));
        }
        else
            std::cout<<"No SIFT found"<< std::endl;
    }
    else
        throw std::runtime_error ("PSiftGPU::Detect: SiftGPU Error!");

    //use indices_ to check if the keypoints and feature should be saved
    //compute SIFT keypoints and SIFT features
    //backproject sift keypoints to 3D and save in keypoints
    //save signatures

    scales.resize(ks.size());
    keypoints->points.resize(ks.size());
    keypoint_indices_.resize(ks.size());
    signatures->points.resize (ks.size ());
    signatures->width = static_cast<int> (ks.size ());
    signatures->height = 1;

    std::vector<bool> obj_mask;
    if(indices_.empty())
        obj_mask.resize(in->width * in->height, true);
    else
        obj_mask = createMaskFromIndices(indices_, in->width * in->height);

    size_t kept = 0;
    for(size_t i=0; i < ks.size(); i++)
    {
        const int v = (int)(ks[i].y+.5);
        const int u = (int)(ks[i].x+.5);
        const int idx = v * in->width + u;

        if(u >= 0 && v >= 0 && u < in->width && v < in->height && pcl::isFinite(in->points[idx]) && obj_mask[idx])
        {
            keypoints->points[kept] = in->at(u,v);
            keypoint_indices_[kept] = idx;
            scales[kept] = ks[i].s;

            for (size_t k = 0; k < 128; k++)
                signatures->points[kept].histogram[k] = descriptors.at<float>(i,k);

            kept++;
        }
    }

    keypoints->points.resize(kept);
    scales.resize(kept);
    keypoint_indices_.resize(kept);
    signatures->points.resize(kept);
    signatures->width = kept;

    std::cout << "Number of SIFT features:" << kept << std::endl;
    indices_.clear();

    return true;
}


template<typename PointInT, typename FeatureT>
bool
SIFTLocalEstimation<PointInT, FeatureT>::estimate (const cv::Mat_ < cv::Vec3b > colorImage, std::vector<SiftGPU::SiftKeypoint> & ks, FeatureTPtr & signatures, std::vector<float> & scales)
{
    cv::Mat grayImage;
    cv::cvtColor (colorImage, grayImage, CV_BGR2GRAY);

    cv::Mat descriptors;

    if (sift_->CreateContextGL () != SiftGPU::SIFTGPU_FULL_SUPPORTED)
        throw std::runtime_error ("PSiftGPU::PSiftGPU: No GL support!");

    sift_->VerifyContextGL();
    if (sift_->RunSIFT (grayImage.cols, grayImage.rows, grayImage.ptr<uchar> (0), GL_LUMINANCE, GL_UNSIGNED_BYTE))
    {
        int num = sift_->GetFeatureNum ();
        if (num > 0)
        {
            ks.resize(num);
            descriptors = cv::Mat(num,128,CV_32F);
            sift_->GetFeatureVector(&ks[0], descriptors.ptr<float>(0));
        }
        else std::cout<<"No SIFT found"<< std::endl;
    }
    else
        throw std::runtime_error ("PSiftGPU::Detect: SiftGPU Error!");

    //use indices_ to check if the keypoints and feature should be saved
    //compute SIFT keypoints and SIFT features
    //backproject sift keypoints to 3D and save in keypoints
    //save signatures

    scales.resize(ks.size());
    signatures->resize (ks.size ());
    signatures->width = static_cast<int> (ks.size ());
    signatures->height = 1;

    for(size_t i=0; i < ks.size(); i++)
    {
        for (int k = 0; k < 128; k++)
            signatures->points[i].histogram[k] = descriptors.at<float>(i,k);

        scales[i] = ks[i].s;
    }


    std::cout << "Number of SIFT features:" << ks.size() << std::endl;
    indices_.clear();

    return true;
}

template class V4R_EXPORTS SIFTLocalEstimation<pcl::PointXYZRGB, pcl::Histogram<128> >;
}
