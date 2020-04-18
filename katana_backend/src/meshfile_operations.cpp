/**
 * Author:      H.F. Herbst
 * Origin:      Stellenbosch University
 * For:         IARPA SuperTools Project
 * license:     MIT
 * Description: Function definitions for mesh file operations
 */
#include <fstream>
#include <iomanip>
#include <string>
#include <sstream>
#include <set>
#include <vector>
#include <map>
#include <unordered_set>
#include <chrono>
#include <ctime>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "meshfile_operations.hpp"
#define LOOPTIMEOUT 1000000

// Checks if file stream has unexpected behaviour
int check_readstate(std::fstream &infile)
{
    if (infile.bad())
    {
        return EXIT_FAILURE;
    }
    else
    {
        return EXIT_SUCCESS;
    }
}
//  Reads meshfile into maps. Calls Mesh optimizer if read-in was successful.
int begin_processing(std::string meshpath, std::string geopath)
{
    std::ofstream log_file;
    std::string logf_print_buffer;
    std::string file_output_name = "logfile.txt";
    time_t log_attempt_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::fstream mesh_file;
    std::string file_buffer;
    std::map<int, std::string> materials_map;
    std::map<int, co_ordinates> nodes_map;
    std::map<int, elements> elements_map;
    std::map<int, face_properties> faces_map;
    std::map<int, node_duplication_data> node_duplicated_data_map;
    std::map<int, material_duplication_data> duplicated_material_data_map;

    log_file.open( file_output_name, std::ios::out | std::ios::app );
    log_file << "=============================================================\n";
    log_file << "Conversion attempt at " << std::ctime(&log_attempt_time);
    mesh_file.open(meshpath, std::ios::in);
    if (mesh_file.is_open())
    {
        log_file << "Mesh file successfully opened.\n";
        while (getline(mesh_file, file_buffer))
        {
            // ============================== Mesh Format Section Parser ===============================
            if (file_buffer.compare("$MeshFormat") == 0)
            {
                getline(mesh_file, file_buffer);
                if (check_readstate(mesh_file)==EXIT_FAILURE)
                {
                    log_file << "Error: File has reached a bad state while looking for version number.\n";
                    log_file << "Error: Please ensure correct format of msh file.\n";
                    break;
                }
                if (file_buffer.compare("2.2 0 8") == 0)
                {
                    log_file << "Version Accepted: Reading in Mesh data.\n";
                }
                else
                {
                    log_file << "Error: SilverLining Currently only works with GMSH format 2.2 0 8\n";
                    log_file.close();
                    return EXIT_FAILURE;
                    break;
                }
                //confirm that you reached end of meshformat.
                getline(mesh_file, file_buffer);
                if (check_readstate(mesh_file)==EXIT_FAILURE)
                {
                    log_file << "Error: File has reached a bad state while looking for mesh format.\n";
                    log_file << "Error: Please ensure correct format of msh file.\n";
                    break;
                }
                if (file_buffer.compare("$EndMeshFormat") == 0)
                {
                    log_file << "Reached end of mesh format section\n";
                }
                else
                {
                    log_file << "Error: End of mesh format section was not found.\n";
                    log_file.close();
                    return EXIT_FAILURE;
                }

            }
            // ============================ Physical Names Section Parser ==============================
            if (file_buffer.compare("$PhysicalNames") == 0)
            {
                getline(mesh_file, file_buffer);
                if (check_readstate(mesh_file)==EXIT_FAILURE)
                {
                    log_file << "Error: File has reached a bad state while looking for physical names.\n";
                    log_file << "Error: Please ensure correct format of msh file.\n";
                    break;
                }
                int physical_names_count = stoi(file_buffer);
                for (int a = 0; a < physical_names_count; a++)
                {
                    getline(mesh_file, file_buffer);
                    file_buffer = file_buffer.substr(6, file_buffer.length() - 7);
                    materials_map.insert(std::pair<int, std::string>((a+1),file_buffer));
                }
                getline(mesh_file, file_buffer);
                if (check_readstate(mesh_file)==EXIT_FAILURE)
                {
                    log_file << "Error: File has reached a bad state while looking for end of physical names.\n";
                    log_file << "Error: Please ensure correct format of msh file.\n";
                    break;
                }
                if (file_buffer.compare("$EndPhysicalNames") == 0)
                {
                    log_file << "Reached end of physical names section\n";
                }
                else
                {
                    log_file << "Error: End of physical names section was not found.\n";
                    log_file.close();
                    return EXIT_FAILURE;
                }
            }
            // ================================= Nodes Section Parser =================================
            if (file_buffer.compare("$Nodes") == 0)
            {
                getline(mesh_file, file_buffer);
                if (check_readstate(mesh_file)==EXIT_FAILURE)
                {
                    log_file << "Error: File has reached a bad state while looking for version number.\n";
                    log_file << "Error: Please ensure correct format of msh file.\n";
                    break;
                }
                int nodes_count = stoi(file_buffer);
                for (int b = 0; b < nodes_count; b++)
                {
                    getline(mesh_file, file_buffer);
                    std::vector<std::string> split_string_vector;
                    boost::split(split_string_vector,file_buffer,boost::is_any_of(" "));
                    co_ordinates current_co_ords;
                    current_co_ords.co_ord[0] = (boost::lexical_cast<double>(split_string_vector[1]))/100;
                    current_co_ords.co_ord[1] = (boost::lexical_cast<double>(split_string_vector[2]))/100;
                    current_co_ords.co_ord[2] = (boost::lexical_cast<double>(split_string_vector[3]))/100;
                    nodes_map.insert(std::pair<int, co_ordinates>(stoi(split_string_vector[0]),current_co_ords));
                }
                getline(mesh_file, file_buffer);
                if (check_readstate(mesh_file)==EXIT_FAILURE)
                {
                    log_file << "Error: File has reached a bad state while looking for nodes.\n";
                    log_file << "Error: Please ensure correct format of msh file.\n";
                    break;
                }
                if (file_buffer.compare("$EndNodes") == 0)
                {
                     log_file << "Reached end of nodes section\n";
                }
                else
                {
                    log_file << "Error: End of nodes section was not found\n";
                }
            }
            // ================================= Elements Section Parser =================================
            if (file_buffer.compare("$Elements") == 0) // FLOOXS Bug puts a space after elements
            {
                getline(mesh_file, file_buffer);
                if (check_readstate(mesh_file)==EXIT_FAILURE)
                {
                    log_file << "Error: File has reached a bad state while looking for Elements.\n";
                    log_file << "Error: Please ensure correct format of msh file.\n";
                    break;
                }
                int elements_count = stoi(file_buffer);
                for (int c = 0; c < elements_count; c++)
                {
                    getline(mesh_file, file_buffer);
                    // Each subvector holds the material index and the triangle nodes.
                    // The node ID number = primary vector index +1
                    std::vector<std::string> split_string_elements_vector;
                    elements current_element;
                    boost::split(split_string_elements_vector,file_buffer,boost::is_any_of(" "));
                    current_element.element_properties[0]=boost::lexical_cast<int>(split_string_elements_vector[4]);
                    current_element.element_properties[1]=boost::lexical_cast<int>(split_string_elements_vector[5]);
                    current_element.element_properties[2]=boost::lexical_cast<int>(split_string_elements_vector[6]);
                    current_element.element_properties[3]=boost::lexical_cast<int>(split_string_elements_vector[7]);

                    elements_map.insert(std::pair<int, elements>(boost::lexical_cast<int>(split_string_elements_vector[0]),current_element));
                }
                getline(mesh_file, file_buffer);
                if (check_readstate(mesh_file)==EXIT_FAILURE)
                {
                    log_file << "Error: File has reached a bad state while looking for Elements.\n";
                    log_file << "Error: Please ensure correct format of msh file.\n";
                    break;
                }
                if (file_buffer.compare("$EndElements") == 0)  //FLOOXS bug also puts a space here
                {
                    log_file << "Reached end of elements section.\n";
                }
                else
                {
                    log_file << "Error: End of elements section was not found.\n";
                }
            }
        }
        mesh_file.close();
        log_file << "Mesh file successfully closed.\n";
        log_file << "Reached end of main function.\n";
        log_file << "========================== STATISTICS =======================\n";
        log_file << "Number of Materials = "<< materials_map.size() << "\n";
        log_file << "Number of Nodes = "<< nodes_map.size() << "\n";
        log_file << "Number of Elements = "<< elements_map.size() << "\n";
        log_file << "-------------------------------------------------------------\n";
        log_file.close();
        if (optimize_meshfile(nodes_map, node_duplicated_data_map, materials_map, duplicated_material_data_map) == EXIT_SUCCESS)
        {
            //log elements before
            log_file.open( file_output_name, std::ios::out | std::ios::app );
            log_file << "=================== Remapping of Elements ===================\n";
            log_file << "------------------- Elements before re-map ------------------\n";
            std::map<int, elements>::iterator element_iterator = elements_map.begin();
            while (element_iterator != elements_map.end())
            {
                log_file <<"E"<<element_iterator->first<<": [Material "
                << element_iterator->second.element_properties[0] << ": "
                << element_iterator->second.element_properties[1] << ", "
                << element_iterator->second.element_properties[2] << ", "
                << element_iterator->second.element_properties[3] << "]\n";
                element_iterator++;
            }
            int exit = redirect_elements(nodes_map, node_duplicated_data_map, elements_map, duplicated_material_data_map);
            log_file << "------------------- Elements after re-map ------------------\n";
            element_iterator = elements_map.begin();
            while (element_iterator != elements_map.end())
            {
                log_file <<"E"<<element_iterator->first<<": [Material "
                << element_iterator->second.element_properties[0] << ": "
                << element_iterator->second.element_properties[1] << ", "
                << element_iterator->second.element_properties[2] << ", "
                << element_iterator->second.element_properties[3] << "]\n";
                element_iterator++;
            }
            exit=generate_face_data(elements_map, faces_map);
            log_file << "------------------------- Faces List -----------------------\n";
            std::map<int, face_properties>::iterator faces_iterator = faces_map.begin();
            while (faces_iterator != faces_map.end())
            {
                log_file <<"Face "<<faces_iterator->first<<": ["
                <<faces_iterator->second.first_node <<", "
                <<faces_iterator->second.second_node <<", "
                <<faces_iterator->second.face_type <<", "
                <<faces_iterator->second.parent_materials[0] <<"|"
                << faces_iterator->second.parent_materials[1] <<"]\n";
                faces_iterator++;
            }
            int err_count = 0;
            if(swap_axes(nodes_map)==EXIT_FAILURE)
            {
                log_file << "Error: Axis swap failed \n";
                err_count++;
            }
            if(plot_geometry(nodes_map, faces_map, geopath)==EXIT_FAILURE)
            {
                log_file << "Error: Geometry plotting failed.\n";
                err_count++;
            }
            if(generate_shapes(faces_map, materials_map, geopath)==EXIT_FAILURE)
            {
                log_file << "Error: Face generation failed.\n";
                err_count++;
            }
            log_file << "Program reached end of conversion with " << err_count << " errors.\n";
            log_file.close();
            return exit;
        }
    }
    else
    {
        log_file << "Unable to open file. Please confirm file name.\n";
        log_file << "-------------------------------------------------------------\n";
    }
    log_file << "Error in Meshfile Optimization, EXIT_FAILURE.\n";
    log_file.close();
    return EXIT_FAILURE;
}

