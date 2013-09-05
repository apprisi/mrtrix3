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

#include <unistd.h>
#include <fcntl.h>
#include <fstream>

#include "datatype.h"
#include "image/stride.h"
#include "types.h"
#include "file/utils.h"
#include "file/entry.h"
#include "file/path.h"
#include "file/key_value.h"
#include "image/utils.h"
#include "image/header.h"
#include "image/handler/sparse.h"
#include "image/name_parser.h"
#include "image/format/list.h"
#include "image/format/mrtrix_utils.h"
#include "image/sparse/keys.h"




namespace MR
{
  namespace Image
  {
    namespace Format
    {

      // extensions are:
      // msih: MRtrix Sparse Image Header
      // msif: MRtrix Sparse Image File

      RefPtr<Handler::Base> MRtrix_sparse::read (Header& H) const
      {

        if (!Path::has_suffix (H.name(), ".msih") && !Path::has_suffix (H.name(), ".msif"))
          return RefPtr<Handler::Base>();

        File::KeyValue kv (H.name(), "mrtrix sparse image");

        read_mrtrix_header (H, kv);

        // Although the endianness of the image data itself (the sparse data offsets) actually doesn't matter
        //   (the Image::Buffer<> class would deal with this conversion), the sparse data itself needs to have
        //   the correct endianness for the system. Since MRtrix_sparse::create() forces the endianness of the
        //   offset data to be native, this is the easiest way to verify that the sparse data also has the
        //   correct endianness
#ifdef BYTE_ORDER_BIG_ENDIAN
        const DataType dt = DataType::UInt64BE;
#else
        const DataType dt = DataType::UInt64LE;
#endif
        if (H.datatype() != dt)
          throw Exception ("Cannot open sparse image file " + H.name() + " due to type mismatch; expect " + dt.description() + ", file is " + H.datatype().description());

        Header::const_iterator name_it = H.find (Image::Sparse::name_key);
        if (name_it == H.end())
          throw Exception ("sparse data class name not specified in sparse image header " + H.name());

        Header::const_iterator size_it = H.find (Image::Sparse::size_key);
        if (size_it == H.end())
          throw Exception ("sparse data class size not specified in sparse image header " + H.name());

        std::string image_fname, sparse_fname;
        size_t image_offset, sparse_offset;

        get_mrtrix_file_path (H, "file", image_fname, image_offset);

        ParsedName::List image_list;
        std::vector<int> image_num = image_list.parse_scan_check (image_fname);

        get_mrtrix_file_path (H, "sparse_file", sparse_fname, sparse_offset);

        Handler::Default base_handler (H);
        for (size_t n = 0; n < image_list.size(); ++n)
          base_handler.files.push_back (File::Entry (image_list[n].name(), image_offset));

        RefPtr<Handler::Base> handler (new Handler::Sparse (base_handler, name_it->second, to<size_t>(size_it->second), File::Entry (sparse_fname, sparse_offset)));

        return handler;
      }





      bool MRtrix_sparse::check (Header& H, size_t num_axes) const
      {
        if (!Path::has_suffix (H.name(), ".msih") &&
            !Path::has_suffix (H.name(), ".msif"))
          return false;

        if (H.find (Image::Sparse::name_key) == H.end() ||
            H.find (Image::Sparse::size_key) == H.end())
          return false;

        H.set_ndim (num_axes);
        for (size_t i = 0; i < H.ndim(); i++)
          if (H.dim (i) < 1)
            H.dim(i) = 1;

        return true;
      }






      RefPtr<Handler::Base> MRtrix_sparse::create (Header& H) const
      {

        Header::const_iterator name_it = H.find (Image::Sparse::name_key);
        if (name_it == H.end())
          throw Exception ("Cannot create sparse image " + H.name() + "; no knowledge of underlying data class type");

        Header::const_iterator size_it = H.find (Image::Sparse::size_key);
        if (size_it == H.end())
          throw Exception ("Cannot create sparse image " + H.name() + "; no knowledge of underlying data class size");

        H.datatype() = DataType::UInt64;
        H.datatype().set_byte_order_native();

        if (!File::is_tempfile (H.name()))
          File::create (H.name());

        std::ofstream out (H.name().c_str(), std::ios::out | std::ios::binary);
        if (!out)
          throw Exception ("error creating file \"" + H.name() + "\":" + strerror (errno));

        out << "mrtrix sparse image\n";

        write_mrtrix_header (H, out);

        bool single_file = Path::has_suffix (H.name(), ".msif");

        int64_t image_offset = 0, sparse_offset = 0;
        std::string image_path, sparse_path;
        if (single_file) {

          image_offset = out.tellp() + int64_t(54);
          image_offset += ((4 - (image_offset % 4)) % 4);
          sparse_offset = image_offset + Image::footprint(H);

          out << "file: . " << image_offset << "\nsparse_file: . " << sparse_offset << "\nEND\n";

          File::resize (H.name(), sparse_offset);
          image_path = H.name();
          sparse_path = H.name();

        } else {

          image_path  = Path::basename (H.name().substr (0, H.name().size()-5) + ".dat");
          sparse_path = Path::basename (H.name().substr (0, H.name().size()-5) + ".sdat");

          out << "file: " << image_path << "\nsparse_file: " << sparse_path << "\nEND\n";

          File::create (image_path, Image::footprint(H));
          File::create (sparse_path);

        }

        Handler::Default base_handler (H);
        base_handler.files.push_back (File::Entry (image_path, image_offset));

        RefPtr<Handler::Base> handler (new Handler::Sparse (base_handler, name_it->second, to<size_t>(size_it->second), File::Entry (sparse_path, sparse_offset)));

        return handler;
      }





    }
  }
}