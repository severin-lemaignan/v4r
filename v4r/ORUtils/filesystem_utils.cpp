#include "filesystem_utils.h"

void faat_pcl::utils::getFilesInDirectory(bf::path & path_with_views,
                       std::vector<std::string> & view_filenames,
                       const std::string & pattern)
{

  std::stringstream filter_str;
  filter_str << pattern;
  const boost::regex my_filter( filter_str.str() );

  bf::directory_iterator end_itr;
  for (bf::directory_iterator itr (path_with_views); itr != end_itr; ++itr)
  {
    if (!(bf::is_directory (*itr)))
    {
      std::vector < std::string > strs;
      std::vector < std::string > strs_;

#if BOOST_FILESYSTEM_VERSION == 3
      std::string file = (itr->path ().filename ()).string();
#else
      std::string file = (itr->path ()).filename ();
#endif

      boost::smatch what;
      if( !boost::regex_match( file, what, my_filter ) ) continue;

#if BOOST_FILESYSTEM_VERSION == 3
        view_filenames.push_back ((itr->path ().filename ()).string());
#else
        view_filenames.push_back ((itr->path ()).filename ());
#endif
    }
  }
}


void
faat_pcl::utils::getFoldersInDirectory (bf::path & dir, std::string & rel_path_so_far, std::vector<std::string> & relative_paths)
{
  bf::directory_iterator end_itr;
  for (bf::directory_iterator itr (dir); itr != end_itr; ++itr)
  {
    //check if its a directory, else ignore
    if (bf::is_directory (*itr))
    {
#if BOOST_FILESYSTEM_VERSION == 3
      std::string path = rel_path_so_far + (itr->path ().filename ()).string();
#else
      std::string path = rel_path_so_far + (itr->path ()).filename ();
#endif
      relative_paths.push_back (path);
    }
  }
}


void faat_pcl::utils::getFilesInDirectory (bf::path & dir, std::string & rel_path_so_far, std::vector<std::string> & relative_paths, std::string & ext)
{
  bf::directory_iterator end_itr;
  for (bf::directory_iterator itr (dir); itr != end_itr; ++itr)
  {
    //check if its a directory, then get models in it
    if (bf::is_directory (*itr))
    {
#if BOOST_FILESYSTEM_VERSION == 3
      std::string so_far = rel_path_so_far + (itr->path ().filename ()).string () + "/";
#else
      std::string so_far = rel_path_so_far + (itr->path ()).filename () + "/";
#endif

      bf::path curr_path = itr->path ();
      getFilesInDirectory (curr_path, so_far, relative_paths, ext);
    }
    else
    {
      //check that it is a ply file and then add, otherwise ignore..
      std::vector<std::string> strs;
#if BOOST_FILESYSTEM_VERSION == 3
      std::string file = (itr->path ().filename ()).string ();
#else
      std::string file = (itr->path ()).filename ();
#endif

      boost::split (strs, file, boost::is_any_of ("."));
      std::string extension = strs[strs.size () - 1];

      if (extension.compare (ext) == 0)
      {
#if BOOST_FILESYSTEM_VERSION == 3
        std::string path = rel_path_so_far + (itr->path ().filename ()).string ();
#else
        std::string path = rel_path_so_far + (itr->path ()).filename ();
#endif

        relative_paths.push_back (path);
      }
    }
  }
}


void faat_pcl::utils::getFilesInDirectoryRecursive (bf::path & path_with_views,
                                std::string & rel_path_so_far,
                                std::vector<std::string> & view_filenames,
                                const std::string & pattern)
{

  std::stringstream filter_str;
  filter_str << pattern;
  const boost::regex my_filter( filter_str.str() );

  bf::directory_iterator end_itr;
  for (bf::directory_iterator itr (path_with_views); itr != end_itr; ++itr)
  {
    if (!(bf::is_directory (*itr)))
    {
      std::vector < std::string > strs;
      std::vector < std::string > strs_;

#if BOOST_FILESYSTEM_VERSION == 3
      std::string file = rel_path_so_far + (itr->path ().filename ()).string();
#else
      std::string file = rel_path_so_far + (itr->path ()).filename ();
#endif

      boost::smatch what;
      if( !boost::regex_match( file, what, my_filter ) ) continue;

#if BOOST_FILESYSTEM_VERSION == 3
        view_filenames.push_back (file);
#else
        view_filenames.push_back (file);
#endif
    }
    else
    {
#if BOOST_FILESYSTEM_VERSION == 3
      std::string so_far = rel_path_so_far + (itr->path ().filename ()).string () + "/";
#else
      std::string so_far = rel_path_so_far + (itr->path ()).filename () + "/";
#endif

      bf::path curr_path = itr->path ();
      getFilesInDirectoryRecursive (curr_path, so_far, view_filenames, pattern);
    }
  }
}


