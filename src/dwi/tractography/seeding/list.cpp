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


#include "dwi/tractography/seeding/list.h"




namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Seeding
      {




        void List::add (Base* const in)
        {
          if (seeders.size() && !(in->is_finite() == is_finite()))
            throw Exception ("Cannot use a combination of seed types where some are number-limited and some are not!");

          if (!App::get_options ("max_seed_attempts").size()) 
            for (auto& i : seeders) 
              if (i->get_max_attempts() != in->get_max_attempts())
                throw Exception ("Cannot use a combination of seed types where the default maximum number "
                    "of sampling attempts per seed is unequal, unless you use the -max_seed_attempts option.");

          seeders.push_back (std::unique_ptr<Base> (in));
          total_volume += in->vol();
          total_count += in->num();
        }



        void List::clear()
        {
          seeders.clear();
          total_volume = 0.0;
          total_count = 0.0;
        }



        bool List::get_seed (Eigen::Vector3f& p, Eigen::Vector3f& d)
        {

          if (is_finite()) {

            for (auto& i : seeders)
              if (i->get_seed (rng, p, d))
                return true;

            p = { NaN, NaN, NaN };
            return false;

          } else {

            if (seeders.size() == 1)
              return seeders.front()->get_seed (rng, p, d);

            do {
              float incrementer = 0.0;
              const float sample = rng() * total_volume;
              for (auto& i : seeders)
                if ((incrementer += i->vol()) > sample)
                  return i->get_seed (rng, p, d);

            } while (1);
            return false;

          }

        }




      }
    }
  }
}


