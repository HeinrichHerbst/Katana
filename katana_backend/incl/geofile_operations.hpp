/**
 * Author:      H.F. Herbst
 * Origin:      Stellenbosch University
 * For:         IARPA SuperTools Project
 * license:     MIT
 * Description: Function declarations for Gmsh .geo geometry file manipulation.
 */
#ifndef KATANA_GEOMETRY_OPERATIONS
#define KATANA_GEOMETRY_OPERATIONS

#include <map>
#include <string>
#include <vector>

namespace GEO{

    struct point
    {
        double x, y, z, char_len;
    };
    bool compare_point(const point &first, const point &second);
    struct line
    {
        int start, end;
    };
    int compare_line(const line &first, const line &second);
    bool check_same_vec(const std::vector<int> &first, const std::vector<int> &second);
    int compare_vector(const std::vector<int> &first, const std::vector<int> &second);
    enum geo_argument
    {
        eBlank_Space,
        eCurve_Loop,
        eDefault,
        eLine,
        ePhysical_Curve,
        ePhysical_Point,
        ePhysical_Surface,
        ePhysical_Volume,
        ePlane_surface,
        eSurface_loop,
        ePoint,
        eVolume,
        eUnroll
    };
    geo_argument hashit (std::string const& inString);
    bool is_integer(std::string const& n) noexcept;
    bool is_e_notation(std::string const& n) noexcept;
    void trim_string_vector(std::vector<std::string> &invec);
    class geofile
    {
    public:
        geofile(/* args */);
        ~geofile();
        int import_geofile(std::string import_path);
        int export_geofile(std::string export_path);
        int simplify_data();
        int merge_with(geofile &secondary_file);
        int translate_data( const double &delta_x,
                            const double &delta_y,
                            const double &delta_z );
        int rotate_data(const double &o_x,       const double &o_y,
                          const double &o_z,       const double &theta_x,
                          const double &theta_y,   const double &theta_z);
        void make_coherent();
    private:
        // Elementary entities
        std::map<int, point>            points_map;
        std::map<int, line>             lines_map;
        std::map<int, std::vector<int>> curve_loops_map;
        std::map<int, int> plane_surfaces_map;
        std::map<int, std::vector<int>> surface_loops_map;
        std::map<int, int> volumes_map;
        // Physical entities
        std::map<std::string, std::vector<int>> physical_points_map;
        std::map<std::string, std::vector<int>> physical_curves_map;
        std::map<std::string, std::vector<int>> physical_surfaces_map;
        std::map<std::string, std::vector<int>> physical_volumes_map;
        // Private functions
        int simplify_element(int &current_status);
        void perform_shift( std::set<int> &gaps,
                            std::set<int> &numbers,
                            std::map<int, int>  &changes);
        int import_physical_entity( std::vector<std::string> &split_string_vector,
                                    std::string &import_path,
                                    int &line_number,
                                    geo_argument &physical_type);

        int import_point(   std::vector<std::string> &split_string_vector,
                            std::string &import_path,
                            int &line_number);
        int simplify_points();
        int adjust_points(std::map<int,int> changes);
        int merge_points(geofile &secondary_file);

        int import_line(   std::vector<std::string> &split_string_vector,
                            std::string &import_path,
                            int &line_number);
        int simplify_lines();
        int adjust_lines(std::map<int,int> changes);
        int merge_lines(geofile &secondary_file);

        int import_curve_loop(  std::vector<std::string> &split_string_vector,
                                std::string &import_path,
                                int &line_number);
        int simplify_curve_loops();
        int adjust_curve_loops(std::map<int,int> changes);
        int merge_curve_loops(geofile &secondary_file);

        int import_plane_surface(  std::vector<std::string> &split_string_vector,
                                std::string &import_path,
                                int &line_number);
        int simplify_plane_surfaces();
        int adjust_plane_surfaces(std::map<int,int> changes);
        int merge_plane_surfaces(geofile &secondary_file);

        int import_surface_loop(  std::vector<std::string> &split_string_vector,
                                std::string &import_path,
                                int &line_number);
        int simplify_surface_loops();
        int adjust_surface_loops(std::map<int,int> changes);
        int merge_surface_loops(geofile &secondary_file);

        int import_volume(  std::vector<std::string> &split_string_vector,
                                std::string &import_path,
                                int &line_number);
        int simplify_volumes();
        int adjust_volumes(std::map<int,int> changes);
        int merge_volumes(geofile &secondary_file);

        int merge_physical_entities(geofile &secondary_file);
        int merge_physical_points(geofile &secondary_file);
        int merge_physical_curves(geofile &secondary_file);
        int merge_physical_surfaces(geofile &secondary_file);
        int merge_physical_volumes(geofile &secondary_file);
        void translate_point(point &point,
                             const double &delta_x,
                             const double &delta_y,
                             const double &delta_z );
        void rotate_point(const double &o_x,       const double &o_y,
                          const double &o_z,       const double &theta_x,
                          const double &theta_y,   const double &theta_z,
                          point &subject);
        void rotate_X(const double &theta, point &in_point);
        void rotate_Y(const double &theta, point &in_point);
        void rotate_Z(const double &theta, point &in_point);
    };
}
#endif