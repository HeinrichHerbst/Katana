/**
 * Author:      H.F. Herbst
 * Origin:      Stellenbosch University
 * For:         IARPA SuperTools Project
 * license:     MIT
 * Description: Function definitions for FLOOXS tcl generation
 */
#include "flooxs_gen.hpp"
using namespace std;

int draw_flooxs_file(section_data section_info, ldf ldf_info,
                     const std::string &outpath, const double &db_unit,
                     const double &db_unit_in_m, const int &spacing)
{
  map<int, layer_data> my_layer_info;
  map<int, map<int, building_blocks>> my_blocks;
  map<char, vector<int>> physical_volumes;
  vector<int> layer_order;
  ofstream tcl_file;

  ldf_info.pull_layer_data(my_layer_info);
  section_info.pull_section_blocks(my_blocks);
  section_info.pull_used_layers(layer_order);
  get_layer_order(layer_order, my_layer_info);

  double cross_section_distance =
      (double)section_info.get_sec_dist() * db_unit_in_m; // conversion to um
  double grid_height =
      (double)get_layer_height(my_layer_info, layer_order) * db_unit_in_m;
  tcl_file.open(outpath);

  vector<string> masks;
  vector<int> masked_layers;
  int xsec_dist = section_info.get_sec_dist();
  generate_masks(masks, my_layer_info, my_blocks,
                 layer_order, db_unit_in_m, masked_layers, xsec_dist);
  masked_layers.erase(unique(masked_layers.begin(), masked_layers.end()),
                      masked_layers.end());
  draw_introduction(tcl_file);
  tcl_file << "# Configure grid spacing\n";
  tcl_file << "set spacing " << (double)spacing / 1000 << "\n";
  tcl_file << "set etchspace " << 5e-2 << "\n";
  tcl_file << "set depospace " << 5e-2 << "\n\n";
  tcl_file << "# Initialize vertical grid\n";
  tcl_file << "line x loc=0\tspac=$spacing tag=T1\n";
  tcl_file << "line x loc=" << grid_height << "\tspac=$spacing tag=T2\n";
  tcl_file << "line x loc=" << grid_height + 0.1
           << "\tspac=$spacing tag=T3\n\n";
  tcl_file << "# Initialize horizontal grid\n";
  tcl_file << "line y loc=0\t spac=$spacing tag=S1\n";
  tcl_file << "line y loc=" << cross_section_distance
           << "\tspac=$spacing\ttag=S2\n\n";
  tcl_file << "# Create gas_filled region (reserve space)\n";
  tcl_file << "region\tgas\txlo=T1\txhi=T2\tylo=S1\tyhi=S2\n\n";
  tcl_file << "# Create oxide base. (Must deposit onto something)\n";
  tcl_file << "region\toxide\txlo=T2\txhi=T3\tylo=S1\tyhi=S2\n\n";
  tcl_file << "# Initialize model according to above parameters\n";
  tcl_file << "init\n\n";
  tcl_file << "# Define all mask locations.\n";
  for (auto i = masks.begin(); i != masks.end(); i++) {
    tcl_file << *i;
  }
  tcl_file << "\n";
  tcl_file << "# Set-up viewing window.\n";
  tcl_file << "window\trow=1\tcol=2\twidth=600\theight=600\n\n";
  tcl_file << "# Plot initial view before deposition and etching.\n";
  tcl_file << "plot2d\tgrid\tgas\n\n";
  tcl_file << "# Alternate between depositing and etching\n";
  for (auto i = layer_order.begin(); i != layer_order.end(); i++) {
    tcl_file << "";
    auto location = masked_layers.end();
    switch (my_layer_info[*i].film_type) {
    case 'I':
      tcl_file << "deposit\toxide\ttime=2\tspacing=$depospace\n";
      location = find(masked_layers.begin(), masked_layers.end(), *i);
      if (location != masked_layers.end())
        tcl_file << "etch\toxide\taniso\tmask=layer_" << *i
                 << "\ttime=2.2\tspacing=$etchspace\n";
      tcl_file << "plot2d\tgrid\tgas\n";
      break;
    case 'R':
      tcl_file << "deposit\tsemi\ttime=0.9\tspacing=$depospace\n";
      location = find(masked_layers.begin(), masked_layers.end(), *i);
      if (location != masked_layers.end())
        tcl_file << "etch\tsemi\taniso\tmask=layer_" << *i
                 << "\ttime=1.3\tspacing=$etchspace\n";
      tcl_file << "plot2d\tgrid\tgas\n";
      break;
    case 'S':
      tcl_file << "deposit\tmetal\ttime=2\tspacing=$depospace\n";
      location = find(masked_layers.begin(), masked_layers.end(), *i);
      if (location != masked_layers.end())
        tcl_file << "etch\tmetal\taniso\tmask=layer_" << *i
                 << "\ttime=2.2\tspacing=$etchspace\n";
      tcl_file << "plot2d\tgrid\tgas\n";
      break;
    default:
      break;
    }
  }
  tcl_file << "plot2d\tgrid\tgas\tgraph=win2\n";
  tcl_file << "# source katana_generated.tcl\n";
  tcl_file.close();
  return EXIT_SUCCESS;
}

