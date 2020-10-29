/**
 * Author:      H.F. Herbst
 * Origin:      Stellenbosch University
 * For:         IARPA SuperTools Project
 * license:     MIT
 * Description: Function declarations for FLOOXS tcl generation
 */
#ifndef flooxsgen
#define flooxsgen

#include "drawing.hpp"
#include "ldf_process.hpp"
#include "section.hpp"

int draw_flooxs_file(section_data section_info, ldf ldf_info,
                     const std::string &outpath, const double &db_unit,
                     const double &db_unit_in_m, const int &spacing);
void draw_introduction(std::ofstream &tcl_file);
int get_layer_height(std::map<int, layer_data> my_layer_info,
                     std::vector<int> &layer_order);
void generate_masks(std::vector<std::string> &masks,
                    std::map<int, layer_data> my_layer_info,
                    std::map<int, std::map<int, building_blocks>> my_blocks,
                    std::vector<int> layer_order, const double &db_unit_in_m,
                    std::vector<int> &masked_layers,
                    const int &xsec_dist);

#endif