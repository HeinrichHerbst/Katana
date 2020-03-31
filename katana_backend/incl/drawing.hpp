/**
 * Author:      H.F. Herbst
 * Origin:      Stellenbosch University
 * For:         IARPA SuperTools Project
 * license:     MIT
 * Description: Function declarations for.geo file drawing
 */
#ifndef drawing
#define drawing

#include "ldf_process.hpp"
#include "section.hpp"

int draw_geofile(section_data section_info, ldf ldf_info,
                 const std::string &outpath, const double &db_unit);
int get_layer_order(std::vector<int> &layers,
                    std::map<int, layer_data> &layer_info);

#endif