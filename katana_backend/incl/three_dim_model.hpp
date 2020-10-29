/**
 * Author:      H.F. Herbst
 * Origin:      Stellenbosch University
 * For:         IARPA SuperTools Project
 * license:     MIT
 * Description: Function declarations for three-dimensional model generation
 */
#ifndef threedimmodel
#define threedimmodel

#include <map>
#include <string>
#include <vector>
#include "section.hpp"
#include "clipper.hpp"
#include "geofile_operations.hpp"
constexpr double SUBTRACTIVE_OVERLAP_FACTOR = 1.05;   // Overlap factor is applied to layer contour width
constexpr double ADDITIVE_OVERLAP_FACTOR = 1.05;
constexpr int MIN_DIST_BETWEEN_POINTS = 30; //Points closer than 30 nm to one another get filtered out
                                            // 50 worked well except for layer 60
constexpr double ISLAND_OVERLAP_FACTOR = 1.00;
constexpr double BLOCK_EXPANSION_FACTOR = 5;// was 1.5
constexpr bool ROUND_CONCAVE_CORNERS = true;//true still causing overlapping facets
namespace MODEL3D{

  struct Layer_prop {
    std::string name;
    int mask_type;
    int z_start;
    int thickness;
    std::string material;
  };

  struct euclid_vector {
    double i;
    double j;
    double k;
  };