// Function to compare co-ordinates structure. Returns true if co-ordinates match.
bool co_ord_compare(const co_ordinates &lhs, const co_ordinates &rhs)
{
    if ((lhs.co_ord[0] == rhs.co_ord[0]) & (lhs.co_ord[1] == rhs.co_ord[1]) & (lhs.co_ord[2] == rhs.co_ord[2]))
    {
        return true;
    }
    else
    {
        return false;
    }
}

//compares the material of iterator a to iterator b
bool material_compare(  const std::map<int,
                        std::string>::iterator &lhs,
                        const std::map<int, std::string>::iterator &rhs)
{
    if(lhs->second.compare(rhs->second) == 0)
    {
        return true;
    }
    else
    {
        return false;
    }

}

//re-adjusts the meshfile to remove duplicate points along mesh boundary - thus increasing performance.
//additionally removes duplicate materials
int optimize_meshfile(  std::map<int, co_ordinates> &nodes_map,
                        std::map<int, node_duplication_data> &node_duplicated_data_map,
                        std::map<int, std::string> &materials_map,
                        std::map<int, material_duplication_data> &duplicated_material_data_map   )
{
    int exit_status = EXIT_FAILURE;
    int duplicates_counter = 0;
    std::ofstream log_file;
    std::map<int, co_ordinates>::iterator main_iterator = nodes_map.begin();
    std::map<int, co_ordinates>::iterator limit = --nodes_map.end();

    std::string logf_print_buffer;
    std::string file_output_name = "logfile.txt";
    log_file.open( file_output_name, std::ios::out | std::ios::app );
    log_file << "================== Mesh Duplication Reduction ===============\n";
    // Display the original node data
    log_file << "--------------------- Original Nodes List -------------------\n";
    while(main_iterator!=nodes_map.end())
    {
        log_file << main_iterator->first
        << ": ["<< main_iterator->second.co_ord[0] << ", "
        << main_iterator->second.co_ord[1] << ", "
        << main_iterator->second.co_ord[2] << "]\n";
        main_iterator++;
    }
    main_iterator = nodes_map.begin();
    // ------------------------------ Node duplication remover --------------------
    while(main_iterator != limit)
    {
        std::map<int, co_ordinates>::iterator nested_iterator = main_iterator;
        nested_iterator++;
        while(nested_iterator != nodes_map.end())
        {
            //compare with next
            if(co_ord_compare( main_iterator->second, nested_iterator->second ) == true)
            {
                // add co-ords to dupe data map
                node_duplication_data duplication;
                duplication.original_node = main_iterator->first;
                duplication.duplicated_node = nested_iterator->first;
                node_duplicated_data_map.insert(std::pair<int, node_duplication_data>(++duplicates_counter, duplication));
                //delete the duplicate
                std::map<int, co_ordinates>::iterator holder_iterator = nested_iterator;
                nested_iterator++;
                nodes_map.erase(holder_iterator);
                if (holder_iterator==limit)
                    limit = --nodes_map.end();
            }
            else
            {
                nested_iterator++;
            }
        }
        main_iterator++;
    }
    //print nodes map
    log_file << "--------------------- Updated Nodes List --------------------\n";
    main_iterator=nodes_map.begin();
    while(main_iterator != nodes_map.end())
    {
        log_file << main_iterator->first
        << ": ["
        << main_iterator->second.co_ord[0]
        << ", "
        << main_iterator->second.co_ord[1]
        << ", "
        << main_iterator->second.co_ord[2]
        << "]"
        << std::endl;
        main_iterator++;
    }
    //print dupe list
    log_file << "---------------------- Duplicated Nodes ---------------------\n";
    std::map<int, node_duplication_data>::iterator dupe_iterator = node_duplicated_data_map.begin();
    while (dupe_iterator != node_duplicated_data_map.end())
    {
        log_file << "Original:  " << dupe_iterator->second.original_node <<"    Duplicate:  " << dupe_iterator->second.duplicated_node << std::endl;
        dupe_iterator++;
    }
    // Material duplication remover ---------------------------------------------------------------------------------------
    std::map<int, std::string>::iterator material_iterator = materials_map.begin();
    int material_duplicates_counter = 0;
    std::map<int, std::string>::iterator materials_map_limit = --materials_map.end();
    while(material_iterator != materials_map_limit)
    {
        std::map<int, std::string>::iterator nested_material_iterator = material_iterator;
        nested_material_iterator++;
        while (nested_material_iterator != materials_map.end())
        {
            if (material_compare(material_iterator, nested_material_iterator) == true)
            {
                material_duplication_data identified_material_duplication;
                identified_material_duplication.original_material_ID = material_iterator->first;
                identified_material_duplication.duplicated_material_ID = nested_material_iterator->first;
                duplicated_material_data_map.insert(
                std::pair<int, material_duplication_data>(++material_duplicates_counter, identified_material_duplication));
                //delete the duplicate
                std::map<int, std::string>::iterator material_holder_iterator = nested_material_iterator;
                nested_material_iterator++;
                materials_map.erase(material_holder_iterator);
                if(material_holder_iterator==materials_map_limit)
                    materials_map_limit = --materials_map.end();
            }
            else
            {
                nested_material_iterator++;
            }
        }
        material_iterator++;
    }
    // Print duplication list of materials for DEBUG purposes
    log_file <<  "------------------- Duplicated Materials -------------------\n";
    std::map<int, material_duplication_data>::iterator material_dupe_iterator = duplicated_material_data_map.begin();
    while (material_dupe_iterator != duplicated_material_data_map.end())
    {
        log_file << "Original:  " << material_dupe_iterator->second.original_material_ID
        <<"    Duplicate:  " << material_dupe_iterator->second.duplicated_material_ID << std::endl;
        material_dupe_iterator++;
    }

    // end material duplication remover ------------------------------------------------------------------------------------
    exit_status = EXIT_SUCCESS;
    log_file.close();
    return exit_status;
}

