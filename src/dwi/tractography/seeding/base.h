/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2012.

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

#ifndef __dwi_tractography_seeding_base_h__
#define __dwi_tractography_seeding_base_h__


#include "image.h"
#include "file/path.h"
#include "algo/threaded_loop.h"
#include "math/rng.h"



// These constants set how many times a tracking algorithm should attempt to propagate
//   from a given seed point, based on the mechanism used to provide the seed point
//
// Mechanisms that provide random seed locations
#define MAX_TRACKING_SEED_ATTEMPTS_RANDOM 1
//
// Dynamic seeding also provides the mean direction of the fixel, so only a small number of
//   attempts should be required to find a direction above the FOD amplitude threshold;
//   this will however depend on this threshold as well as the angular threshold
#define MAX_TRACKING_SEED_ATTEMPTS_DYNAMIC 50
//
// GM-WM interface seeding incurs a decent overhead when generating the seed points;
//   therefore want to make maximal use of each seed point generated, bearing in mind that
//   the FOD amplitudes may be small there.
#define MAX_TRACKING_SEED_ATTEMPTS_GMWMI 300
//
// Mechanisms that provide a fixed number of seed points; hence the maximum effort should
//   be made to find an appropriate tracking direction from every seed point provided
#define MAX_TRACKING_SEED_ATTEMPTS_FIXED 5000




namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Seeding
      {




      template <class ImageType>
      uint32_t get_count (ImageType& data)
      {
        uint32_t count = 0;
        ThreadedLoop (data).run ([&] (decltype(data)& v) { if (v.value()) ++count; }, data);
        return count;
      }


      template <class ImageType>
      float get_volume (ImageType& data)
      {
        default_type volume = 0.0;
        ThreadedLoop (data).run ([&] (decltype(data)& v) { volume += v.value(); }, data);
        return volume;
      }




      // Common interface for providing streamline seeds
      class Base {

        public:
          Base (const std::string& in, const std::string& desc, const size_t attempts) :
            volume (0.0),
            count (0),
            type (desc),
            name (Path::exists (in) ? Path::basename (in) : in),
            max_attempts (attempts) { }

          virtual ~Base() { }

          default_type vol() const { return volume; }
          uint32_t num() const { return count; }
          bool is_finite() const { return count; }
          const std::string& get_type() const { return type; }
          const std::string& get_name() const { return name; }
          size_t get_max_attempts() const { return max_attempts; }

          virtual bool get_seed (Math::RNG::Uniform<float>& rng, Eigen::Vector3f&) const = 0;
          virtual bool get_seed (Math::RNG::Uniform<float>& rng, Eigen::Vector3f& p, Eigen::Vector3f&) const { return get_seed (rng, p); }

          friend inline std::ostream& operator<< (std::ostream& stream, const Base& B) {
            stream << B.name;
            return (stream);
          }


        protected:
          // Finite seeds are defined by the number of seeds; non-limited are defined by volume
          float volume;
          uint32_t count;
          mutable std::mutex mutex;
          const std::string type; // Text describing the type of seed this is

        private:
          const std::string name; // Could be an image path, or spherical coordinates
          const size_t max_attempts; // Maximum number of times the tracking algorithm should attempt to start from each provided seed point

      };





      }
    }
  }
}

#endif