void generate_masks(std::vector<std::string> &masks,
                    std::map<int, layer_data> my_layer_info,
                    std::map<int, std::map<int, building_blocks>> my_blocks,
                    std::vector<int> layer_order, const double &db_unit_in_m,
                    std::vector<int> &masked_layers,
                    const int &xsec_dist)
{
  for (auto layer_order_it = layer_order.begin();
       layer_order_it != layer_order.end(); layer_order_it++)
    {
    int layer_number = *layer_order_it;
    int layer_mask_value = my_layer_info[layer_number].mask;
    int sublayer_mask_count = 0;
    for (auto block_it = my_blocks[layer_number].begin();
         block_it != my_blocks[layer_number].end(); block_it++)
    {
      // If mask doesn't cover the entire layer
      if(!( (block_it->second.start_distance==0)&&
            (block_it->second.end_distance==xsec_dist) ) )
      {
        // generate a mask
        bool primary = block_it->second.primary;
        if (((layer_mask_value == 1) && (primary == true)) ||
            ((layer_mask_value == -1) && (primary == false)))
        {
          ostringstream current_mask;
          current_mask << "mask name=layer_";
          current_mask << layer_number;
          current_mask << "\tleft=";
          current_mask << block_it->second.start_distance * db_unit_in_m;
          current_mask << "\tright=";
          current_mask << block_it->second.end_distance * db_unit_in_m;
          current_mask << "\n";
          masks.push_back(current_mask.str());
          masked_layers.push_back(layer_number);
        }
      }
    }
  }
}

int get_layer_height(std::map<int, layer_data> my_layer_info,
                     std::vector<int> &layer_order)
{
  int layer_height = 0;
  for (auto layer_order_it = layer_order.begin();
       layer_order_it != layer_order.end(); layer_order_it++) {
    if (!((my_layer_info[*layer_order_it].film_type == 'A') ||
          (my_layer_info[*layer_order_it].film_type == 'Z'))) {
      layer_height = layer_height + my_layer_info[*layer_order_it].thickness;
    }
  }
  return layer_height;
}

void draw_introduction(ofstream &tcl_file)
{
  tcl_file << "\n";
  tcl_file << "# "
              "----------------------------------------------------------------"
              "-------- #\n";
  tcl_file << "#     FLOOXS PROCESS MODELING SCRIPT Generated by Katana "
              "version Alpha.    #\n";
  tcl_file << "#                      heinrichherbst.github.io/Katana          "
              "           #\n";
  tcl_file << "#                        _                                      "
              "           #\n";
  tcl_file << "#              _________| |___________________________________  "
              "           #\n";
  tcl_file << "#             [_x_x_x_x_   ______________Katana______________/  "
              "           #\n";
  tcl_file << "#                       |_|                                     "
              "           #\n";
  tcl_file << "#                                                               "
              "           #\n";
  tcl_file << "# FLOOXS is distrubuted and developed separately by University "
              "of Florida. #\n";
  tcl_file << "#                          www.flooxs.ece.ufl.edu               "
              "           #\n";
  tcl_file << "#                                                               "
              "           #\n";
  tcl_file << "#     FLOOXS and Katana development funded by IARPA SuperTools "
              "Project     #\n";
  tcl_file << "# "
              "----------------------------------------------------------------"
              "-------- #\n";
}