void sort_element(std::map<int, elements>::iterator &element_iteration)
{
    int placeholder[3]={    element_iteration->second.element_properties[1],
                            element_iteration->second.element_properties[2],
                            element_iteration->second.element_properties[3]};
    int n = sizeof(placeholder)/sizeof(placeholder[0]);
    std::sort(placeholder, placeholder+n);
    element_iteration->second.element_properties[1]= placeholder[0];
    element_iteration->second.element_properties[2]= placeholder[1];
    element_iteration->second.element_properties[3]= placeholder[2];
}

bool redirect_elements( std::map<int, co_ordinates> &nodes_map,
                        std::map<int, node_duplication_data> &node_duplicated_data_map,
                        std::map<int, elements> &elements_map,
                        std::map<int, material_duplication_data> &duplicated_material_data_map)
{
    // First redirect the materials
    std::map<int, material_duplication_data>::iterator
    material_duplicates_iterator = duplicated_material_data_map.begin();
    while(material_duplicates_iterator != duplicated_material_data_map.end())
    {
        int current_duplicate_material = material_duplicates_iterator->second.duplicated_material_ID;
        int current_original_material = material_duplicates_iterator->second.original_material_ID;
        std::map<int, elements>::iterator second_elements_iterator = elements_map.begin();
        while(second_elements_iterator != elements_map.end())
        {
            if( second_elements_iterator->second.element_properties[0] == current_duplicate_material)
                second_elements_iterator->second.element_properties[0] = current_original_material;
            second_elements_iterator++;
        }
        material_duplicates_iterator++;
    }
    // Then redirect the elements
    std::map<int, node_duplication_data>::iterator duplicates_iterator = node_duplicated_data_map.begin();
    while(duplicates_iterator != node_duplicated_data_map.end())
    {
        int current_duplicate_node = duplicates_iterator->second.duplicated_node;
        int current_original_node = duplicates_iterator->second.original_node;
        std::map<int, elements>::iterator elements_iterator = elements_map.begin();
        while(elements_iterator != elements_map.end())
        {
            if (elements_iterator->second.element_properties[1]==current_duplicate_node)
            {
                elements_iterator->second.element_properties[1]= current_original_node;
            }
            if (elements_iterator->second.element_properties[2]==current_duplicate_node)
            {
                elements_iterator->second.element_properties[2]= current_original_node;
            }
            if (elements_iterator->second.element_properties[3]==current_duplicate_node)
            {
                elements_iterator->second.element_properties[3]= current_original_node;
            }
            sort_element(elements_iterator);
            elements_iterator++;
        }
        duplicates_iterator++;
    }
    return EXIT_SUCCESS;
}

