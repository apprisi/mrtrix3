/*
   Copyright 2013 Brain Research Institute, Melbourne, Australia

   Written by David Raffelt 10/04/2013

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

#include "mrtrix.h"
#include "gui/mrview/window.h"
#include "gui/mrview/mode/base.h"
#include "gui/mrview/tool/screen_capture.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        Capture::Capture (Window& main_window, Dock* parent) :
          Base (main_window, parent),
          rotation_type(RotationType::World),
          translation_type(TranslationType::Voxel)
        {
          VBoxLayout* main_box = new VBoxLayout (this);

          QGroupBox* rotate_group_box = new QGroupBox (tr("Rotate"));
          GridLayout* rotate_layout = new GridLayout;
          rotate_layout->setContentsMargins (5, 5, 5, 5);
          rotate_layout->setSpacing (5);
          main_box->addWidget (rotate_group_box);
          rotate_group_box->setLayout (rotate_layout);

          rotation_type_combobox = new QComboBox;
          rotation_type_combobox->insertItem (0, tr("World"), RotationType::World);
          rotation_type_combobox->insertItem (1, tr("Camera"), RotationType::Eye);
          connect (rotation_type_combobox, SIGNAL (activated(int)), this, SLOT (on_rotation_type(int)));
          rotate_layout->addWidget(rotation_type_combobox, 0, 1, 1, 4);

          rotate_layout->addWidget (new QLabel (tr("Axis: ")), 1, 0);
          rotation_axis_x = new AdjustButton (this);
          rotate_layout->addWidget (rotation_axis_x, 1, 1);
          rotation_axis_x->setValue (0.0);
          rotation_axis_x->setRate (0.1);

          rotation_axis_y = new AdjustButton (this);
          rotate_layout->addWidget (rotation_axis_y, 1, 2);
          rotation_axis_y->setValue (0.0);
          rotation_axis_y->setRate (0.1);

          rotation_axis_z = new AdjustButton (this);
          rotate_layout->addWidget (rotation_axis_z, 1, 3);
          rotation_axis_z->setValue (1.0);
          rotation_axis_z->setRate (0.1);

          rotate_layout->addWidget (new QLabel (tr("Angle: ")), 2, 0);
          degrees_button = new AdjustButton (this);
          rotate_layout->addWidget (degrees_button, 2, 1, 1, 3);
          degrees_button->setValue (0.0);
          degrees_button->setRate (0.1);

          QGroupBox* translate_group_box = new QGroupBox (tr("Translate"));
          GridLayout* translate_layout = new GridLayout;
          translate_layout->setContentsMargins (5, 5, 5, 5);
          translate_layout->setSpacing (5);
          main_box->addWidget (translate_group_box);
          translate_group_box->setLayout (translate_layout);

          translation_type_combobox = new QComboBox;
          translation_type_combobox->insertItem (0, tr("Voxel"), TranslationType::Voxel);
          translation_type_combobox->insertItem (1, tr("Scanner"), TranslationType::Scanner);
          connect (translation_type_combobox, SIGNAL (activated(int)), this, SLOT (on_translation_type(int)));
          translate_layout->addWidget(translation_type_combobox, 0, 1, 1, 4);

          translate_layout->addWidget (new QLabel (tr("Axis: ")), 1, 0);
          translate_x = new AdjustButton (this);
          translate_layout->addWidget (translate_x, 1, 1);
          translate_x->setValue (0.0);
          translate_x->setRate (0.1);

          translate_y = new AdjustButton (this);
          translate_layout->addWidget (translate_y, 1, 2);
          translate_y->setValue (0.0);
          translate_y->setRate (0.1);

          translate_z = new AdjustButton (this);
          translate_layout->addWidget (translate_z, 1, 3);
          translate_z->setValue (0.0);
          translate_z->setRate (0.1);

          QGroupBox* volume_group_box = new QGroupBox (tr("Volume"));
          GridLayout* volume_layout = new GridLayout;
          volume_layout->setContentsMargins (5, 5, 5, 5);
          volume_layout->setSpacing (5);
          main_box->addWidget (volume_group_box);
          volume_group_box->setLayout (volume_layout);

          volume_layout->addWidget (new QLabel (tr("Axis: ")), 0, 0);
          volume_axis = new QSpinBox (this);
          volume_axis->setMinimum (3);
          volume_axis->setValue (3);
          volume_layout->addWidget (volume_axis, 0, 1);

          volume_layout->addWidget (new QLabel (tr("Target: ")), 0, 2);
          target_volume = new QSpinBox (this);
          volume_layout->addWidget (target_volume, 0, 3);
          target_volume->setMinimum (0);
          target_volume->setMaximum (std::numeric_limits<int>::max());
          target_volume->setValue (0);

          QGroupBox* FOV_group_box = new QGroupBox (tr("FOV"));
          GridLayout* FOV_layout = new GridLayout;
          FOV_layout->setContentsMargins (5, 5, 5, 5);
          FOV_layout->setSpacing (5);
          main_box->addWidget (FOV_group_box);
          FOV_group_box->setLayout (FOV_layout);

          FOV_layout->addWidget (new QLabel (tr("Multiplier: ")), 0, 0);
          FOV_multipler = new AdjustButton (this);
          FOV_layout->addWidget (FOV_multipler, 0, 1);
          FOV_multipler->setValue (1.0);
          FOV_multipler->setRate (0.01);

          QGroupBox* output_group_box = new QGroupBox (tr("Output"));
          main_box->addWidget (output_group_box);
          GridLayout* output_grid_layout = new GridLayout;
          output_group_box->setLayout (output_grid_layout);

          output_grid_layout->addWidget (new QLabel (tr("Prefix: ")), 0, 0);
          prefix_textbox = new QLineEdit ("screenshot", this);
          output_grid_layout->addWidget (prefix_textbox, 0, 1);
          connect (prefix_textbox, SIGNAL (textChanged(const QString&)), this, SLOT (on_output_update()));

          folder_button = new QPushButton (tr("Select output folder"), this);
          folder_button->setToolTip (tr ("Output folder"));
          connect (folder_button, SIGNAL (clicked()), this, SLOT (select_output_folder_slot ()));
          output_grid_layout->addWidget (folder_button, 1, 0, 1, 2);

          QGroupBox* capture_group_box = new QGroupBox (tr("Capture"));
          main_box->addWidget (capture_group_box);
          GridLayout* capture_grid_layout = new GridLayout;
          capture_group_box->setLayout (capture_grid_layout);

          capture_grid_layout->addWidget (new QLabel (tr("Start Index: ")), 0, 0);
          start_index = new QSpinBox (this);
          start_index->setMinimum (0);
          start_index->setMaximum (std::numeric_limits<int>::max());
          start_index->setMinimumWidth(50);
          start_index->setValue (0);
          capture_grid_layout->addWidget (start_index, 0, 1);

          capture_grid_layout->addWidget (new QLabel (tr("Frames: ")), 0, 2);
          frames = new QSpinBox (this);
          frames->setMinimumWidth(50);
          frames->setMinimum (1);
          frames->setMaximum (std::numeric_limits<int>::max());
          frames->setValue (1);
          capture_grid_layout->addWidget (frames, 0, 3);


          QPushButton* preview = new QPushButton (this);
          preview->setToolTip(tr("Preview play"));
          preview->setIcon(QIcon (":/play.svg"));
          connect (preview, SIGNAL (clicked()), this, SLOT (on_screen_preview()));
          capture_grid_layout->addWidget (preview, 2, 0);

          QPushButton* stop = new QPushButton (this);
          stop->setToolTip (tr ("Stop"));
          stop->setIcon(QIcon (":/stop.svg"));
          connect (stop, SIGNAL (clicked()), this, SLOT (on_screen_stop()));
          capture_grid_layout->addWidget (stop, 2, 1);

          QPushButton* restore = new QPushButton (this);
          restore->setToolTip (tr ("Restore"));
          restore->setIcon(QIcon (":/restore.svg"));
          connect (restore, SIGNAL (clicked()), this, SLOT (on_restore_capture_state()));
          capture_grid_layout->addWidget (restore, 2, 2);

          QPushButton* capture = new QPushButton (this);
          capture->setToolTip (tr ("Record"));
          capture->setIcon(QIcon (":/record.svg"));
          connect (capture, SIGNAL (clicked()), this, SLOT (on_screen_capture()));
          capture_grid_layout->addWidget (capture, 2, 3);

          main_box->addStretch ();

          directory = new QDir();

          connect (&window, SIGNAL (imageChanged()), this, SLOT (on_image_changed()));
          on_image_changed();
        }




        void Capture::on_image_changed() {
          cached_state.clear();
          const auto image = window.image();
          if(!image) return;

          Image::VoxelType& vox (image->interp);
          int max_axis = std::max((int)vox.ndim() - 1, 0);
          volume_axis->setMaximum(max_axis);
          volume_axis->setValue(std::min(volume_axis->value(), max_axis));
        }




        void Capture::on_rotation_type(int index) {
          rotation_type = static_cast<RotationType>(rotation_type_combobox->itemData(index).toInt());
        }

        void Capture::on_translation_type(int index) {
          translation_type = static_cast<TranslationType>(translation_type_combobox->itemData(index).toInt());
        }



        void Capture::on_screen_preview () { if(!is_playing) run (false); }

        void Capture::on_screen_capture () { if(!is_playing) run (true); }

        void Capture::on_screen_stop () { is_playing = false; }


        void Capture::cache_capture_state()
        {
            if (!window.image())
              return;
            Image::VoxelType& vox (window.image()->interp);

            cached_state.emplace( cached_state.end(),
              window.orientation(), window.focus(), window.target(), window.FOV(),
              volume_axis->value() < ssize_t (vox.ndim()) ? vox[volume_axis->value()] : 0,
              volume_axis->value(), start_index->value(), window.plane()
            );

            if(cached_state.size() > max_cache_size)
              cached_state.pop_front();
        }




        void Capture::on_restore_capture_state()
        {
            if (!window.image() || !cached_state.size())
              return;

            const CaptureState& state = cached_state.back();

            window.set_plane(state.plane);
            window.set_orientation(state.orientation);
            window.set_focus(state.focus);
            window.set_target(state.target);
            window.set_FOV(state.fov);
            window.set_image_volume(state.volume_axis, state.volume);
            start_index->setValue(state.frame_index);

            cached_state.pop_back();
        }




        void Capture::run (bool with_capture)
        {
          if (!window.image())
            return;

          is_playing = true;

          cache_capture_state();

          Image::VoxelType& vox (window.image()->interp);

          if (std::isnan (rotation_axis_x->value()))
            rotation_axis_x->setValue (0.0);
          if (std::isnan (rotation_axis_y->value()))
            rotation_axis_y->setValue (0.0);
          if (std::isnan (rotation_axis_z->value()))
            rotation_axis_z->setValue (0.0);
          if (std::isnan (degrees_button->value()))
            degrees_button->setValue (0.0);

          if (std::isnan (translate_x->value()))
            translate_x->setValue(0.0);
          if (std::isnan (translate_y->value()))
            translate_y->setValue(0.0);
          if (std::isnan (translate_z->value()))
            translate_z->setValue(0.0);

          if (std::isnan (target_volume->value()))
            target_volume->setValue(0.0);

          if (std::isnan (FOV_multipler->value()))
            FOV_multipler->setValue(1.0);

          if (window.snap_to_image () && degrees_button->value() > 0.0)
            window.set_snap_to_image (false);


          size_t frames_value = frames->value();

          std::string folder (directory->path().toUtf8().constData());
          std::string prefix (prefix_textbox->text().toUtf8().constData());
          float radians = degrees_button->value() * (Math::pi / 180.0) / frames_value;
          size_t first_index = start_index->value();

          float volume = 0, volume_inc = 0;
          if (volume_axis->value() < ssize_t (vox.ndim())) {
            if (target_volume->value() >= vox.dim(volume_axis->value()))
              target_volume->setValue (vox.dim(volume_axis->value())-1);
            volume = vox[volume_axis->value()];
            volume_inc = target_volume->value() / (float)frames_value;
          }

          size_t i = first_index;
          do {
            if (!is_playing)
              break;

            if (with_capture)
              this->window.captureGL (folder + "/" + prefix + printf ("%04d.png", i));

            // Rotation
            Math::Versor<float> orientation (this->window.orientation());
            Math::Vector<float> axis (3);
            axis[0] = rotation_axis_x->value();
            axis[1] = rotation_axis_y->value();
            axis[2] = rotation_axis_z->value();
            Math::Versor<float> rotation (radians, axis.ptr());

            switch (rotation_type) {
              case RotationType::World:
                orientation = rotation*orientation;
                break;
              case RotationType::Eye:
                orientation *= rotation;
                break;
              default:
                break;
            }

            this->window.set_orientation (orientation);

            // Translation
            Point<float> trans_vec(translate_x->value(), translate_y->value(), translate_z->value());
            trans_vec /= frames_value;
            if(translation_type == TranslationType::Voxel)
              trans_vec = window.image()->interp.voxel2scanner_dir(trans_vec);

            Point<float> focus (this->window.focus());
            focus += trans_vec;
            window.set_focus (focus);
            Point<float> target (this->window.target());
            target += trans_vec;
            window.set_target (target);

            // Volume
            if (volume_axis->value() < ssize_t (vox.ndim())) {
              volume += volume_inc;
              window.set_image_volume (volume_axis->value(), std::round(volume));
            }

            // FOV
            window.set_FOV (window.FOV() * (std::pow (FOV_multipler->value(), (float) 1.0 / frames_value)));

            start_index->setValue (i + 1);
            this->window.updateGL();
            qApp->processEvents();

            i+=1;
          } while((i % frames_value));

          is_playing = false;
        }






        void Capture::select_output_folder_slot ()
        {
          directory->setPath(QFileDialog::getExistingDirectory (this, tr("Directory"), directory->path()));
          QString path (shorten(directory->path().toUtf8().constData(), 20, 0).c_str());
          if (!path.size()) return;
          folder_button->setText(path);
          on_output_update ();
        }






        void Capture::on_output_update () {
          start_index->setValue (0);
        }








        void Capture::add_commandline_options (MR::App::OptionList& options) 
        { 
          using namespace MR::App;
          options
            + OptionGroup ("Screen Capture tool options")

            + Option ("capture.folder", "Set the output folder for the screen capture tool.")
            +   Argument ("path").type_text()

            + Option ("capture.prefix", "Set the output file prefix for the screen capture tool.")
            +   Argument ("string").type_text()

            + Option ("capture.grab", "Start the screen capture process.");
        }

        bool Capture::process_commandline_option (const MR::App::ParsedOption& opt) 
        {
          if (opt.opt->is ("capture.folder")) {
            directory->setPath (std::string(opt[0]).c_str());
            QString path (shorten(directory->path().toUtf8().constData(), 20, 0).c_str());
            folder_button->setText(path);
            on_output_update ();
            return true;
          }

          if (opt.opt->is ("capture.prefix")) {
            prefix_textbox->setText (std::string(opt[0]).c_str());
            on_output_update ();
            return true;
          }

          if (opt.opt->is ("capture.grab")) {
            this->window.updateGL();
            qApp->processEvents();
            on_screen_capture();
            return true;
          }

          return false;
        }


      }
    }
  }
}






