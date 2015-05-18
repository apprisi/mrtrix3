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

#ifndef __dwi_tractography_seeding_basic_h__
#define __dwi_tractography_seeding_basic_h__

#include "dwi/tractography/roi.h"
#include "dwi/tractography/seeding/base.h"


// By default, the rejection sampler will perform its sampling based on image intensity values,
//   and then randomly select a position within that voxel
// Use this flag to instead perform rejection sampling on the trilinear-interpolated value
//   at each trial seed point
//#define REJECTION_SAMPLING_USE_INTERPOLATION



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Seeding
      {


        class Sphere : public Base
        {

          public:
            Sphere (const std::string& in) :
              Base (in, "sphere", MAX_TRACKING_SEED_ATTEMPTS_RANDOM) {
                auto F = parse_floats (in);
                if (F.size() != 4)
                  throw Exception ("Could not parse seed \"" + in + "\" as a spherical seed point; needs to be 4 comma-separated values (XYZ position, then radius)");
                pos = { float(F[0]), float(F[1]), float(F[2]) };
                rad = F[3];
                volume = 4.0*Math::pi*Math::pow3(rad)/3.0;
              }

            virtual bool get_seed (Math::RNG::Uniform<float>& rng, Eigen::Vector3f& p) const override;

          private:
            Eigen::Vector3f pos;
            float rad;

        };


        class SeedMask : public Base
        {

          public:
            SeedMask (const std::string& in) :
              Base (in, "random seeding mask", MAX_TRACKING_SEED_ATTEMPTS_RANDOM),
              mask (new Mask (in)) {
                volume = get_count (*mask) * mask->voxsize(0) * mask->voxsize(1) * mask->voxsize(2);
              }

            virtual bool get_seed (Math::RNG::Uniform<float>& rng, Eigen::Vector3f& p) const override;

          private:
            std::unique_ptr<Mask> mask;

        };



        class Random_per_voxel : public Base
        {

          public:
            Random_per_voxel (const std::string& in, const size_t num_per_voxel) :
              Base (in, "random per voxel", MAX_TRACKING_SEED_ATTEMPTS_FIXED),
              mask (new Mask (in)),
              num (num_per_voxel),
              vox (0, 0, -1),
              inc (0),
              expired (false) {
                count = get_count (*mask) * num_per_voxel;
              }

            virtual bool get_seed (Math::RNG::Uniform<float>& rng, Eigen::Vector3f& p) const override;

          private:
            std::unique_ptr<Mask> mask;
            const size_t num;
            mutable Eigen::Vector3i vox;
            mutable uint32_t inc;
            mutable bool expired;

        };



        class Grid_per_voxel : public Base
        {

          public:
            Grid_per_voxel (const std::string& in, const size_t os_factor) :
              Base (in, "grid per voxel", MAX_TRACKING_SEED_ATTEMPTS_FIXED),
              mask (new Mask (in)),
              os (os_factor),
              vox (0, 0, -1),
              pos (os, os, os),
              offset (-0.5 + (1.0 / (2*os))),
              step (1.0 / os),
              expired (false) {
                count = get_count (*mask) * Math::pow3 (os_factor);
              }

            virtual bool get_seed (Math::RNG::Uniform<float>&, Eigen::Vector3f& p) const override;

          private:
            std::unique_ptr<Mask> mask;
            const int os;
            mutable Eigen::Vector3i vox, pos;
            const float offset, step;
            mutable bool expired;

        };



        class Rejection : public Base
        {
          public:
            Rejection (const std::string&);

            virtual bool get_seed (Math::RNG::Uniform<float>& rng, Eigen::Vector3f& p) const override;

          private:
            Image<float> image;
#ifdef REJECTION_SAMPLING_USE_INTERPOLATION
            Interp::Linear<Image<float>> interp;
#else
            transform_type voxel2scanner;
#endif
            float max;

        };






      }
    }
  }
}

#endif

