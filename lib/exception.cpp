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

#include "app.h"
#include "exception.h"
#include "file/config.h"
#include "debug.h"

#ifdef MRTRIX_AS_R_LIBRARY
# include "wrap_r.h"
#endif


namespace MR
{

  void display_exception_cmdline (const Exception& E, int log_level)
  {
    if (App::log_level >= log_level) 
      for (size_t n = 0; n < E.description.size(); ++n) 
        report_to_user_func (E.description[n], log_level);
  }

  off_t __stderr_offset = 0;


  namespace {

    inline const char* console_prefix (int type) 
    { 
      switch (type) {
        case 0: return "[ERROR] ";
        case 1: return "[WARNING] ";
        case 2: return "[INFO] ";
        case 3: return "[DEBUG] ";
        default: return "";
      }
    }

  }

  void cmdline_report_to_user_func (const std::string& msg, int type)
  {
    static constexpr const char* colour_format_strings[] = {
      "%s: %s%s\n",
      "%s: \033[01;31m%s%s\033[0m\n",
      "%s: \033[00;31m%s%s\033[0m\n",
      "%s: \033[03;32m%s%s\033[0m\n",
      "%s: \033[03;34m%s%s\033[0m\n"
    };

    if (__stderr_offset) {
      __print_stderr ("\n");
      __stderr_offset = 0;
    }

    auto clamp = [](int t) { if (t < -1 || t > 3) t = -1; return t+1; };

    __print_stderr (printf (colour_format_strings[App::terminal_use_colour ? clamp(type) : 0], 
          App::NAME.c_str(), console_prefix (type), msg.c_str()));
    if (type == 1 && App::fail_on_warn)
      throw Exception ("terminating due to request to fail on warning");
  }



  void cmdline_print_func (const std::string& msg)
  {
#ifdef MRTRIX_AS_R_LIBRARY
    Rprintf (msg.c_str());
#else 
    std::cout << msg;
#endif
  }




  void (*print) (const std::string& msg) = cmdline_print_func;
  void (*report_to_user_func) (const std::string& msg, int type) = cmdline_report_to_user_func;
  void (*Exception::display_func) (const Exception& E, int log_level) = display_exception_cmdline;

}

