/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 25/11/14.

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

#ifndef __image_atomic_value_h__
#define __image_atomic_value_h__

#include <iostream>


namespace MR
{
  namespace Image
  {
    namespace Atomic
    {


    template <typename ValueType> class Voxel;


    template <typename ValueType> class Value
    {
      public:
        typedef ValueType value_type;

        Value (Atomic::Voxel<ValueType>& parent) : S (parent) { }
        operator value_type () const {
          return S.get_value();
        }
        void operator= (value_type value) {
          S.update_value ([&](value_type old){ return value; });
        }
        // A copy-constructor would no longer be atomic...
        value_type operator= (const Atomic::Value<ValueType>& V) = delete;
        void operator+= (value_type value) {
          S.update_value ([&](value_type old){ return (old + value); });
        }
        void operator-= (value_type value) {
          S.update_value ([&](value_type old){ return (old - value); });
        }
        void operator*= (value_type value) {
          S.update_value ([&](value_type old){ return (old * value); });
        }
        void operator/= (value_type value) {
          S.update_value ([&](value_type old){ return (old / value); });
        }

        //! return RAM address of current voxel
        value_type* address () { 
          return S.address();
        }

        friend std::ostream& operator<< (std::ostream& stream, const Atomic::Value<ValueType>& V) {
          stream << V.get_value();
          return stream;
        }
      private:
        Atomic::Voxel<ValueType>& S;

        value_type get_value () const {
          return S.get_value();
        }
    };

    }
  }
}

#endif