bool compare_faces(const face_properties &face_in_question, const face_properties &iterator_face)
{
    if( (face_in_question.first_node == iterator_face.first_node)  &&  (face_in_question.second_node == iterator_face.second_node)  )
    {
        return true;
    }
    else
    {
        return false;
    }
}

void facemap_scanner(std::map<int, face_properties> &faces_map, int &face_map_index, face_properties &current_face)
{
    std::map<int, face_properties>::iterator face_map_searcher_iterator = faces_map.begin();
    bool face_already_exists = false;
    bool current_face_has_been_added = false;
    if (faces_map.empty() == false)
    {
        while (face_map_searcher_iterator != faces_map.end())
        {
            face_already_exists = compare_faces(current_face, face_map_searcher_iterator->second);
            if (face_already_exists == true) //update face data to change type and append parent materials
            {
                // if parent materials match -> make insider
                if (face_map_searcher_iterator->second.parent_materials[0] == current_face.parent_materials[0])
                {
                    face_map_searcher_iterator->second.face_type = 3; //face=insider
                    face_map_searcher_iterator->second.parent_materials[1] = current_face.parent_materials[0];
                }
                // if parent materials don't match -> make interface and add the second reference material
                else
                {
                    face_map_searcher_iterator->second.face_type = 2; //face=interface
                    face_map_searcher_iterator->second.parent_materials[1] = current_face.parent_materials[0];
                    int n = sizeof(face_map_searcher_iterator->second.parent_materials) / sizeof(face_map_searcher_iterator->second.parent_materials[0]);
                    std::sort(face_map_searcher_iterator->second.parent_materials, face_map_searcher_iterator->second.parent_materials + n);
                }
                current_face_has_been_added = true;
                face_map_searcher_iterator = faces_map.end(); //break from the loop as we have found the second and only face.
            }
            else
            {
                face_map_searcher_iterator++;
            }
        }
        if (current_face_has_been_added == false) // Add the element as no previous reference was found.
        {
            faces_map.insert(std::pair<int, face_properties>(++face_map_index, current_face));
        }

    }
    else // Add the element as there can't possibly be one already.
    {
        faces_map.insert(std::pair<int, face_properties>(++face_map_index, current_face));
    }
}

