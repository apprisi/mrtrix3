/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

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

#ifndef __image_atomic_voxel_h__
#define __image_atomic_voxel_h__

#include "image/atomic/value.h"
#include "image/buffer.h"
#include "image/copy.h"
#include "image/voxel.h"

namespace MR
{
  namespace Image
  {
    namespace Atomic
    {


    template <typename ValueType> class BufferScratch;


    template <class ValueType>
      class Voxel : public Image::Voxel< Atomic::BufferScratch<ValueType> > {
        public:
          Voxel (Atomic::BufferScratch<ValueType>& array) :
              Image::Voxel< Atomic::BufferScratch<ValueType> > (array) { }

          typedef ValueType value_type;
          typedef Voxel voxel_type;

          Atomic::Value<ValueType> value () {
            return Atomic::Value<ValueType> (*this);
          }



          friend std::ostream& operator<< (std::ostream& stream, const Atomic::Voxel<ValueType>& V) {
            stream << "atomic " << *(dynamic_cast< Image::Voxel< Atomic::BufferScratch<ValueType> >*>(&V));
            return stream;
          }

          std::string save (const std::string& filename) const 
          {
            Atomic::Voxel<value_type> in (*this);
            Image::Header header;
            header.info() = Image::Voxel< Atomic::BufferScratch<ValueType> >::info();
            Image::Buffer<value_type> buffer_out (filename, header);
            typename Image::Buffer<value_type>::voxel_type out (buffer_out);
            Image::threaded_copy (in, out);
            return buffer_out.__get_handler()->files[0].name;
          }


        protected:

          using Image::Voxel< Atomic::BufferScratch<ValueType> >::data_;
          using Image::Voxel< Atomic::BufferScratch<ValueType> >::offset_;

          value_type get_value () const {
            return data_.get_value (offset_);
          }

          template <class Functor>
          void update_value (const Functor& functor) {
            data_.update_value (functor, offset_);
          }

          friend class Atomic::Value<ValueType>;
      };


    }
  }
}

#endif