bool faat_pcl::utils::writeMatrixToFile (std::string file, Eigen::Matrix4f & matrix)
{
  std::ofstream out (file.c_str ());
  if (!out)
  {
    std::cout << "Cannot open file.\n";
    return false;
  }

  for (size_t i = 0; i < 4; i++)
  {
    for (size_t j = 0; j < 4; j++)
    {
      out << matrix (i, j);
      if (!(i == 3 && j == 3))
        out << " ";
    }
  }
  out.close ();

  return true;
}

bool faat_pcl::utils::readMatrixFromFile (std::string file, Eigen::Matrix4f & matrix)
{

  std::ifstream in;
  in.open (file.c_str (), std::ifstream::in);

  char linebuf[1024];
  in.getline (linebuf, 1024);
  std::string line (linebuf);
  std::vector < std::string > strs_2;
  boost::split (strs_2, line, boost::is_any_of (" "));

  for (int i = 0; i < 16; i++)
  {
    matrix (i / 4, i % 4) = static_cast<float> (atof (strs_2[i].c_str ()));
  }

  return true;
}

bool faat_pcl::utils::readMatrixFromFile (std::string file, Eigen::Matrix4f & matrix, int padding)
{

  std::ifstream in;
  in.open (file.c_str (), std::ifstream::in);

  char linebuf[1024];
  in.getline (linebuf, 1024);
  std::string line (linebuf);
  std::vector < std::string > strs_2;
  boost::split (strs_2, line, boost::is_any_of (" "));

  for (int i = 0; i < 16; i++)
  {
    matrix (i / 4, i % 4) = static_cast<float> (atof (strs_2[padding+i].c_str ()));
  }

  return true;
}

bool
faat_pcl::utils::writeCentroidToFile (std::string file, Eigen::Vector3f & centroid)
{
  std::ofstream out (file.c_str ());
  if (!out)
  {
    std::cout << "Cannot open file.\n";
    return false;
  }

  out << centroid[0] << " " << centroid[1] << " " << centroid[2] << std::endl;
  out.close ();

  return true;
}

bool
faat_pcl::utils::getCentroidFromFile (std::string file, Eigen::Vector3f & centroid)
{
    std::ifstream in;

    in.open (file.c_str (), std::ifstream::in);

    if (!in)
    {
      std::cout << "Cannot open file " << file.c_str () << ".\n";
      return false;
    }

    char linebuf[256];
    in.getline (linebuf, 256);
    std::string line (linebuf);
    std::vector < std::string > strs;
    boost::split (strs, line, boost::is_any_of (" "));
    centroid[0] = static_cast<float> (atof (strs[0].c_str ()));
    centroid[1] = static_cast<float> (atof (strs[1].c_str ()));
    centroid[2] = static_cast<float> (atof (strs[2].c_str ()));

    return true;
}

bool
faat_pcl::utils::writeFloatToFile (std::string file, float value)
{
  std::ofstream out (file.c_str ());
  if (!out)
  {
    std::cout << "Cannot open file.\n";
    return false;
  }

  out << value;
  out.close ();

  return true;
}

bool
faat_pcl::utils::readFloatFromFile (std::string file, float& value)
{

  std::ifstream in;
  in.open (file.c_str (), std::ifstream::in);

  if (!in)
  {
    std::cout << "Cannot open file " << file.c_str () << ".\n";
    return false;
  }

  char linebuf[1024];
  in.getline (linebuf, 1024);
  value = static_cast<float> (atof (linebuf));

  return true;
}
