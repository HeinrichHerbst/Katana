/**
 * Author:      H.F. Herbst
 * Origin:      Stellenbosch University
 * For:         IARPA SuperTools Project
 * license:     MIT
 * Description: Function declarations for geofile stitching.
 */
#ifndef stitching
#define stitching
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

struct node {
  double x = 0;
  double y = 0;
  double z = 0;
  double characteristic_length = 0;
  bool raised = false;
};
namespace stitch{
struct line {
  int first = 0;
  int second = 0;
  bool raised = false;
};
}

int shift_file_b(std::map<int, node> &b_nodes_map,
                 std::map<int, stitch::line> &b_lines_map, int &highest_node_ID,
                 int &highest_line_ID, double &offset);

int calculate_shifting_parameters(std::map<int, node> &a_nodes_map,
                                  std::map<int, stitch::line> &a_lines_map,
                                  int &highest_node_ID, int &highest_line_ID,
                                  double &offset);

int read_in_geofiles(const std::string &f1, const std::string &f2,
                     const std::string &f3, std::map<int, node> &a_nodes_map,
                     std::map<int, stitch::line> &a_lines_map,
                     std::map<int, node> &b_nodes_map,
                     std::map<int, stitch::line> &b_lines_map);

int print_stitched_file(std::map<int, node> &a_nodes_map,
                        std::map<int, stitch::line> &a_lines_map,
                        std::map<int, node> &b_nodes_map,
                        std::map<int, stitch::line> &b_lines_map,
                        const std::string &outfile_path,
                        const std::string &char_len_over);

void simple_append( std::string first_geo,
                        std::string second_geo,
                        std::string output_geo,
                        std::string char_len);

#endif