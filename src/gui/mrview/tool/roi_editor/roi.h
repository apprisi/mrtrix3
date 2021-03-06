/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 2014.

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

#ifndef __gui_mrview_tool_roi_editor_roi_h__
#define __gui_mrview_tool_roi_editor_roi_h__

#include <vector>

#include "memory.h"
#include "image/header.h"
#include "image/transform.h"

#include "gui/mrview/mode/base.h"
#include "gui/mrview/tool/base.h"
#include "gui/mrview/mode/slice.h"
#include "gui/color_button.h"
#include "gui/mrview/adjust_button.h"

#include "gui/mrview/tool/roi_editor/item.h"
#include "gui/mrview/tool/roi_editor/model.h"
#include "gui/mrview/tool/roi_editor/undoentry.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


        class ROI : public Base
        {
            Q_OBJECT

          public:
            ROI (Window& main_window, Dock* parent);
            ~ROI();

            void draw (const Projection& projection, bool is_3D, int axis, int slice);

            static void add_commandline_options (MR::App::OptionList& options);
            virtual bool process_commandline_option (const MR::App::ParsedOption& opt);

            virtual bool mouse_press_event ();
            virtual bool mouse_move_event ();
            virtual bool mouse_release_event ();
            virtual QCursor* get_cursor ();

          private slots:
            void new_slot ();
            void open_slot ();
            void save_slot ();
            void close_slot ();
            void draw_slot ();
            void undo_slot ();
            void redo_slot ();
            void hide_all_slot ();
            void slice_copy_slot (QAction*);
            void select_edit_mode (QAction*);
            void toggle_shown_slot (const QModelIndex&, const QModelIndex&);
            void update_selection ();
            void update_slot ();
            void colour_changed ();
            void opacity_changed (int unused);

          protected:
             QPushButton *hide_all_button, *close_button, *save_button;
             QToolButton *draw_button, *undo_button, *redo_button;
             QToolButton *brush_button, *rectangle_button, *fill_button;
             QToolButton *copy_from_above_button, *copy_from_below_button;
             QActionGroup *edit_mode_group, *slice_copy_group;
             ROI_Model* list_model;
             QListView* list_view;
             QColorButton* colour_button;
             QSlider *opacity_slider;
             AdjustButton *brush_size_button;
             int current_axis, current_slice;
             bool in_insert_mode, insert_mode_value;
             Point<> current_origin, prev_pos;
             float current_slice_loc;

             Mode::Slice::Shader shader;

             void update_undo_redo ();
             void updateGL() { 
               window.get_current_mode()->update_overlays = true;
               window.updateGL();
             }
             
             void load (std::vector<std::unique_ptr<MR::Image::Header>>& list); 
             void save (ROI_Item*);

             int normal2axis (const Point<>&, const MR::Image::Transform&) const;
        };


      }
    }
  }
}

#endif



