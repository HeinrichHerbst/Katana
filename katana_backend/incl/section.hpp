/**
 * Author:      H.F. Herbst
 * Origin:      Stellenbosch University
 * For:         IARPA SuperTools Project
 * license:     MIT
 * Description: Function declarations for cross-section generation.
 */
#ifndef gdssection
#define gdssection
#include "gdsCpp.hpp"
#include <cassert>

// Holds co-ordinates of a polygon.
struct co_ord
{
  std::vector<int> x; // Linked to y (xy pair) - path of polygon.
  std::vector<int> y; // Linked to X (xy pair) - path of polygon.
};

struct POINT
{
  double x = 0;
  double y = 0;
};

struct layer_data
{
  // Layer number is used as the key for map.
  std::string name = "not_initialized"; // Name of layer
  int thickness = 0;                    // Stored in nanometers
  int order = 0; // Vertical order. 0 = lowest, 23 ~ highest
  int mask = 0;  // Mask polarity. -1 = inverse, 0 = don't show, 1=normal
  char film_type =
      'Z';        // S = superconductor, N = normal conductor, I = isolator, A =
                  // auxiliary/don't care, Z = un-initialized, R = Resistor
  int colour = 0; // DXF colour map value
};

struct intercept_data
{
  int x;    // x-coord of intercept
  int y;    // y-coord of intercept
  int dist; // distance_along intercept path
  bool operator<(const intercept_data &a) const
  {
    return (dist < a.dist);
  }
  bool operator==(const intercept_data &a) const
  {
    return ((x == a.x) && (y == a.y) && (dist == a.dist));
  }
};

// Holds blocks used with layer information to generate cross section
struct building_blocks
{
  int start_distance = 0; // Distance from start of cross-section
  int end_distance = 0;   // Distance from start of cross-section
                          // True indicates material
                          // False indicates absence of material
  bool primary = true;
};
class section_data
{
private:
  // Holds all elements converted to polygons
  std::map<unsigned int, std::vector<co_ord>> polygons; // key = layer number
  std::map<int, std::vector<intercept_data>>
      layer_intercepts; // key = layer number
  std::map<int, std::map<int, building_blocks>>
      section_blocks; // key = layer number, second key = block_ID
  int x1, y1, x2, y2; // x-section co-ordinates
  int axis_type;      // x-z, y-z or d-z axes
  int xsec_distance;  // Database units (standard is nanometer)

  int calibrate_axes(int xx1, int yy1, int xx2, int yy2);
  int populate_polygons(gdscpp &gds_file);

  int calculate_intercepts();
  int analyze_polygon(int layer, const co_ord &line_data);
  int analyze_line_seg(int ln_strt[2], int ln_end[2], int layer);
  int slice_horiz_horiz(int line_start[2], int line_end[2], int &layer);    // 1
  int slice_horiz_angled(int line_start[2], int line_end[2], int &layer);   // 2
  int slice_horiz_vertical(int line_start[2], int line_end[2], int &layer); // 3
  int slice_angled_horiz(int line_start[2], int line_end[2], int &layer);   // 4
  int slice_angled_angled(int line_start[2], int line_end[2], int &layer);  // 5
  int slice_angled_vertical(int line_start[2], int line_end[2],
                            int &layer);                                    // 6
  int slice_vertical_horiz(int line_start[2], int line_end[2], int &layer); // 7
  int slice_vertical_angled(int line_start[2], int line_end[2],
                            int &layer);                               // 8
  int slice_vert_vert(int line_start[2], int line_end[2], int &layer); // 9
  int push_intercept(int &layer, intercept_data intercept);
  int unique_sort_intercepts();
  void print_intercepts();
  int generate_blocks();
  int improve_blocks();

public:
  int populate(int &xx1, int &yy1, int &xx2, int &yy2, gdscpp &gds_file);
  int pull_section_blocks(
      std::map<int, std::map<int, building_blocks>> &target_destination);
  int get_sec_dist();
  int pull_used_layers(std::vector<int> &destination_vector);
  void polygon_to_gds(const std::string &outpath);

  section_data()
  {
    x1 = 0;
    x2 = x1;
    y1 = x2;
    y2 = y1;
    xsec_distance = y2, axis_type = xsec_distance;
  }
  ~section_data() {}
};

bool check_for_intercept(int *bounding_box, int &axis_type, int &x1, int &y1,
                         int &x2, int &y2);
int calibrate_line(int x1, int y1, int x2, int y2);
int set_section_type(int &line_type, int &section_type);
int path_to_polygon(gdsPATH path_data, co_ord &dest_co_ord);

void recursive_unpack(
    const std::string &structure_name, std::vector<gdsSTR> &struct_vect,
    std::unordered_map<std::string, int> &struct_lookup,
    std::map<unsigned int, std::vector<co_ord>> &retrieved_polygon_data);

int process_boundaries(
    std::vector<gdsBOUNDARY> &current_boundaries,
    std::map<unsigned int, std::vector<co_ord>> &poly_map_destination);
int process_boxes(
    std::vector<gdsBOX> &current_boxes,
    std::map<unsigned int, std::vector<co_ord>> &poly_map_destination);
int process_paths(
    std::vector<gdsPATH> &current_paths,
    std::map<unsigned int, std::vector<co_ord>> &poly_map_destination);
int process_SREFs(
    std::vector<gdsSREF> &current_SREFs,
    std::map<unsigned int, std::vector<co_ord>> &poly_map_destination,
    std::vector<gdsSTR> &struct_vect,
    std::unordered_map<std::string, int> &struct_lookup);
int process_AREFs(
    std::vector<gdsAREF> &current_arefs,
    std::map<unsigned int, std::vector<co_ord>> &poly_map_destination,
    std::vector<gdsSTR> &struct_vect,
    std::unordered_map<std::string, int> &struct_lookup);
int transform_polygon_map(
    std::map<unsigned int, std::vector<co_ord>> &polygon_map, bool reflect_x,
    double magnification, int angle, int x_offset, int y_offset);
int transform_aref_map(std::map<unsigned int, std::vector<co_ord>> &target,
                       gdsAREF &aref_info);
void repeat_polygon_map(std::map<unsigned int, std::vector<co_ord>> &target,
                        gdsAREF &aref_info);
void insert_or_append_to_polymap(
    std::map<unsigned int, std::vector<co_ord>> &target_map, int key,
    const co_ord &polygon);
void rotate_vector(double cx, double cy, double angle,
                   std::vector<POINT> &subject);
void combine_maps(std::map<unsigned int, std::vector<co_ord>> &main,
                  std::map<unsigned int, std::vector<co_ord>> &auxiliary);
constexpr auto PI = 3.14159265358979323846;
#endif