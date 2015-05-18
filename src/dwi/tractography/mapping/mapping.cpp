/*
   Copyright 2011 Brain Research Institute, Melbourne, Australia

   Written by Robert E. Smith, 2011.

   This file is part of MRtrix.

   MRtrix is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   MRtrix is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

 */





#include "dwi/tractography/mapping/mapping.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Mapping {




        size_t determine_upsample_ratio (const Header& header, const float step_size, const float ratio)
        {
          size_t upsample_ratio = 1;
          if (step_size && std::isfinite (step_size))
            upsample_ratio = std::ceil (step_size / (minvalue (header.voxsize(0), header.voxsize(1), header.voxsize(2)) * ratio));
          return upsample_ratio;
        }

        size_t determine_upsample_ratio (const Header& header, const std::string& tck_path, const float ratio)
        {
          Properties properties;
          Reader reader (tck_path, properties);
          return determine_upsample_ratio (header, properties, ratio);
        }


        size_t determine_upsample_ratio (const Header& header, const Tractography::Properties& properties, const float ratio)
        {
          if (header.ndim() < 3)
            throw Exception ("Cannot perform streamline mapping on image with less than three dimensions");

          Properties::const_iterator i = properties.find ("output_step_size");
          if (i == properties.end()) {
            i = properties.find ("step_size");
            if (i == properties.end())
              throw Exception ("Cannot perform streamline mapping: no step size information in track file header");
          }
          const float step_size = to<float> (i->second);

          return determine_upsample_ratio (header, step_size, ratio);
        }








        void generate_header (Header& header, const std::string& tck_file_path, const std::vector<float>& voxel_size)
        {

          Properties properties;
          Reader file (tck_file_path, properties);

          Streamline<> tck;
          size_t track_counter = 0;

          Eigen::Vector3f min_values ( Inf,  Inf,  Inf);
          Eigen::Vector3f max_values (-Inf, -Inf, -Inf);

          {
            ProgressBar progress ("creating new template image...", 0);
            while (file (tck) && track_counter++ < MAX_TRACKS_READ_FOR_HEADER) {
              for (const auto& i : tck) {
                min_values[0] = std::min (min_values[0], i[0]);
                max_values[0] = std::max (max_values[0], i[0]);
                min_values[1] = std::min (min_values[1], i[1]);
                max_values[1] = std::max (max_values[1], i[1]);
                min_values[2] = std::min (min_values[2], i[2]);
                max_values[2] = std::max (max_values[2], i[2]);
              }
              ++progress;
            }
          }

          min_values -= Eigen::Vector3f (3.0*voxel_size[0], 3.0*voxel_size[1], 3.0*voxel_size[2]);
          max_values += Eigen::Vector3f (3.0*voxel_size[0], 3.0*voxel_size[1], 3.0*voxel_size[2]);

          header.name() = "tckmap image header";
          header.set_ndim (3);

          for (size_t i = 0; i != 3; ++i) {
            header.size(i) = std::ceil((max_values[i] - min_values[i]) / voxel_size[i]);
            header.voxsize(i) = voxel_size[i];
            header.stride(i) = i+1;
          }

          header.transform().matrix().setIdentity();
          header.transform().translation() = min_values.cast<double>();
          file.close();
        }





        void oversample_header (Header& header, const std::vector<float>& voxel_size)
        {
          INFO ("oversampling header...");

          header.transform().translation() += header.transform().rotation() * Eigen::Vector3d (
              0.5 * (voxel_size[0] - header.voxsize(0)),
              0.5 * (voxel_size[1] - header.voxsize(1)),
              0.5 * (voxel_size[2] - header.voxsize(2))
              );

          for (size_t n = 0; n < 3; ++n) {
            header.size(n) = std::ceil(header.size(n) * header.voxsize(n) / voxel_size[n]);
            header.voxsize(n) = voxel_size[n];
          }
        }





      }
    }
  }
}



