// This file is part of the AliceVision project.
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <aliceVision/feature/Descriptor.hpp>
#include <aliceVision/system/Logger.hpp>

#include <boost/progress.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp> 
#include <boost/algorithm/string/case_conv.hpp> 

#include <cstdlib>

namespace bfs = boost::filesystem;
namespace po = boost::program_options;

using namespace aliceVision;
/*
 * 
 */
int main( int argc, char** argv )
{
  std::string outputFolder;
  std::string inputFolder;
  bool doSanityCheck = false;
  const int siftSize = 128;

  po::options_description desc("This program is used to convert SIFT features from float representation to unsigned char representation");
  desc.add_options()
        ("inputFolder,i", po::value<std::string>(&inputFolder)->required(), "Input folder containing the sift in float format.")
        ("outputFolder,o", po::value<std::string>(&outputFolder)->required(), "Output folder that stores the sift in uchar format.")
        ("sanityCheck,s", po::value<bool>(&doSanityCheck)->default_value(doSanityCheck), "Perform a sanity check to check that the conversion and the genrated files are the same.");

  po::variables_map vm;

  try
  {
    po::store(po::parse_command_line(argc, argv, desc), vm);

    if(vm.count("help") || (argc == 1))
    {
      ALICEVISION_COUT(desc);
      return EXIT_SUCCESS;
    }

    po::notify(vm);
  }
  catch(boost::program_options::required_option& e)
  {
    ALICEVISION_CERR("ERROR: " << e.what() << std::endl);
    ALICEVISION_COUT("Usage:\n\n" << desc);
    return EXIT_FAILURE;
  }
  catch(boost::program_options::error& e)
  {
    ALICEVISION_CERR("ERROR: " << e.what() << std::endl);
    ALICEVISION_COUT("Usage:\n\n" << desc);
    return EXIT_FAILURE;
  }

  if(!(bfs::exists(inputFolder) && bfs::is_directory(inputFolder)))
  {
    ALICEVISION_CERR("ERROR: " << inputFolder << " does not exists or it is not a folder" << std::endl);
    return EXIT_FAILURE;
  }

  // if the folder does not exist create it (recursively)
  if(!bfs::exists(outputFolder))
  {
    bfs::create_directories(outputFolder);
  }
  
  size_t countFeat = 0;
  size_t countDesc = 0;

  bfs::directory_iterator iterator(inputFolder);
  for(; iterator != bfs::directory_iterator(); ++iterator)
  {
    // get the extension of the current file to check whether it is an image
    std::string ext = iterator->path().extension().string();
    const std::string &filename = iterator->path().filename().string();
    boost::to_lower(ext);

    if(ext == ".feat")
    {
      // just copy the file into the output folder
      bfs::copy_file(iterator->path(), bfs::path(outputFolder)/bfs::path(filename), bfs::copy_option::overwrite_if_exists);
      
      ++countFeat;
    }
    else if(ext == ".desc")
    {
      const std::string outpath = (bfs::path(outputFolder)/bfs::path(filename)).string(); 
      std::vector<feature::Descriptor<float, siftSize> > floatDescriptors;
      
      // load the float descriptors
      feature::loadDescsFromBinFile(iterator->path().string(), floatDescriptors, false);
      
      const size_t numDesc = floatDescriptors.size();
      
      std::vector<feature::Descriptor<unsigned char, siftSize> > charDescriptors(numDesc);
 
      for(std::size_t i = 0; i < numDesc; ++i)
      {
        float* fptr = floatDescriptors[i].getData();
        assert(fptr!=nullptr);
        unsigned char* uptr = charDescriptors[i].getData();
        assert(uptr!=nullptr);
      
        std::copy(fptr, fptr+siftSize, uptr);
      
        if(!doSanityCheck)
          continue;    
        // check that they are actually the same
        for(std::size_t j = 0; j < siftSize; ++j)
        {      
          const unsigned char compare = (unsigned char) fptr[j];
          assert(compare == uptr[j]);
        }
      }    
      
      assert(charDescriptors.size() == floatDescriptors.size());
      
      // save the unsigned char
      feature::saveDescsToBinFile(outpath, charDescriptors);
      
      if(doSanityCheck)
      {
        // sanity check 
        // reload everything and compare
        floatDescriptors.clear();
        charDescriptors.clear();
        feature::loadDescsFromBinFile(iterator->path().string(), floatDescriptors, false);
        feature::loadDescsFromBinFile(outpath, charDescriptors, false);

        assert(charDescriptors.size() == numDesc);
        assert(charDescriptors.size() == floatDescriptors.size());

        for(std::size_t i = 0; i < numDesc; ++i)
        {
          const feature::Descriptor<float, siftSize> &currFloat = floatDescriptors[i];
          const feature::Descriptor<unsigned char, siftSize> &currUchar = charDescriptors[i];
          for(std::size_t j = 0; j < siftSize; ++j)
          {
            const unsigned char compare = (unsigned char) currFloat[j];
            assert(compare == currUchar[j]);
          }
        }
      }
      ++countDesc;
    }
  }
  ALICEVISION_COUT("Converted " << countDesc << " files .desc and copied " << countFeat << " files .feat");
}
