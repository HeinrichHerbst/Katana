/**
 * Author:      H.F. Herbst
 * Origin:      Stellenbosch University
 * For:         IARPA SuperTools Project
 * license:     MIT
 * Description: Function definitions for new mesh file operations.
 */
#include "new_meshfile_operations.hpp"
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <fstream>

MSH::mesh_file::mesh_file()
{
}

MSH::mesh_file::~mesh_file()
{
}

// if 0  = success
// if !0 = fail
int MSH::check_read_state(std::fstream &infile)
{
    if (infile.bad())
    {
        std::cout << "Error: Reached bad read state of msh file." << std::endl;
        return EXIT_FAILURE;
    }
    else
        return EXIT_SUCCESS;
    return EXIT_FAILURE;
}

MSH::line_type MSH::hash_line_type(std::string const& inString)
{
    if(inString == "$MeshFormat") return eMeshFormat;
    if(inString == "$PhysicalNames") return ePhysicalNames;
    if(inString == "$Nodes") return eNodes;
    if(inString == "$Elements") return eElements;
    return eDefault;
}

int MSH::mesh_file::import_meshfile(const std::string &import_path)
{
    std::fstream mesh_file;
    std::string line_buffer;
    mesh_file.open(import_path, std::ios::in);
    if (mesh_file.is_open())
    {
        while (getline(mesh_file, line_buffer))
        {
            line_type hash_line = hash_line_type(line_buffer);
            switch (hash_line)
            {
            case eMeshFormat:
                if(scan_MeshFormat(mesh_file, line_buffer)) return EXIT_FAILURE;
                break;
            case ePhysicalNames:
                if(scan_PhysicalNames(mesh_file, line_buffer)) return EXIT_FAILURE;
                break;
            case eNodes:
                if(scan_Nodes(mesh_file, line_buffer)) return EXIT_FAILURE;
                break;
            case eElements:
                if(scan_Elements(mesh_file, line_buffer)) return EXIT_FAILURE;
                break;
            default:
                break;
            }
        }
    }
    mesh_file.close();
    return EXIT_SUCCESS;
}
int MSH::mesh_file::scan_MeshFormat(std::fstream &mesh_file, std::string &line_buffer)
{
    getline(mesh_file, line_buffer);
    if (check_read_state(mesh_file)) return EXIT_FAILURE;
    if (line_buffer.compare("2.2 0 8"))
    {
        std::cout << "Error: Katana only supports Gmsh 2.2 0 8 format. Export file into old format.";
        return EXIT_FAILURE;
    }
    getline(mesh_file, line_buffer);
    if (check_read_state(mesh_file)) return EXIT_FAILURE;
    if (line_buffer.compare("$EndMeshFormat"))
    {
        std::cout << "Error: End of mesh format section was not found." << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int MSH::mesh_file::scan_PhysicalNames(std::fstream &mesh_file, std::string &line_buffer)
{
    while ( (getline(mesh_file, line_buffer))&&
            (line_buffer.compare("$EndPhysicalNames")) )
    {
        if (check_read_state(mesh_file)) return EXIT_FAILURE;
        std::vector<std::string> split_str_vec;
        boost::split(split_str_vec, line_buffer, boost::is_any_of("\""));
        if (split_str_vec.size()>2)
            physical_names_vect.push_back(split_str_vec[1]);
    }
    return EXIT_SUCCESS;
}

int MSH::mesh_file::scan_Nodes(std::fstream &mesh_file, std::string &line_buffer)
{
    while ( (getline(mesh_file, line_buffer))&&
            (line_buffer.compare("$EndNodes")) )
    {
        if (check_read_state(mesh_file)) return EXIT_FAILURE;
        std::vector<std::string> split_str_vec;
        boost::split(split_str_vec, line_buffer, boost::is_any_of(" "));
        if (split_str_vec.size()==4)
        {
            node input_node;
            input_node.x = std::stod(split_str_vec[1]);
            input_node.y = std::stod(split_str_vec[2]);
            input_node.z = std::stod(split_str_vec[3]);
            nodes_map.insert({std::stoi(split_str_vec[0]),input_node});
        }
    }
    return EXIT_SUCCESS;
}

int MSH::mesh_file::scan_Elements(std::fstream &mesh_file, std::string &line_buffer)
{
    while ( (getline(mesh_file, line_buffer))&&
            (line_buffer.compare("$EndElements")) )
    {
        if (check_read_state(mesh_file)) return EXIT_FAILURE;
        std::vector<std::string> split_str_vec;
        boost::split(split_str_vec, line_buffer, boost::is_any_of(" "));
        auto element_size = split_str_vec.size();
        if (element_size>5)
        {
            element input_ele;
            int counter = 0;
            int index = std::stoi(split_str_vec[counter++]);
            input_ele.type = std::stoi(split_str_vec[counter++]);
            int tag_count = std::stoi(split_str_vec[counter++]);
            auto first_end = counter+tag_count;
            for (auto i = counter; i < first_end; i++)
            {
                input_ele.tags.push_back(std::stoi(split_str_vec[counter]));
                counter++;
            }
            for (auto j = counter; j < element_size; j++)
            {
                input_ele.node_vect.push_back(std::stoi(split_str_vec[j]));
            }
            elements_map.insert({index, input_ele});
        }
    }
    return EXIT_SUCCESS;
}

int MSH::mesh_file::print_volumes()
{
    if(physical_names_vect.size()!=0)
    {
        std::vector<double> volumes_vect;
        auto pnv_end = physical_names_vect.end();
        for (auto i = physical_names_vect.begin(); i != pnv_end; i++)
        {
            volumes_vect.push_back(0);
        }
        auto elmap_end = elements_map.end();
        for (auto i = elements_map.begin(); i != elmap_end; i++)
        {
            if(i->second.type==4)
            {
                if (i->second.node_vect.size()==4)
                {
                    double tet_vol = vol_tetrahedron( nodes_map[i->second.node_vect[0]],
                                                    nodes_map[i->second.node_vect[1]],
                                                    nodes_map[i->second.node_vect[2]],
                                                    nodes_map[i->second.node_vect[3]] );
                    volumes_vect[i->second.tags[0]-1] = volumes_vect[i->second.tags[0]-1] + tet_vol;
                }
                else
                {
                    std::cout << "Error: Tetrahedron specified but not enough points." << std::endl;
                    return EXIT_FAILURE;
                }
            }
        }
        std::cout << "Volumes:" << std::endl;
        int counter = 0;
        for (auto i = physical_names_vect.begin(); i != pnv_end; i++)
        {
            std::cout << *i << " = " << volumes_vect[counter++] << std::endl;
        }
        return EXIT_SUCCESS;
    }
    else
    {
        std::cout << "No physical names detected." << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_FAILURE;
}

void MSH::cross_product(const node &U, const node &V, node &out)
{
    out.x = U.y * V.z - V.y * U.z;
    out.y = V.x * U.z - U.x * V.z;
    out.z = U.x * V.y - V.x * U.y;
}

double MSH::dot_product(const node &U, const node &V)
{
    double output = U.x * V.x + U.y * V.y + U.z * V.z;
    return output;
}

double MSH::vol_tetrahedron(const node &a, const node &b, const node &c, const node &d)
{
    node a_minus_d, b_minus_d, c_minus_d;
    a_minus_d.x = a.x - d.x;
    a_minus_d.y = a.y - d.y;
    a_minus_d.z = a.z - d.z;
    b_minus_d.x = b.x - d.x;
    b_minus_d.y = b.y - d.y;
    b_minus_d.z = b.z - d.z;
    c_minus_d.x = c.x - d.x;
    c_minus_d.y = c.y - d.y;
    c_minus_d.z = c.z - d.z;
    node b_minus_d_X_c_minus_d;
    cross_product(b_minus_d,c_minus_d, b_minus_d_X_c_minus_d);
    double volume = std::abs(dot_product(a_minus_d,b_minus_d_X_c_minus_d))/6;
    return volume;
}