int generate_face_data(std::map<int, elements> &elements_map, std::map<int, face_properties> &faces_map)
{
    std::map<int, elements>::iterator element_iterator = elements_map.begin();
        face_properties current_face;
    int face_map_index = 0;// first face key will start at 1
    while (element_iterator != elements_map.end())
    {
        // Create Face 1 of element
        current_face.parent_materials[0] = element_iterator->second.element_properties[0];    //set the parent material
        current_face.parent_materials[1] = 0;
        current_face.first_node = element_iterator ->second.element_properties[1];
        current_face.second_node = element_iterator ->second.element_properties[2];
        current_face.face_type = 1;
        facemap_scanner(faces_map, face_map_index, current_face);// Place if not already in
        current_face.first_node = element_iterator ->second.element_properties[1];
        current_face.second_node = element_iterator ->second.element_properties[3];
        facemap_scanner(faces_map, face_map_index, current_face);// Place if not already in
        current_face.first_node = element_iterator ->second.element_properties[2];
        current_face.second_node = element_iterator ->second.element_properties[3];
        facemap_scanner(faces_map, face_map_index, current_face);// Place if not already in
        element_iterator++; // move onto next element
    }

    return EXIT_SUCCESS;
}

//takes the nodes map and faces map and outputs it to a geo file
int plot_geometry(    std::map<int, co_ordinates> &nodes_map,
                      std::map<int, face_properties> &faces_map,
                     const std::string &geo_file_path)
{
    std::ofstream geo_file;
    std::map<int, face_properties>::iterator face_iterator = faces_map.begin();
    //tries to insert every node referenced at least once, if it's already there, doesn't get inserted again.
    std::set<int> unique_nodes;
    while (face_iterator != faces_map.end())
    {
        if (face_iterator->second.face_type !=3)
        {
            unique_nodes.insert(face_iterator->second.first_node);
            unique_nodes.insert(face_iterator->second.second_node);
        }
        face_iterator++;
    }
    geo_file.open(geo_file_path);
    geo_file <<  "//========================== Border Nodes ==========================\n";
    auto i = unique_nodes.begin();
    while (i!=unique_nodes.end())
    {
        geo_file << "Point(" << *i << ") = {"
                 << nodes_map[*i].co_ord[0] << ", "
                 << nodes_map[*i].co_ord[1] << ", "
                 << nodes_map[*i].co_ord[2] << ", "
                 << "1.0" << "};\n";
        i++;
    }
    geo_file <<  "//========================== Shape Faces ===========================\n";
    face_iterator = faces_map.begin();
    while (face_iterator != faces_map.end())
    {
        if (face_iterator->second.face_type != 3)
        {
            geo_file    << "Line(" << face_iterator->first      << ") = {"
                        << face_iterator->second.first_node     << ", "
                        << face_iterator->second.second_node    << "};\n";
        }
        face_iterator++;
    }
    geo_file.close();
    return EXIT_SUCCESS;
}

