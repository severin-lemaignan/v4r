/**
 * $Id$
 *
 * Copyright (c) 2014, Johann Prankl
 * @author Johann Prankl (prankl@acin.tuwien.ac.at)
 */

#include "CodebookMatcher.hh"
#include "v4r/KeypointTools/ScopeTime.hpp"


namespace kp 
{


using namespace std;


inline bool cmpViewRandDec(const std::pair<int,int> &i, const std::pair<int,int> &j)
{
  return (i.second>j.second);
}


/************************************************************************************
 * Constructor/Destructor
 */
CodebookMatcher::CodebookMatcher(const Parameter &p)
 : param(p), max_view_index(0)
{ 
  rnn.dbg = true;
}

CodebookMatcher::~CodebookMatcher()
{
}





/***************************************************************************************/

/**
 * @brief CodebookMatcher::clear
 */
void CodebookMatcher::clear()
{
  max_view_index = 0;
  descs.clear();
  vk_indices.clear();
}

/**
 * @brief CodebookMatcher::addView
 * @param descriptors
 * @param view_idx
 */
void CodebookMatcher::addView(const cv::Mat &descriptors, int view_idx)
{
  descs.reserve(descs.rows+descriptors.rows, descriptors.cols);
  vk_indices.reserve(vk_indices.size()+descriptors.rows);

  for (unsigned i=0; i<(unsigned)descriptors.rows; i++)
  {
    descs.push_back(&descriptors.at<float>(i,0), descriptors.cols);
    vk_indices.push_back(std::make_pair(view_idx,i));
  }

  if (view_idx > max_view_index)
    max_view_index = view_idx;
}

/**
 * @brief CodebookMatcher::createCodebook
 */
void CodebookMatcher::createCodebook()
{
  kp::ScopeTime t("CodebookMatcher::createCodebook");

  // rnn clustering
  kp::DataMatrix2Df centers;
  std::vector<std::vector<int> > clusters;

  rnn.param.dist_thr = param.thr_desc_rnn;
  rnn.cluster(descs);
  rnn.getClusters(clusters);
  rnn.getCenters(centers);

  cb_entries.clear();
  cb_entries.resize(clusters.size());
  cb_centers = cv::Mat_<float>(clusters.size(), centers.cols);

  for (unsigned i=0; i<clusters.size(); i++)
  {
    for (unsigned j=0; j<clusters[i].size(); j++)
      cb_entries[i].push_back(vk_indices[clusters[i][j]]);

    cv::Mat_<float>(1,centers.cols,&centers(i,0)).copyTo(cb_centers.row(i));
  }

  cout<<"codbeook.size()="<<clusters.size()<<"/"<<descs.rows<<endl;

  // create flann for matching
  { kp::ScopeTime t("create FLANN");
  matcher = new cv::FlannBasedMatcher();
  matcher->add(std::vector<cv::Mat>(1,cb_centers));
  matcher->train();
  }

  // once the codebook is created clear the temp containers
  rnn = ClusteringRNN();
  descs = DataMatrix2Df();
  vk_indices = std::vector< std::pair<int,int> >();
  cb_centers.release();

}

/**
 * @brief CodebookMatcher::queryViewRank
 * @param descriptors
 * @param view_rank <view_index, rank_number>  sorted better first
 */
void CodebookMatcher::queryViewRank(const cv::Mat &descriptors, std::vector< std::pair<int, int> > &view_rank)
{
  std::vector< std::vector< cv::DMatch > > cb_matches;

  matcher->knnMatch( descriptors, cb_matches, 2 );

  view_rank.resize(max_view_index+1);

  for (unsigned i=0; i<view_rank.size(); i++)
    view_rank[i] = std::make_pair((int)i,0.);

  for (unsigned i=0; i<cb_matches.size(); i++)
  {
    if (cb_matches[i].size()>1)
    {
      cv::DMatch &ma0 = cb_matches[i][0];

      if (ma0.distance/cb_matches[i][1].distance < param.nnr)
      {
        const std::vector< std::pair<int,int> > &occs = cb_entries[ma0.trainIdx];

        for (unsigned j=0; j<occs.size(); j++)
          view_rank[occs[j].first].second++;
      }
    }
  }

  //sort
  std::sort(view_rank.begin(),view_rank.end(),cmpViewRandDec);
}

/**
 * @brief CodebookMatcher::queryMatches
 * @param descriptors
 * @param matches
 */
void CodebookMatcher::queryMatches(const cv::Mat &descriptors, std::vector< std::vector< cv::DMatch > > &matches)
{
  std::vector< std::vector< cv::DMatch > > cb_matches;

  matcher->knnMatch( descriptors, cb_matches, 2 );

  matches.clear();
  matches.resize(descriptors.rows);
  view_rank.resize(max_view_index+1);

  for (unsigned i=0; i<view_rank.size(); i++)
    view_rank[i] = std::make_pair((int)i,0.);

  for (unsigned i=0; i<cb_matches.size(); i++)
  {
    if (cb_matches[i].size()>1)
    {
      cv::DMatch &ma0 = cb_matches[i][0];
      if (ma0.distance/cb_matches[i][1].distance < param.nnr)
      {
        std::vector< cv::DMatch > &ms = matches[ma0.queryIdx];
        const std::vector< std::pair<int,int> > &occs = cb_entries[ma0.trainIdx];

        for (unsigned j=0; j<occs.size(); j++)
        {
          ms.push_back(cv::DMatch(ma0.queryIdx,occs[j].second,occs[j].first,ma0.distance));
          view_rank[occs[j].first].second++;
        }
      }
    }
  }

  //sort
  std::sort(view_rank.begin(),view_rank.end(),cmpViewRandDec);
}




}











