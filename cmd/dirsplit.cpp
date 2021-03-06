/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#include "command.h"
#include "progressbar.h"
#include "math/vector.h"
#include "math/matrix.h"
#include "math/rng.h"
#include "math/SH.h"
#include "thread.h"
#include "point.h"
#include "dwi/directions/file.h"



using namespace MR;
using namespace App;

void usage () {

DESCRIPTION
  + "split a set of evenly distributed directions (as generated "
  "by dirgen) into approximately uniformly distributed subsets.";

ARGUMENTS
  + Argument ("dirs", "the text file containing the directions.").type_file_in()
  + Argument ("out", "the output partitioned directions").type_file_out().allow_multiple();


OPTIONS
  + Option ("permutations", "number of permutations to try")
  +   Argument ("num").type_integer (1, 1e8)

  + Option ("cartesian", "Output the directions in Cartesian coordinates [x y z] instead of [az el].");
}


typedef double value_type;



class Shared {
  public:
    Shared (const Math::Matrix<value_type>& directions, size_t num_subsets, size_t target_num_permutations) :
      directions (directions), subset (num_subsets), 
      best_energy (std::numeric_limits<value_type>::max()),
      target_num_permutations (target_num_permutations),
      num_permutations (0) {
        size_t s = 0;
        for (size_t n = 0; n < directions.rows(); ++n) {
          subset[s++].push_back (n);
          if (s >= num_subsets) s = 0;
        }
        INFO ("split " + str(directions.rows()) + " directions into subsets with " + 
            str([&]{ std::vector<size_t> c; for (auto& x : subset) c.push_back (x.size()); return c; }()) + " volumes");
      }




    bool update (value_type energy, const std::vector<std::vector<size_t>>& set) 
    {
      std::lock_guard<std::mutex> lock (mutex);
      if (!progress) progress.reset (new ProgressBar ("distributing directions...", target_num_permutations));
      if (energy < best_energy) {
        best_energy = energy;
        best_subset = set;
        progress->set_text ("distributing directions (current best configuration: energy = " + str(best_energy) + ")...");
      }
      ++num_permutations;
      ++(*progress);
      return num_permutations < target_num_permutations;
    }



    value_type energy (size_t i, size_t j) const {
      Point<value_type> a = { directions(i,0), directions(i,1), directions(i,2) };
      Point<value_type> b = { directions(j,0), directions(j,1), directions(j,2) };
      return 1.0 / (a-b).norm2() + 1.0 / (a+b).norm2();
    }


    const std::vector<std::vector<size_t>>& get_init_subset () const { return subset; }
    const std::vector<std::vector<size_t>>& get_best_subset () const { return best_subset; }


  protected:
    const Math::Matrix<value_type>& directions;
    std::mutex mutex;
    std::vector<std::vector<size_t>> subset, best_subset;
    value_type best_energy;
    const size_t target_num_permutations;
    size_t num_permutations;
    std::unique_ptr<ProgressBar> progress;
};







class EnergyCalculator {
  public:
    EnergyCalculator (Shared& shared) : shared (shared), subset (shared.get_init_subset()) { }

    void execute () {
      while (eval()); 
    }


    void next_permutation ()
    {
      size_t i,j;
      std::uniform_int_distribution<size_t> dist(0, subset.size()-1);
      do {
        i = dist (rng);
        j = dist (rng);
      } while (i == j);

      size_t n_i = std::uniform_int_distribution<size_t> (0, subset[i].size()-1) (rng);
      size_t n_j = std::uniform_int_distribution<size_t> (0, subset[j].size()-1) (rng);

      std::swap (subset[i][n_i], subset[j][n_j]);
    }

    bool eval ()
    {
      next_permutation();

      value_type energy = 0.0;
      for (auto& s: subset) {
        value_type current_energy = 0.0;
        for (size_t i = 0; i < s.size(); ++i) 
          for (size_t j = i+1; j < s.size(); ++j) 
            current_energy += shared.energy (s[i], s[j]);
        energy = std::max (energy, current_energy);
      }

      return shared.update (energy, subset);
    }

  protected:
    Shared& shared;
    std::vector<std::vector<size_t>> subset;
    Math::RNG rng;
};







void run () 
{
  Math::Matrix<value_type> directions = DWI::Directions::load_cartesian<value_type> (argument[0]);

  size_t num_subsets = argument.size() - 1;

  size_t num_permutations = 1e8;
  Options opt = get_options ("permutations");
  if (opt.size())
    num_permutations = opt[0][0];

  std::vector<std::vector<size_t>> best;
  {
    Shared shared (directions, num_subsets, num_permutations);
    Thread::run (Thread::multi (EnergyCalculator (shared)), "energy eval thread");
    best = shared.get_best_subset();
  }



  bool cartesian = get_options("cartesian").size();
  for (size_t i = 0; i < best.size(); ++i) {
    Math::Matrix<value_type> output (best[i].size(), 3);
    for (size_t n = 0; n < best[i].size(); ++n) 
      output.row(n) = directions.row (best[i][n]);
    DWI::Directions::save (output, argument[i+1], cartesian);
  }

}


