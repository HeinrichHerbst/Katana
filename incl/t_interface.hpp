/**
 * Author:      H.F. Herbst
 * Origin:      Stellenbosch University
 * For:         IARPA SuperTools Project
 * license:     MIT
 * Description: Function declarations for user interface of Katana terminal edition
 */
#ifndef terminal_interface
#define terminal_interface
#include <string>
#include <cctype>
#include <vector>
#include <filesystem>

constexpr auto CURRENT_VERSION = 1.0;   // T for terminal
namespace UI
{
    void print_ln(std::string line);
    void skip_ln();
    void clear_screen();
    void print_help();
    void print_welcome();
    void print_version();
    void main_menu();
    int get_choice();
    void interactive_mode();
    void argument_mode(int &argc, char *argv[]);
    void gather_3D_arg(int &argc, char *argv[]);
    void gather_slice_arg(int &argc, char *argv[]);
    void gather_modeling_arg(int &argc, char *argv[]);
    void gather_meshops_arg(int &argc, char *argv[]);
    void gather_slice_interactive();
    void gather_modeling_interactive();
    void gather_mesh_interactive();
    void gather_3DGen_interactive();
    void gather_silver_linings_interactive();
    void execute_silver_linings(std::string mesh_in, std::string geo_out);
    void execute_volume_calculator(const std::string &mesh_in);
    void gather_append_interactive();
    void process_co_ords(std::vector<int> &co_ords, int &points_gathered);
    bool isinteger(std::string const& n) noexcept;
    void execute_slice(std::string gds_path, std::string ldf_path, int x1, int y1, int x2, int y2);
    void execute_simple_append(int &argc, char *argv[]);
    void execute_simple_merge(int &argc, char *argv[]);
    void gather_merge_interactive();
    void execute_rotate(int &argc, char *argv[]);
    void gather_rotate_interactive();
    void execute_translate(int &argc, char *argv[]);
    void gather_translate_interactive();
    void execute_simple_append_interactive( std::string first_geo,
                                                std::string second_geo,
                                                std::string output_geo,
                                                std::string char_len);
    void gather_volume_calc_interactive();
    void gather_scale_interactive();
    void execute_scale(int &argc, char *argv[]);
}
#endif