  enum input_type
  {
      eError,
      eNumber,
      eName,
      eMask_type,
      eZ_Start,
      eThickness,
      eMaterial,
      eDefault
  };
  class three_dim_model
  {
  private:
    std::map<unsigned int, std::vector<co_ord>> polygons_map;
    std::map<unsigned int, std::vector<ClipperLib::Path>> clipper_polygons_map;
    std::map<int, Layer_prop> proc_info_map;
    std::map<int, std::vector<int>> layer_boundary_map;
    std::map<int, std::vector<GEO::point>> etch_contour_map;
    std::vector<std::string> py_script;
    double model_default_char_length = 1e-6;
    std::string cell_name;
    bool enable_multiprocessing = false;
    double subtractive_overlap_factor = SUBTRACTIVE_OVERLAP_FACTOR;
    double additive_overlap_factor = ADDITIVE_OVERLAP_FACTOR;
    int min_distance_between_points = MIN_DIST_BETWEEN_POINTS;
    double island_overlap_factor = ISLAND_OVERLAP_FACTOR;
    bool round_concave_corners = ROUND_CONCAVE_CORNERS;
    double block_expansion_factor= BLOCK_EXPANSION_FACTOR;
    int pnt_cnt = 0;
    int ln_cnt  = 0;
    int cl_cnt  = 0;
    int ps_cnt  = 0;
    int sl_cnt  = 0;
    int vol_cnt = 0;
    void        generate_freecad_fill(  const ClipperLib::Path &outline,
                                        const ClipperLib::Paths &primary,
                                        const ClipperLib::Paths &secondary,
                                        const int &layer_number);
    void        generate_freecad_solid( const ClipperLib::Paths &primary,
                                        const ClipperLib::Paths &secondary,
                                        const bool &has_holes,
                                        const int &layer_number,
                                        const int &mask_type);
    void        push_freecad_pss( const ClipperLib::Path &shape_outline,
                                  const bool &bottom_start,
                                  const int &layer_number,
                                  const bool &is_additive,
                                  const double &overlap_factor);
    void        push_freecad_prism( const ClipperLib::Path &shape_outline,
                                    const bool &is_additive,
                                    const int &z_start,
                                    const int &height,
                                    const std::string &sublayer_name);
    void        python_script_write(std::string output_path);
    void        python_script_conclude();
    void        add_recompute();
    void        build_subtractive_shapes(const int &layer_number);
    void        build_additive_shapes(const int &layer_number, const bool &is_pss);
    void        build_additive_sub_layer(const std::string &sub_layer_name);
    void        arrange_path(ClipperLib::Path &to_arrange, std::vector<int> &offset);
    void        initialize_python_script();
    void        initialize_layer_boundary_map();
    int         load_etch_contour_map(const std::string &cont_path);
    int         import_process_info(const std::string &pif_path);
    void        co_ord_to_path(const co_ord &in_vec, ClipperLib::Path &out_path);
    void        path_to_co_ord(const ClipperLib::Path &in_path, co_ord &out_co_ord);
    void        remove_outer_path(ClipperLib::Paths &paths_vec);
    void        close_paths(ClipperLib::Paths &paths_vec);
    void        calculate_all_layer_bounds();
    void        synchronize_clipper_polygons_map();
    void        synchronize_polygon_map();
    void        polygon_layer_to_gds(const int &layer_number);
    void        polygon_map_to_gds();
    void        clipper_paths_to_gds( const ClipperLib::Paths &target_paths,
                                      const std::string &gds_name,
                                      const int &layer_number);
    void        clipper_path_to_gds(const ClipperLib::Path &in_path, const std::string gds_name);
    void        add_right_plane( int &n, int &gain, int &depth,
                                 std::map<int, GEO::line> &lines_map,
                                 std::map<int, std::vector<int>> &curve_loops_map,
                                 std::map<int, int> &plane_surfaces_map,
                                 const int &base_points_count,
                                 int &panel_count,
                                 std::vector<int> &roof_line_vector,
                                 const int &prev_cl_count,
                                 const int &prev_pls_count,
                                 const int &prev_pnt_count);
    void        add_normal_plane( int &n, int &gain, int &depth,
                                 std::map<int, GEO::line> &lines_map,
                                 std::map<int, std::vector<int>> &curve_loops_map,
                                 std::map<int, int> &plane_surfaces_map,
                                 const int &base_points_count,
                                 int &panel_count,
                                 std::vector<int> &roof_line_vector,
                                 const int &prev_cl_count,
                                 const int &prev_pls_count,
                                 const int &prev_pnt_count);
    bool        filter_paths(ClipperLib::Paths &primary_paths, ClipperLib::Paths &secondary_paths);
    void        print_polygons_map();
    int         import_layer_block( std::fstream &infile,
                            std::string &file_buffer,
                            int  &line_number);
    input_type  hash_prop(std::string const& inString);
    int         generate_layer(const int &layer_number);
    void        build_layer_box(  const int &layer_number,
                                  const ClipperLib::IntPoint &a,
                                  const ClipperLib::IntPoint &b,
                                  const ClipperLib::IntPoint &c,
                                  const ClipperLib::IntPoint &d,
                                  GEO::geofile &generated_geofile );
    int         generate_non_fill_layer(const int &layer_number,
                                        const int &mask_type);
    int         generate_inverted_layer(const int &layer_number);
    int         generate_filled_layer(const int &layer_number);
    int         build_polyhedron( const ClipperLib::Path &contour,
                                  const int &layer_number,
                                  const int &unique_ID,
                                  const bool &is_positive,
                                  const bool &outward_facing,
                                  GEO::geofile &generated_geofile);
    bool        clipper_path_is_equal(const ClipperLib::Path &a, const ClipperLib::Path &b);
    double      calculate_theta(const GEO::point &prev,
                                const GEO::point &curr,
                                const GEO::point &next );
    int rotate_point(double cx, double cy, double angle, POINT &subject);
    void rotate_vector( double cx, double cy, double angle,
                        std::vector<GEO::point> &subject);
    void translate_vector( std::vector<GEO::point> &point_vec, const double &delta_x,
                           const double &delta_y, const double &delta_z );
    bool is_elbow(const GEO::point &prev, const GEO::point &curr, const GEO::point &next);
    void tidy_paths(ClipperLib::Paths &input_paths);
    void tidy_path(ClipperLib::Path &input_path);
    void repair_circles(ClipperLib::Paths &target_paths);
    bool circle_test(ClipperLib::Path &target_path, const double &tolerance);
    void calc_bounding_box(const ClipperLib::Path &target_path, ClipperLib::Path &bounding_box);
    double calculate_distance(const ClipperLib::IntPoint &a, const ClipperLib::IntPoint &b);
    void reconstruct_circle(ClipperLib::Path &circle);
    ClipperLib::IntPoint rotate_IntPoint( const ClipperLib::IntPoint &origin,
                                          const ClipperLib::IntPoint &subject,
                                          const double &theta );
    ClipperLib::IntPoint calculate_centroid(const ClipperLib::Path &target_polygon);
  public:
      int generate_model( const std::string &gds_path,
                          const std::string &pif_path,
                          const std::string &py_path,
                          const std::string &cont_path,
                          const bool &multi_flag);
      three_dim_model()
      {

      }
      ~three_dim_model() {}
  };
}
#endif