bool check_for_completion(int required_count, int current_count)
{
    if (current_count == required_count)
    {
        return true;
    }
    else
    {
        return false;
    }
}

int generate_shapes(std::map<int, face_properties> &faces_map,
                    std::map<int, std::string> &materials_map,
                    const std::string &geo_file_path)
{
    std::ofstream geo_file;
    geo_file.open(geo_file_path, std::ios::out | std::ios::app);
    geo_file << "//======================== Material Shapes =========================\n";
    bool unable_to_complete_shape = false;
    //First remove the insider faces as we do not require them------------------------
    std::map<int, face_properties>::iterator faces_iterator = faces_map.begin();
    while (faces_iterator != faces_map.end())
    {
        if (faces_iterator->second.face_type == 3)
        {
            std::map<int, face_properties>::iterator placeholder = faces_iterator;
            faces_iterator++;
            faces_map.erase(placeholder);
        }
        else
        {
            faces_iterator++;
        }
    }
    //============================== Shape Finder Code ==============================
    std::map<int, std::string>::iterator material_being_shaped = materials_map.begin();
    int total_shapes_count = 0;
    bool error_reached = false;
    while (material_being_shaped != materials_map.end())
    {
        std::map<int, line> current_faces_map;
        faces_iterator = faces_map.begin();
        // Create a map of all the faces that are borders for this material type
        while (faces_iterator != faces_map.end())
        {
            // if face is child of parent material and is a boundary, add it to the allowable map for border placement.
            if ((faces_iterator->second.parent_materials[0] == material_being_shaped->first) || (faces_iterator->second.parent_materials[1] == material_being_shaped->first))
            {
                line temp;
                temp.a = faces_iterator->second.first_node;
                temp.b = faces_iterator->second.second_node;
                temp.used = false;
                current_faces_map.insert(std::pair<int, line>(faces_iterator->first, temp));
            }
            faces_iterator++;
        }
        // This section looks for loops
        bool new_start_found = false;
        int all_used_requirement = current_faces_map.size();
        int faces_used_counter = 0;
        int origin = 0;
        int current_head = 0;
        std::map<int, line>::iterator current_faces_iterator = current_faces_map.begin();
        // Internal loop to complete all material shapes
        while ((check_for_completion(all_used_requirement, faces_used_counter) == false) && (unable_to_complete_shape == false))
        {
            std::vector<int> current_shape_vector;
            while (new_start_found == false)
            {
                if (current_faces_iterator->second.used == true)
                {
                    if (current_faces_iterator != current_faces_map.end())
                    {
                        current_faces_iterator++;
                    }
                    else
                    {
                        unable_to_complete_shape = true; //Means all shapes were not completed without causing an error
                    }
                }
                else
                {
                    new_start_found = true;
                    origin = current_faces_iterator->second.a;
                    current_head = current_faces_iterator->second.b;
                    current_shape_vector.push_back(current_faces_iterator->first);
                    current_faces_iterator->second.used = true;
                    faces_used_counter++;
                }
            }
            bool current_shape_complete = false;
            bool timed_out = false;
            int submaterial_loop_counter = 0;
            while ((current_shape_complete == false) && (unable_to_complete_shape == false))
            {
                if (current_faces_iterator->second.used == false)
                {
                    if (current_head == current_faces_iterator->second.a)
                    {
                        current_head = current_faces_iterator->second.b;
                        if (current_head == origin) //reached starting point
                        {
                            current_shape_complete = true;
                            total_shapes_count++;
                        }
                        current_shape_vector.push_back(current_faces_iterator->first);
                        current_faces_iterator->second.used = true;
                        faces_used_counter++;
                        current_faces_iterator = current_faces_map.begin(); //Send back to start to check for previously skipped entries that might be relevant now
                    }
                    else if (current_head == current_faces_iterator->second.b)
                    {
                        current_head = current_faces_iterator->second.a;
                        if (current_head == origin) //reached starting point
                        {
                            current_shape_complete = true;
                            total_shapes_count++;
                        }
                        current_shape_vector.push_back(-(current_faces_iterator->first));
                        current_faces_iterator->second.used = true;
                        faces_used_counter++;
                        current_faces_iterator = current_faces_map.begin();
                    }
                }
                if (current_faces_iterator != current_faces_map.end())
                {
                    current_faces_iterator++;
                }
                else
                {
                    if (current_shape_complete == false)
                    {
                        unable_to_complete_shape = true; //never found origin before faces map ended
                    }
                }
                submaterial_loop_counter++;
                if (submaterial_loop_counter >= LOOPTIMEOUT)
                {
                    unable_to_complete_shape = true; //timeout
                }
            }
            if (current_shape_complete == true)
            {
                // plot the vector with a unique curve loop ID & make it a child of physical group
                std::ostringstream curve;
                curve << "Curve Loop(" << total_shapes_count << ") = {";
                auto vector_iterator = current_shape_vector.begin();
                auto final = current_shape_vector.end();
                final--;
                while (vector_iterator != final)
                {
                    curve << *vector_iterator << ", ";
                    vector_iterator++;
                }
                curve << *vector_iterator << "};\n";
                std::string curve_loop = curve.str();
                geo_file << curve_loop;
                geo_file << "Plane Surface(" << total_shapes_count << ") = {" << total_shapes_count << "};\n";
                geo_file << "Physical Surface(\"" << material_being_shaped->second << " " << total_shapes_count << "\") = {" << total_shapes_count << "};\n";
                new_start_found = false;
            }
        }
        if (unable_to_complete_shape == true)
        {
            error_reached = true;
        }
        else
        {
            error_reached = false;
        }
        material_being_shaped++;
    }
    if (error_reached == false)
    {
        return EXIT_SUCCESS;
    }
    else
    {
        return EXIT_FAILURE;
    }
}

/* One cannot simply swap X&Y co-ordinates in order to
lie the outputs down, in addition to swapping the axes
we must reflect about the x axis and offset the shape
back to it's original height.
*/
int swap_axes(std::map<int, co_ordinates> &nodes_map)
{
    //first determine the offset of the grid.
    std::map<int, co_ordinates>::iterator nodes_it = nodes_map.begin();
    double offset = 0;
    while (nodes_it!=nodes_map.end())
    {
        if (nodes_it->second.co_ord[0]>offset)
            offset = nodes_it->second.co_ord[0];
        nodes_it++;
    }
    //then change [ x1, y1 ] to [y0,( -x0 + offset)]
    nodes_it = nodes_map.begin();
    while (nodes_it!=nodes_map.end())
    {
        double x0 = nodes_it->second.co_ord[0];
        double y0 = nodes_it->second.co_ord[1];
        nodes_it->second.co_ord[0] = y0;
        nodes_it->second.co_ord[1] = -x0+offset;
        nodes_it++;
    }
    return EXIT_SUCCESS;
}