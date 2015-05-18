/*
   Copyright 2011 Brain Research Institute, Melbourne, Australia

   Written by Robert E. Smith, 2014.

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


#include "dwi/tractography/editing/worker.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Editing {




        bool Worker::operator() (const Streamline<>& in, Streamline<>& out) const
        {

          out.clear();
          out.index = in.index;
          out.weight = in.weight;

          if (!thresholds (in)) {
            // Want to test thresholds before wasting time on upsampling; but if -inverse is set,
            //   still need to apply both the upsampler and downsampler before writing to output
            if (inverse) {
              std::vector<Eigen::Vector3f> tck (in);
              upsampler (tck);
              downsampler (tck);
              tck.swap (out);
            }
            return true;
          }

          // Upsample track before mapping to ROIs
          std::vector<Eigen::Vector3f> tck (in);
          upsampler (tck);

          // Assign to ROIs
          if (properties.include.size() || properties.exclude.size()) {

            include_visited.assign (properties.include.size(), false);

            if (ends_only) {
              for (size_t i = 0; i != 2; ++i) {
                const Eigen::Vector3f& p (i ? tck.back() : tck.front());
                properties.include.contains (p, include_visited);
                if (properties.exclude.contains (p)) {
                  if (inverse) {
                    downsampler (tck);
                    tck.swap (out);
                  }
                  return true;
                }
              }
            } else {
              for (const auto& p : tck) {
                properties.include.contains (p, include_visited);
                if (properties.exclude.contains (p)) {
                  if (inverse) {
                    downsampler (tck);
                    tck.swap (out);
                  }
                  return true;
                }
              }
            }

            // Make sure all of the include regions were visited
            for (const auto& i : include_visited) {
              if (!i) {
                if (inverse) {
                  downsampler (tck);
                  tck.swap (out);
                }
                return true;
              }
            }

          }

          if (properties.mask.size()) {

            // Split tck into separate tracks based on the mask
            std::vector<std::vector<Eigen::Vector3f>> cropped_tracks;
            std::vector<Eigen::Vector3f> temp;

            for (const auto& p : tck) {
              const bool contains = properties.mask.contains (p);
              if (contains == inverse) {
                if (temp.size() >= 2)
                  cropped_tracks.push_back (temp);
                temp.clear();
              } else {
                temp.push_back (p);
              }
            }
            if (temp.size() >= 2)
              cropped_tracks.push_back (temp);

            if (cropped_tracks.empty())
              return true;

            // Apply downsampler independently to each
            for (auto& i : cropped_tracks)
              downsampler (i);

            if (cropped_tracks.size() == 1) {
              cropped_tracks[0].swap (out);
              return true;
            }

            // Stitch back together in preparation for sending down queue as a single track
            out.push_back (Eigen::Vector3f());
            for (const auto& i : cropped_tracks) {
              for (const auto& p : i)
                out.push_back (p);
              out.push_back ({ NaN, NaN, NaN });
            }
            out.push_back ({ NaN, NaN, NaN });
            return true;

          } else {

            if (!inverse) {
              downsampler (tck);
              tck.swap (out);
            }
            return true;

          }

        }









        Worker::Thresholds::Thresholds (Tractography::Properties& properties) :
          max_num_points (std::numeric_limits<size_t>::max()),
          min_num_points (0),
          max_weight (std::numeric_limits<float>::infinity()),
          min_weight (0.0)
        {

          std::string step_size_string;
          if (properties.find ("output_step_size") == properties.end())
            step_size_string = ((properties.find ("step_size") == properties.end()) ? "0.0" : properties["step_size"]);
          else
            step_size_string = properties["output_step_size"];

          float maxlength = 0.0, minlength = 0.0;
          if (properties.find ("max_dist") != properties.end()) {
            try {
              maxlength = to<float>(properties["max_dist"]);
            } catch (...) { }
          }
          if (properties.find ("min_dist") != properties.end()) {
            try {
              minlength = to<float>(properties["min_dist"]);
            } catch (...) { }
          }

          if (step_size_string == "variable" && (maxlength || minlength))
            throw Exception ("Cannot apply length threshold; step size is inconsistent between input track files");

          const float step_size = to<float>(step_size_string);

          if ((!step_size || !std::isfinite (step_size)) && (maxlength || minlength))
            throw Exception ("Cannot apply length threshold; step size information is incomplete");

          if (maxlength)
            max_num_points = std::round (maxlength / step_size) + 1;
          if (minlength)
            min_num_points = std::max (2, round (minlength/step_size) + 1);

          if (properties.find ("max_weight") != properties.end())
            max_weight = to<float>(properties["max_weight"]);

          if (properties.find ("min_weight") != properties.end())
            min_weight = to<float>(properties["min_weight"]);

        }




        Worker::Thresholds::Thresholds (const Worker::Thresholds& that) :
          max_num_points (that.max_num_points),
          min_num_points (that.min_num_points),
          max_weight (that.max_weight),
          min_weight (that.min_weight) { }




        bool Worker::Thresholds::operator() (const Streamline<>& in) const
        {
          return ((in.size() <= max_num_points) &&
              (in.size() >= min_num_points) &&
              (in.weight <= max_weight) &&
              (in.weight >= min_weight));
        }





      }
    }
  }
}

