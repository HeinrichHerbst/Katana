/**
 * Author:      H.F. Herbst
 * Origin:      Stellenbosch University
 * For:         IARPA SuperTools Project
 * license:     MIT
 * Description: Function declarations for new mesh file operations
 */
#ifndef NEW_MESHOPS_H
#define NEW_MESHOPS_H

#include <map>
#include <string>
#include <vector>

namespace MSH
{
    int check_read_state(std::fstream &infile);
    struct node
    {
        double x = 0;
        double y = 0;
        double z = 0;
    };

    struct element
    {
        int type;
        std::vector<int> tags;
        std::vector<int> node_vect;
    };

    enum line_type {
        eDefault,
        ePhysicalNames,
        eNodes,
        eElements,
        eMeshFormat
    };
    line_type hash_line_type(std::string const& inString);
    void cross_product(const node &U, const node &V, node &out);
    double dot_product(const node &U, const node &V);
    double vol_tetrahedron(const node &A, const node &B, const node &C, const node &D);


    class mesh_file
    {
    public:
        mesh_file(/* args */);
        ~mesh_file();
        int import_meshfile(const std::string &import_path);
        int print_volumes();
    private:
        std::map<int,node>          nodes_map;
        std::map<int, element>      elements_map;
        std::vector<std::string>    physical_names_vect;
        int scan_MeshFormat(std::fstream &mesh_file, std::string &line_buffer);
        int scan_PhysicalNames(std::fstream &mesh_file, std::string &line_buffer);
        int scan_Nodes(std::fstream &mesh_file, std::string &line_buffer);
        int scan_Elements(std::fstream &mesh_file, std::string &line_buffer);
    };
}
#endif