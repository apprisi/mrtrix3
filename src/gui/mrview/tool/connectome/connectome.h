/*
   Copyright 2014 Brain Research Institute, Melbourne, Australia

   Written by Robert E. Smith, 2015.

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

#ifndef __gui_mrview_tool_connectome_connectome_h__
#define __gui_mrview_tool_connectome_connectome_h__

#include <map>
#include <vector>

#include "point.h"
#include "ptr.h"

#include "gui/opengl/gl.h"
#include "gui/opengl/lighting.h"
#include "gui/opengl/shader.h"
#include "gui/mrview/adjust_button.h"
#include "gui/mrview/colourmap_button.h"
#include "gui/mrview/image.h"
#include "gui/mrview/mode/base.h"
#include "gui/mrview/tool/base.h"
#include "gui/shapes/cube.h"
#include "gui/shapes/cylinder.h"
#include "gui/shapes/sphere.h"
#include "gui/color_button.h"
#include "gui/projection.h"

#include "image/buffer_preload.h"
#include "image/buffer_scratch.h"

#include "mesh/mesh.h"

#include "connectome/mat2vec.h"

#include "dwi/tractography/connectomics/config.h"
#include "dwi/tractography/connectomics/connectomics.h"
#include "dwi/tractography/connectomics/lut.h"

#include "gui/mrview/tool/connectome/colourmap_observers.h"
#include "gui/mrview/tool/connectome/edge.h"
#include "gui/mrview/tool/connectome/file_data_vector.h"
#include "gui/mrview/tool/connectome/node.h"
#include "gui/mrview/tool/connectome/node_overlay.h"
#include "gui/mrview/tool/connectome/shaders.h"




// TODO Elements that still need to be added to the Connectome tool:
//
// * Drawing edges
//   - Display options:
//     * Colour by: Corresponding nodes? Would require some plotting cleverness
//   - Draw as streamlines
//     This would probably require some cross-talk with the tractography tool,
//     but the theory would essentially be:
//     * Read in a tractogram, assigning streamlines to node pairs; and for
//       every edge in the connectome, build a mean exemplar streamline path,
//       constrain to start & end at the node centres
//     * Re-sample the resulting exemplars to an appropriate step size, and
//       store this as an entry in the tractography tool
//     * The Connectome tool would retain access to streamline lengths etc., so
//       that it can write to the scalar & colour buffers
//     Alternatively this could be done by drawing actual streamtubes:
//     use the streamline tangent at each point to rotate a hollow cylinder
//
//
// * Drawing nodes
//   - When in 2D mode, mesh mode should detect triangles intersecting with the
//     focus plane, and draw the projections on that plane as lines
//     (preferably with adjustable thickness)
//     This may be best handled within a geometry shader: Detect that polygon
//     intersects viewing plane, emit two vertices, draw with GL_LINES
//     (shader will need access to the following two vertices, and run on only
//     every third vertex)
//   - Drawing as overlay: Volume render seems to work, but doesn't always update immediately
//   - In 2D mode, use mask image extent / node location & size to detect when there is no
//     need to process a particular node (need to save the image extent from construction)
//   - Drawing as spheres
//     * May be desirable in some instances to symmetrize the node centre-of-mass positions...?
//     * When in 2D mode, as with mesh mode, detect triangles intersecting with the viewing
//       plane and draw as lines
//     * Rename current Shapes::Sphere class to Halfsphere, use in the DWI renderer,
//       but derive a full-sphere for use here
//       (no point using a full-sphere in the DWI renderer as you'd still have the issue of negative lobes)
//   - Draw as points
//   - Meshes
//     * Get right hand rule working, use face culling
//     * Have both original and smoothed meshes available from the drop-down list
//     * Only mesh nodes when it is necessary to do so (i.e. user has selected that option),
//       rather than meshing them all at image load
//   - Drawing as cubes: Instead of relying on flat normals, just duplicate the vertices
//     and store normals for each; keep things simple
//     (leave this until necessary, i.e. trying to do a full polygon depth search)
//
// * OpenGL drawing general:
//   - Solve the 'QWidget::repaint: Recursive repaint detected' issue
//     (arose with implementation of the node overlay image)
//   - Make transparency sliders a little more sensible
//     (may need linear scale in 2D mode, non-linear in 3D)
//     Think this only applies to the volume render
//   - Consider generating all polygonal geometry, store in a vector, sort by camera distance,
//     update index vector accordingly, do a single draw call for both edges and nodes
//     (this is the only way transparency of both nodes and edges can work)
//
// * Nodes GUI section
//   - Prevent other non-sensible behaviour, e.g.:
//     * Trying to colour by LUT when no LUT is provided
//   - Implement list view with list of nodes, enable manual manupulation of nodes
//
// * Toolbar load
//   - Speed up the node tessellation; either doing it all in a single pass, or multi-threading
//
// * Toolbar
//   - Figure out why the toolbar is initially being drawn twice
//     -> May be something to do with dual screen...
//   - Add lighting checkbox; need to be able to take screenshots with quantitative colour mapping
//   - Implement check for determining when the shader needs to be updated
//   - Disable LUT, config file options and all visualisation options if no image is loaded
//   - Enable collapsing of node / edge visualisation groups; will make room for future additions
//
// * Additional functionalities:
//   - Print node name in the GL window
//     How to get access to shorter node names? Rely on user making a new LUT?
//   - External window with capability of showing bar plots for different node parameters,
//     clicking on a node in the main GL window highlights that node in those external plots






namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        class Connectome : public Base
        {
            Q_OBJECT

            typedef MR::DWI::Tractography::Connectomics::node_t    node_t;
            typedef MR::DWI::Tractography::Connectomics::Node_info Node_info;
            typedef MR::DWI::Tractography::Connectomics::Node_map  Node_map;

            enum node_geometry_t   { NODE_GEOM_SPHERE, NODE_GEOM_CUBE, NODE_GEOM_OVERLAY, NODE_GEOM_MESH, NODE_GEOM_SMOOTH_MESH };
            enum node_colour_t     { NODE_COLOUR_FIXED, NODE_COLOUR_RANDOM, NODE_COLOUR_LUT, NODE_COLOUR_FILE };
            enum node_size_t       { NODE_SIZE_FIXED, NODE_SIZE_VOLUME, NODE_SIZE_FILE };
            enum node_visibility_t { NODE_VIS_ALL, NODE_VIS_NONE, NODE_VIS_FILE, NODE_VIS_DEGREE };
            enum node_alpha_t      { NODE_ALPHA_FIXED, NODE_ALPHA_LUT, NODE_ALPHA_FILE };

            enum edge_geometry_t   { EDGE_GEOM_LINE, EDGE_GEOM_CYLINDER };
            enum edge_colour_t     { EDGE_COLOUR_FIXED, EDGE_COLOUR_DIR, EDGE_COLOUR_FILE };
            enum edge_size_t       { EDGE_SIZE_FIXED, EDGE_SIZE_FILE };
            enum edge_visibility_t { EDGE_VIS_ALL, EDGE_VIS_NONE, EDGE_VIS_NODES, EDGE_VIS_FILE };
            enum edge_alpha_t      { EDGE_ALPHA_FIXED, EDGE_ALPHA_FILE };

          public:

            Connectome (Window& main_window, Dock* parent);

            virtual ~Connectome ();

            void draw (const Projection& transform, bool is_3D, int axis, int slice);
            void drawOverlays (const Projection& transform) override;
            bool process_batch_command (const std::string& cmd, const std::string& args);

            size_t num_nodes() const { return nodes.size() ? nodes.size() - 1 : 0; }
            size_t num_edges() const { return edges.size(); }

          private slots:
            void image_open_slot ();
            void lut_open_slot (int);
            void config_open_slot ();
            void hide_all_slot ();

            void node_geometry_selection_slot (int);
            void node_colour_selection_slot (int);
            void node_size_selection_slot (int);
            void node_visibility_selection_slot (int);
            void node_alpha_selection_slot (int);

            void sphere_lod_slot (int);
            void overlay_interp_slot (int);
            void node_colour_change_slot();
            void node_colour_parameter_slot();
            void node_size_value_slot();
            void node_size_parameter_slot();
            void node_visibility_parameter_slot();
            void node_alpha_value_slot (int);
            void node_alpha_parameter_slot();

            void edge_geometry_selection_slot (int);
            void edge_colour_selection_slot (int);
            void edge_size_selection_slot (int);
            void edge_visibility_selection_slot (int);
            void edge_alpha_selection_slot (int);

            void cylinder_lod_slot (int);
            void edge_colour_change_slot();
            void edge_colour_parameter_slot();
            void edge_size_value_slot();
            void edge_size_parameter_slot();
            void edge_visibility_parameter_slot();
            void edge_alpha_value_slot (int);
            void edge_alpha_parameter_slot();

          protected:

            QPushButton *image_button, *hide_all_button;
            QComboBox *lut_combobox;
            QPushButton *config_button;

            QComboBox *node_geometry_combobox, *node_colour_combobox, *node_size_combobox, *node_visibility_combobox, *node_alpha_combobox;

            QLabel *node_geometry_sphere_lod_label;
            QSpinBox *node_geometry_sphere_lod_spinbox;
            QCheckBox *node_geometry_overlay_interp_checkbox;

            QColorButton *node_colour_fixedcolour_button;
            ColourMapButton *node_colour_colourmap_button;
            QLabel *node_colour_range_label;
            AdjustButton *node_colour_lower_button, *node_colour_upper_button;

            AdjustButton *node_size_button;
            QLabel *node_size_range_label;
            AdjustButton *node_size_lower_button, *node_size_upper_button;
            QCheckBox *node_size_invert_checkbox;

            QLabel *node_visibility_threshold_label;
            AdjustButton *node_visibility_threshold_button;
            QCheckBox *node_visibility_threshold_invert_checkbox;

            QSlider *node_alpha_slider;
            QLabel *node_alpha_range_label;
            AdjustButton *node_alpha_lower_button, *node_alpha_upper_button;
            QCheckBox *node_alpha_invert_checkbox;

            QComboBox *edge_geometry_combobox, *edge_colour_combobox, *edge_size_combobox, *edge_visibility_combobox, *edge_alpha_combobox;

            QLabel *edge_geometry_cylinder_lod_label;
            QSpinBox *edge_geometry_cylinder_lod_spinbox;

            QColorButton *edge_colour_fixedcolour_button;
            ColourMapButton *edge_colour_colourmap_button;
            QLabel *edge_colour_range_label;
            AdjustButton *edge_colour_lower_button, *edge_colour_upper_button;

            AdjustButton *edge_size_button;
            QLabel *edge_size_range_label;
            AdjustButton *edge_size_lower_button, *edge_size_upper_button;
            QCheckBox *edge_size_invert_checkbox;

            QLabel *edge_visibility_threshold_label;
            AdjustButton *edge_visibility_threshold_button;
            QCheckBox *edge_visibility_threshold_invert_checkbox;

            QSlider *edge_alpha_slider;
            QLabel *edge_alpha_range_label;
            AdjustButton *edge_alpha_lower_button, *edge_alpha_upper_button;
            QCheckBox *edge_alpha_invert_checkbox;

          private:

            NodeShader node_shader;
            EdgeShader edge_shader;

            // For the sake of viewing nodes as an overlay, need to ALWAYS
            // have access to the parcellation image
            Ptr< MR::Image::BufferPreload<node_t> > buffer;


            std::vector<Node> nodes;
            std::vector<Edge> edges;


            // For converting connectome matrices to vectors
            MR::Connectome::Mat2Vec mat2vec;


            // If a lookup table is provided, this container will store the
            //   properties of each node as provided in that file (e.g. name & colour)
            Node_map lut;

            // If a connectome configuration file is provided, this will map
            //   each structure name to an index in the parcellation image;
            //   this can then be used to produce the lookup table
            std::vector<std::string> config;

            // If both a LUT and a config file have been provided, this provides
            //   a direct vector mapping from image node index to a position in
            //   the lookup table, pre-generated
            std::vector<Node_map::const_iterator> lut_mapping;


            // Used when the geometry of node visualisation is a sphere
            Shapes::Sphere sphere;
            GL::VertexArrayObject sphere_VAO;

            // Used when the geometry of node visualisation is a cube
            Shapes::Cube cube;
            GL::VertexArrayObject cube_VAO;

            // Used when the geometry of node visualisation is an image overlay
            Ptr<NodeOverlay> node_overlay;

            // Used when the geometry of edge visualisation is a cylinder
            Shapes::Cylinder cylinder;
            GL::VertexArrayObject cylinder_VAO;


            // Fixed lighting settings from the main window
            const GL::Lighting& lighting;


            // Current node visualisation settings
            node_geometry_t node_geometry;
            node_colour_t node_colour;
            node_size_t node_size;
            node_visibility_t node_visibility;
            node_alpha_t node_alpha;

            // Other values that need to be stored w.r.t. node visualisation
            bool have_meshes, have_smooth_meshes;
            Point<float> node_fixed_colour;
            size_t node_colourmap_index;
            bool node_colourmap_invert;
            float node_fixed_alpha;
            float node_size_scale_factor;
            float voxel_volume;
            FileDataVector node_values_from_file_colour;
            FileDataVector node_values_from_file_size;
            FileDataVector node_values_from_file_visibility;
            FileDataVector node_values_from_file_alpha;


            // Current edge visualisation settings
            edge_geometry_t edge_geometry;
            edge_colour_t edge_colour;
            edge_size_t edge_size;
            edge_visibility_t edge_visibility;
            edge_alpha_t edge_alpha;

            // Other values that need to be stored w.r.t. edge visualisation
            Point<float> edge_fixed_colour;
            size_t edge_colourmap_index;
            bool edge_colourmap_invert;
            float edge_fixed_alpha;
            float edge_size_scale_factor;
            FileDataVector edge_values_from_file_colour;
            FileDataVector edge_values_from_file_size;
            FileDataVector edge_values_from_file_visibility;
            FileDataVector edge_values_from_file_alpha;


            // Classes to receive inputs from the colourmap buttons and act accordingly
            NodeColourObserver node_colourmap_observer;
            EdgeColourObserver edge_colourmap_observer;


            // Helper functions
            void clear_all();
            void initialise (const std::string&);

            void import_file_for_node_property (FileDataVector&, const std::string&);
            void import_file_for_edge_property (FileDataVector&, const std::string&);

            void load_properties();

            void calculate_node_colours();
            void calculate_node_sizes();
            void calculate_node_visibility();
            void calculate_node_alphas();

            void update_node_overlay();

            void calculate_edge_colours();
            void calculate_edge_sizes();
            void calculate_edge_visibility();
            void calculate_edge_alphas();

            friend class NodeColourObserver;
            friend class EdgeColourObserver;

            friend class NodeShader;
            friend class EdgeShader;

        };








      }
    }
  }
}

#endif



