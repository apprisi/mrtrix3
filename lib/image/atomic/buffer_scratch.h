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

#ifndef __image_atomic_buffer_scratch_h__
#define __image_atomic_buffer_scratch_h__

#include "get_set.h"
#include "image/buffer_scratch.h"
#include "image/info.h"


namespace MR
{
  namespace Image
  {
    namespace Atomic
    {


    template <typename ValueType>
      class BufferScratch : protected Image::BufferScratch
    {
      public:
        template <class Template>
        BufferScratch (const Template& info, const std::memory_order order = std::memory_order_seq_cst) :
            Image::BufferScratch (info),
            mem_order (order)) { }

        template <class Template>
        BufferScratch (const Template& info, const std::string& label, const std::memory_order order = std::memory_order_seq_cst) :
            Image::BufferScratch (info, label),
            mem_order (order)) { }

        typedef ValueType value_type;
        typedef Atomic::Voxel voxel_type;

        voxel_type voxel() { return voxel_type (*this); }

        value_type get_value (size_t index) const
        {
          return std::atomic_load_explicit (reinterpret_cast< std::atomic<value_type> >(data_[index]), mem_order);
        }

        template <class Functor>
        void update_value (const Functor& functor, size_t index)
        {
          value_type value = std::atomic_load_explicit (reinterpret_cast< std::atomic<value_type> >(data_[index]), std::memory_order_relaxed);
          while (std::atomic_compare_exchange_weak_explicit (&reinterpret_cast< std::atomic<value_type> >(data_[index]), &value, functor (value), mem_order, std::memory_order_relaxed));
        }

        friend std::ostream& operator<< (std::ostream& stream, const Atomic::BufferScratch& V) {
          stream << "atomic " << *(dynamic_cast<Image::BufferScratch*>(this));
          return stream;
        }

      protected:
        const std::memory_order mem_order;

        template <class InfoType>
          BufferScratch& operator= (const InfoType&) { assert (0); return *this; }

        BufferScratch (const BufferScratch& that) : Image::BufferScratch (that), mem_order (std::memory_order_seq_cst) { assert (0); }
    };





    template <>
    class BufferScratch<bool> : protected Image::BufferScratch<bool>
    {
      public:
        template <class Template>
        BufferScratch (const Template& info, const std::memory_order order = std::memory_order_seq_cst) :
            Image::BufferScratch<bool> (info),
            mem_order (order)) { }

        template <class Template>
        BufferScratch (const Template& info, const std::string& label, const std::memory_order order = std::memory_order_seq_cst) :
            Image::BufferScratch<bool> (info, label),
            mem_order (order)) { }

        typedef Atomic::Voxel<bool> voxel_type;

        voxel_type voxel() { return voxel_type (*this); }

        // TODO Necessary to make this thread-safe?
        void zero () {
          memset (data_, 0x00, bytes_);
        }

        bool get_value (size_t index) const
        {
          const uint8_t byte = std::atomic_load_explicit (reinterpret_cast< std::atomic<value_type> >(data_[index>>3]), mem_order);
          return (byte & (BITMASK >> (index&7)));
        }

        template <class Functor>
        void update_value (const Functor& functor, size_t index)
        {
          std::atomic<uint8_t>* byte = reinterpret_cast<std::atomic<uint8_t>*> (data_ + (index>>3));
          uint8_t prev_value = *byte, new_value;
          do {
            if (functor (prev_value))
              new_value = prev_value |  (BITMASK >> i&7);
            else
              new_value = prev_value & ~(BITMASK >> i&7);
          } while (!byte->compare_exchange_weak (&prev_value, new_value, mem_order));
        }

        friend std::ostream& operator<< (std::ostream& stream, const BufferScratch& V) {
          stream << "atomic " << *(dynamic_cast<Image::BufferScratch<bool>*>(this));
          return stream;
        }

      protected:
        const std::memory_order mem_order;

        template <class InfoType>
          BufferScratch& operator= (const InfoType&) { assert (0); return *this; }

        BufferScratch (const BufferScratch& that) : Image::BufferScratch<bool> (that), mem_order (std::memory_order_seq_cst) { assert (0); }

    };



    }
  }
}

#endif




