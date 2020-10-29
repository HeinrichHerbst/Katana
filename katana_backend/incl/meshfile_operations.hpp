/**
 * Author:      H.F. Herbst
 * Origin:      Stellenbosch University
 * For:         IARPA SuperTools Project
 * license:     MIT
 * Description: Function declarations for mesh file operations
 */
#ifndef MESHFILE_OPERATIONS_H
#define MESHFILE_OPERATIONS_H
#include <vector>
#include <map>
//Structures-----------------------------------------
struct co_ordinates
{
    double co_ord[3];
};
struct node_duplication_data
{
    int original_node;
    int duplicated_node;
};
struct material_duplication_data
{
    int original_material_ID;
    int duplicated_material_ID;
};
    // Assumption that all elements are triangless
struct elements
{
    int element_properties[4];// parent material, node 1, node 2, node 3
};
/*
Face types:
1 = border (appears on outer edges of mesh)
2 = interface (appears between two materials)
3 = insider (appears between two faces of the same material)
*/
struct face_properties
{
    int first_node;
    int second_node;
    int face_type;
    int parent_materials[2];
};
struct line
{
    int a;
    int b;
    bool used;
};
int check_readstate(std::fstream &infile);
int begin_processing(std::string meshpath, std::string geopath);
bool co_ord_compare(const co_ordinates &lhs, const co_ordinates &rhs);
int optimize_meshfile(  std::map<int, co_ordinates> &nodes_map,
                        std::map<int, node_duplication_data> &node_duplicated_data_map,
                        std::map<int, std::string> &materials_map,
                        std::map<int, material_duplication_data> &duplicated_material_data_map   );
void sort_element(std::map<int, elements>::iterator &element_iteration);
bool redirect_elements( std::map<int, co_ordinates> &nodes_map,
                        std::map<int, node_duplication_data> &node_duplicated_data_map,
                        std::map<int, elements> &elements_map,
                        std::map<int, material_duplication_data> &duplicated_material_data_map);
bool compare_faces(const face_properties &face_in_question, const face_properties &iterator_face);
void facemap_scanner(std::map<int, face_properties> &faces_map, int &face_map_index, face_properties &current_face);
int generate_face_data(std::map<int, elements> &elements_map, std::map<int, face_properties> &faces_map);
int plot_geometry(    std::map<int, co_ordinates> &nodes_map,
                      std::map<int, face_properties> &faces_map,
                        const std::string &geo_file_path);
int generate_shapes(    std::map<int, face_properties> &faces_map,
                        std::map<int, std::string> &materials_map,
                        const std::string &geo_file_path);
bool check_for_completion(int required_count, int current_count);
int swap_axes(std::map<int, co_ordinates> &nodes_map);
#endif
