/**
 * Author:      H.F. Herbst
 * Origin:      Stellenbosch University
 * For:         IARPA SuperTools Project
 * license:     MIT
 * Description: Function definitions for three-dimensional model generation
 */

#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <boost/algorithm/string.hpp>
#include "geofile_operations.hpp"
#include "gdsCpp.hpp"
#include "three_dim_model.hpp"
#include "section.hpp"
#include "clipper.hpp"
#include "geofile_operations.hpp"

int MODEL3D::three_dim_model::generate_model(
const std::string &gds_path,
const std::string &pif_path,
const std::string &py_path,
const std::string &cont_path,
const bool        &multi_flag)
{
    gdscpp gds_file;
    section_data section_polygon_only;
    enable_multiprocessing = multi_flag;
    std::vector<std::string> split_string_vector;
    boost::split(split_string_vector, py_path, boost::is_any_of("./\\"));
    cell_name = *std::prev(split_string_vector.end(),2);;
    if(gds_file.import(gds_path)==EXIT_SUCCESS)
    {
        import_process_info(pif_path);
        if(load_etch_contour_map(cont_path)==EXIT_SUCCESS)
        {
            // Populate polygon map with all GDS data
            std::cout<<"Beginning GDS expansion process..."<<std::endl;
            if(section_polygon_only.extract_all_polygons(gds_file, polygons_map)==EXIT_SUCCESS)
                std::cout<<"GDS expansion completed."<<std::endl;
            calculate_all_layer_bounds();
            // Convert map to clipper library compatible form
            std::cout<<"Converting polygons to clipper format..."<<std::endl;
            synchronize_clipper_polygons_map();
            std::cout<<"Polygon map ready."<<std::endl;
            // Generate a volume for each layer
            std::cout<< "# ------------------------- Generating 3D Layers ------------------------- #"<<std::endl;
            auto layer_end = polygons_map.end();
            int layer_count = 0;
            size_t total_layers = polygons_map.size();
            // if all layer contours are loaded
            // else state that a layer contour is missing
            // int p_cnt = 0;
            // int ln_cnt = 0;
            // int cl_cnt = 0;
            // int ps_cnt = 0;
            // int sl_cnt = 0;
            // int vl_cnt = 0;
            initialize_python_script();
            for (auto layer = polygons_map.begin(); layer != layer_end; layer++)
            {
                std::cout << "Generating Layer "
                << ++layer_count << "/" << total_layers
                << "\t---> ";
                if(proc_info_map.find(layer->first)!=proc_info_map.end())
                    generate_layer(layer->first);
                std::cout << "Done." << std::endl;
            }
            // generate_layer(60);
            python_script_conclude();
            python_script_write(py_path);
            std::cout<< "# ----------------------- Model Generation Complete ---------------------- #"<<std::endl;
            return EXIT_SUCCESS;
        }
        else
            return EXIT_FAILURE;
    }
    else
        return EXIT_FAILURE;
}

//Overwrites clipper_polygons_map with polygons_map data
void MODEL3D::three_dim_model::synchronize_clipper_polygons_map()
{
    clipper_polygons_map.clear();
    auto layer_it_end = polygons_map.end();
    // for each layer
    for (auto layer_it = polygons_map.begin(); layer_it != layer_it_end; layer_it++)
    {
        // for each polygon
        auto polygon_it_end = layer_it->second.end();
        for (auto polygon_it = layer_it->second.begin(); polygon_it != polygon_it_end; polygon_it++)
        {
            ClipperLib::Path placeholder_path;              // Path = polygon in this case
            co_ord_to_path(*polygon_it, placeholder_path);
            if (clipper_polygons_map.count(layer_it->first) > 0) // Check map for key
            {
                clipper_polygons_map[layer_it->first].push_back(placeholder_path);
            }
            else
            {
                std::vector<ClipperLib::Path> path_vec;
                path_vec.push_back(placeholder_path);
                clipper_polygons_map.insert({layer_it->first, path_vec});
            }
        }
    }
}

//Overwrites polygons_map with clipper_polygons_map data.
void MODEL3D::three_dim_model::synchronize_polygon_map()
{
    polygons_map.clear();
    auto clipper_polymap_end = clipper_polygons_map.end();
    //For each layer
    for (   auto clipper_polymap_it = clipper_polygons_map.begin();
            clipper_polymap_it != clipper_polymap_end;
            clipper_polymap_it++)
    {
        // Create the polygons' vector for that layer
        std::vector<co_ord> layer_polygons;
        // For each polygon (clipper::Path) , convert to co_ord (vector of x and vector of y),
        // Then add co-ord to vector of co_ord.
        auto clipper_path_end = clipper_polymap_it->second.end();
        for (   auto clipper_path_it = clipper_polymap_it->second.begin();
                clipper_path_it != clipper_path_end;
                clipper_path_it++)
        {
            //convert polygon to co-ord
            co_ord p_hold_co_ord;
            auto i_end = clipper_path_it->end();
            for (auto i = clipper_path_it->begin(); i != i_end; i++)
            {
                p_hold_co_ord.x.push_back(i->X);
                p_hold_co_ord.y.push_back(i->Y);
            }
            //Push to co-ord to vector of<co_ord> for the layer
            layer_polygons.push_back(p_hold_co_ord);
        }
        // Finally, push the vector of co ord to the polygon map.
        polygons_map.insert({clipper_polymap_it->first, layer_polygons});
    }
}


void MODEL3D::three_dim_model::co_ord_to_path(  const co_ord &in_vec,
                                                ClipperLib::Path &out_path)
{
    // for each point convert to clipper point
    // then push point to path
    ClipperLib::Path holder_path;
    auto point_it_y = in_vec.y.begin();
    auto point_it_x_end = in_vec.x.end();
    for (auto point_it_x = in_vec.x.begin(); point_it_x != point_it_x_end; point_it_x++)
    {
        ClipperLib::IntPoint current_point;
        current_point.X = *point_it_x;
        current_point.Y = *point_it_y;
        holder_path.push_back(current_point);
        point_it_y++;
    }
    out_path = holder_path;
}

// This function converts a ClipperLib::Path to co_ord.
// (polygon of one type to polygon of another type)
void MODEL3D::three_dim_model::path_to_co_ord(const ClipperLib::Path &in_path, co_ord &out_co_ord)
{
    co_ord holder_co_ord;
    //For each clipper point, push onto co_ord
    auto path_it_end = in_path.end();
    for (auto i = in_path.begin(); i != path_it_end; i++)
    {
        holder_co_ord.x.push_back(i->X);
        holder_co_ord.y.push_back(i->Y);
    }
    out_co_ord = holder_co_ord;
}

int MODEL3D::three_dim_model::import_process_info(const std::string &pif_path)
{
    std::fstream geo_file;
    std::string file_buffer;
    geo_file.open(pif_path, std::ios::in);
    if (geo_file.is_open())
    {
        int line_number =1;
        while (getline(geo_file, file_buffer))
        {
            if (file_buffer[0]!='*')
            {
                std::vector<std::string> split_string_vector;
                boost::split(split_string_vector, file_buffer, boost::is_any_of("="));
                GEO::trim_string_vector(split_string_vector);
                if (*split_string_vector.begin()=="$Layer")
                {
                    import_layer_block(geo_file, file_buffer, line_number);
                }
            }
            line_number++;
        }
        geo_file.close();
    }
    return EXIT_SUCCESS;
}

MODEL3D::input_type MODEL3D::three_dim_model::hash_prop(std::string const& inString)
{
    if(inString == "Number") return eNumber;
    if(inString == "Name") return eName;
    if(inString == "MaskType") return eMask_type;
    if(inString == "ZStart") return eZ_Start;
    if(inString == "Thickness") return eThickness;
    if(inString == "Material") return eMaterial;
    if(inString == "$End") return eError;
    return eDefault;
}

int MODEL3D::three_dim_model::import_layer_block(std::fstream &infile,
                                                 std::string &file_buffer,
                                                 int  &line_number)
{
    int elements_loaded = 0;
    std::vector<std::string> split_string_vector;
    int layer_number=0;
    Layer_prop layer_block;
    while (elements_loaded < 6)
    {
        getline(infile, file_buffer);
        line_number++;
        boost::split(split_string_vector, file_buffer, boost::is_any_of("="));
        GEO::trim_string_vector(split_string_vector);
        input_type intype = hash_prop(*split_string_vector.begin());
        switch (intype)
        {
        case eNumber:
            if(GEO::is_integer(*std::prev(split_string_vector.end(),1)))
            {
                layer_number = std::stoi(*std::prev(split_string_vector.end(),1));
                elements_loaded++;
            }
            else
            {
                std::cout << "Error: Number is not an integer in .PF file." << std::endl;
                std::cout << "Line " << line_number << ". " << std::endl;
                return EXIT_FAILURE;
            }
            break;
        case eName:
            layer_block.name = *std::prev(split_string_vector.end(),1);
            elements_loaded++;
            break;
        case eMask_type:
            if(GEO::is_integer(*std::prev(split_string_vector.end(),1)))
            {
                layer_block.mask_type = std::stoi(*std::prev(split_string_vector.end(),1));
                elements_loaded++;
            }
            else
            {
                std::cout << "Error: Number is not an integer in .PF file." << std::endl;
                std::cout << "Line " << line_number << ". " << std::endl;
                return EXIT_FAILURE;
            }
            break;
        case eZ_Start:
            if(GEO::is_integer(*std::prev(split_string_vector.end(),1)))
            {
                layer_block.z_start = std::stoi(*std::prev(split_string_vector.end(),1));
                elements_loaded++;
            }
            else
            {
                std::cout << "Error: Number is not an integer in .PF file." << std::endl;
                std::cout << "Line " << line_number << ". " << std::endl;
                return EXIT_FAILURE;
            }
            break;
        case eThickness:
            if(GEO::is_integer(*std::prev(split_string_vector.end(),1)))
            {
                layer_block.thickness = std::stoi(*std::prev(split_string_vector.end(),1));
                elements_loaded++;
            }
            else
            {
                std::cout << "Error: Number is not an integer in .PF file." << std::endl;
                std::cout << "Line " << line_number << ". " << std::endl;
                return EXIT_FAILURE;
            }
            break;
        case eMaterial:
            layer_block.material = *std::prev(split_string_vector.end(),1);
            elements_loaded++;
            break;
        case eError:
            std::cout << "Error: End command reached before 6 entries in process information block." << std::endl;
            std::cout << "Line " << line_number << ". " << std::endl;
            return EXIT_FAILURE;
            break;
        default:
            std::cout   << "Warning: Unknown property specified in .PF file ["
                        << *split_string_vector.begin()<<"]." << std::endl;
            std::cout << "Line " << line_number << ". " << std::endl;
            break;
        }
    }
    if(proc_info_map.find(layer_number)==proc_info_map.end())
        proc_info_map.insert({layer_number, layer_block});
    getline(infile, file_buffer);
    line_number++;
    boost::split(split_string_vector, file_buffer, boost::is_any_of("="));
    GEO::trim_string_vector(split_string_vector);
    if (split_string_vector[0]=="$End")
        return EXIT_SUCCESS;
    else
        return EXIT_FAILURE;
}

int MODEL3D::three_dim_model::generate_layer(const int &layer_number)
{
    switch (proc_info_map[layer_number].mask_type)
    {
    case 0://standard
        generate_non_fill_layer(layer_number, proc_info_map[layer_number].mask_type);
        break;
    case 1://inverted
        generate_non_fill_layer(layer_number, proc_info_map[layer_number].mask_type);
        break;
    case 2://filled
        generate_filled_layer(layer_number);
        break;
    case 3://junction
        generate_non_fill_layer(layer_number, proc_info_map[layer_number].mask_type);
        break;
    case 4://auxiliary, do nothing.
        break;
    default:
        std::cout << "Error. Unknown layer type." << std::endl;
        return EXIT_FAILURE;
        break;
    }
    return EXIT_SUCCESS;
}

int MODEL3D::three_dim_model::generate_non_fill_layer(const int &layer_number, const int &mask_type)
{
    double cont_wid = std::prev(etch_contour_map[layer_number].end(),1)->x;
    double countour_width = std::round(cont_wid*1.4);//40% tolerance to avoid bunching
    double foot_adjustment = std::round(-(countour_width/10));
    ClipperLib::Paths positives;
    auto poly_end = clipper_polygons_map[layer_number].end();
    // For each polygon on this layer.
    for (   auto poly_it = clipper_polygons_map[layer_number].begin();
            poly_it != poly_end; poly_it++)
    {
        ClipperLib::Path my_path = *poly_it;
        ClipperLib::Paths positive;
        ClipperLib::SimplifyPolygon(my_path, positive, ClipperLib::pftNonZero);
        if(positive.size()>0)
        {
            auto pos_end = positive.end();
            for (auto pos_it = positive.begin(); pos_it != pos_end; pos_it++)
            {
                positives.insert(positives.end(),positive.begin(),positive.end());
            }
        }
    }
    ClipperLib::Clipper my_mask;
    ClipperLib::Paths solution_paths;
    ClipperLib::ClipperOffset co;
    // shift by foot adjustment
    co.Clear();
    co.AddPaths(positives, ClipperLib::jtMiter, ClipperLib::etClosedPolygon);
    solution_paths.clear();
    co.Execute(solution_paths, foot_adjustment);
    // grow by contour width
    co.Clear();
    co.AddPaths(solution_paths, ClipperLib::jtMiter, ClipperLib::etClosedPolygon);
    solution_paths.clear();
    co.Execute(solution_paths, countour_width);
    // reduce by contour width (creates correct contour profile)
    co.Clear();
    if(round_concave_corners==true)
        co.AddPaths(solution_paths, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
    else
        co.AddPaths(solution_paths, ClipperLib::jtMiter, ClipperLib::etClosedPolygon);
    solution_paths.clear();
    // co.ArcTolerance = 0.25;
    co.Execute(solution_paths, -countour_width);
    // filter into primary and secondary shapes
    ClipperLib::Paths primary_paths, secondary_paths;
    primary_paths = solution_paths;
    bool has_holes = filter_paths(primary_paths, secondary_paths);
    tidy_paths(primary_paths);
    tidy_paths(secondary_paths);
    repair_circles(primary_paths);
    repair_circles(secondary_paths);
    py_script.push_back("");
    py_script.push_back("    # Layer " + std::to_string(layer_number) );
    py_script.push_back("    current_layer =  " + std::to_string(layer_number) );
    py_script.push_back("    print(f'Generating Layer {current_layer}.')");
    generate_freecad_solid(primary_paths, secondary_paths, has_holes, layer_number, mask_type);
    // if (primary_paths.size()>0)
    //     clipper_paths_to_gds(primary_paths, "Primary_paths", layer_number);
    // if (secondary_paths.size()>0)
    //     clipper_paths_to_gds(secondary_paths, "Secondary_paths", layer_number);
    return EXIT_SUCCESS;
}

int MODEL3D::three_dim_model::generate_filled_layer(const int &layer_number)
{
    if(etch_contour_map[layer_number].size() > 0)
    {
        double cont_wid = std::prev(etch_contour_map[layer_number].end(),1)->x;
        double countour_width = std::round(cont_wid*1.4);//40% tolerance to avoid bunching
        double foot_adjustment = std::round(-(countour_width/10));
        // make bounding polygon (Will be used for OpenCascade Boolean difference)
        ClipperLib::IntPoint a,b,c,d;//clockwise
        a.X = layer_boundary_map[layer_number][0];
        a.Y = layer_boundary_map[layer_number][1];
        b.X = layer_boundary_map[layer_number][0];
        b.Y = layer_boundary_map[layer_number][3];
        c.X = layer_boundary_map[layer_number][2];
        c.Y = layer_boundary_map[layer_number][3];
        d.X = layer_boundary_map[layer_number][2];
        d.Y = layer_boundary_map[layer_number][1];
        // GEO::geofile layer_parent, child_polyhedron;
        // build_layer_box(layer_number, a, b, c, d, layer_parent);
        ClipperLib::Path bp_vec = {a,b,c,d};
        // Determine ground plane holes
        ClipperLib::Paths negatives;
        auto poly_end = clipper_polygons_map[layer_number].end();
        // For each polygon on this layer.
        for (   auto poly_it = clipper_polygons_map[layer_number].begin();
                poly_it != poly_end; poly_it++)
        {
            ClipperLib::Path my_path = *poly_it;
            ClipperLib::Paths negative;
            ClipperLib::SimplifyPolygon(my_path, negative, ClipperLib::pftNonZero);
            if(negative.size()>0)
            {
                auto neg_end = negative.end();
                for (auto neg_it = negative.begin(); neg_it != neg_end; neg_it++)
                {
                    negatives.insert(negatives.end(),negative.begin(),negative.end());
                }
            }
        }
        ClipperLib::Clipper my_mask;
        ClipperLib::Paths solution_paths;
        // clipper_paths_to_gds(negatives)
        my_mask.AddPath(bp_vec, ClipperLib::ptSubject, true);
        my_mask.AddPaths(negatives, ClipperLib::ptClip, true);
        my_mask.Execute(    ClipperLib::ctDifference,
                            solution_paths,
                            ClipperLib::pftNonZero,
                            ClipperLib::pftNonZero);

        ClipperLib::ClipperOffset co;
        // shift by foot adjustment
        co.Clear();
        co.AddPaths(solution_paths, ClipperLib::jtMiter, ClipperLib::etClosedPolygon);
        solution_paths.clear();
        co.Execute(solution_paths, foot_adjustment);
        // grow by contour width
        co.Clear();
        co.AddPaths(solution_paths, ClipperLib::jtMiter, ClipperLib::etClosedPolygon);
        solution_paths.clear();
        co.Execute(solution_paths, countour_width);
        // reduce by contour width (creates correct contour profile)
        co.Clear();
        if(round_concave_corners==true)
            co.AddPaths(solution_paths, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
        else
            co.AddPaths(solution_paths, ClipperLib::jtMiter, ClipperLib::etClosedPolygon);
        solution_paths.clear();
        co.Execute(solution_paths, -countour_width);
        // filter into primary and secondary shapes
        ClipperLib::Paths primary_paths, secondary_paths;
        primary_paths = solution_paths;
        // clipper_paths_to_gds(primary_paths, "Before_filter", layer_number);
        bool has_holes = filter_paths(primary_paths, secondary_paths);
        tidy_paths(primary_paths);
        tidy_paths(secondary_paths);
        // clipper_paths_to_gds(primary_paths, "Primary", layer_number);//DEBUG
        // clipper_paths_to_gds(secondary_paths, "Secondary", layer_number);
        // Compartmentalize the layer
        py_script.push_back("");
        py_script.push_back("    # Layer " + std::to_string(layer_number) );
        py_script.push_back("    current_layer =  " + std::to_string(layer_number) );
        py_script.push_back("    print(f'Generating Layer {current_layer}.')");
        // generate additive-block
        double b_e_factor = (std::prev(etch_contour_map[layer_number].end(),1)->x)*BLOCK_EXPANSION_FACTOR;
        //chose ceiling for consistency and preference to rather have more material than less
        //Rounding not necessary but simply preferred in this case.
        double origin_x = std::ceil(a.X - b_e_factor);
        double origin_y = std::ceil(a.Y - b_e_factor);
        double origin_z = proc_info_map[layer_number].z_start;
        double delta_x =  std::ceil((c.X - a.X)+2*b_e_factor);
        double delta_y =  std::ceil((c.Y - a.Y)+2*b_e_factor);
        double delta_z = std::prev(etch_contour_map[layer_number].end(),1)->z;
        std::string additive_block = "    additive_block_"
        +std::to_string(layer_number)+" = Part.makeBox("
        +std::to_string(delta_x)+","
        +std::to_string(delta_y)+","
        +std::to_string(delta_z)+
        ",FreeCAD.Vector("
        +std::to_string(origin_x)+","
        +std::to_string(origin_y)+","
        +std::to_string(origin_z)+
        "), FreeCAD.Vector(0,0,1))";
        py_script.push_back(additive_block);
        generate_freecad_fill(bp_vec, primary_paths, secondary_paths, layer_number);
        return EXIT_SUCCESS;
    }
    else
    {
        std::cout << "Error. Contour for layer " << layer_number << " not found." << std::endl;
        return EXIT_FAILURE;
    }
}

void MODEL3D::three_dim_model::remove_outer_path(ClipperLib::Paths &paths_vec)
{
    if (paths_vec.size()>1)
    {
        std::vector<double> areas;
        // for each path calculate area
        auto paths_vec_end = paths_vec.end();
        for (auto paths_vec_it = paths_vec.begin(); paths_vec_it != paths_vec_end; paths_vec_it++)
            areas.push_back(std::abs(ClipperLib::Area(*paths_vec_it)));
        auto it = std::max_element(std::begin(areas), std::end(areas));
        int index = std::distance(areas.begin(), it);
        paths_vec.erase(paths_vec.begin()+index);
    }
}

//Converts specified polygon map layer to GDS output file.
void MODEL3D::three_dim_model::polygon_layer_to_gds(const int &layer_number)
{
    gdscpp my_polygons;
    gdsSTR main;
    main.name = std::to_string(layer_number);
    auto poly_end = polygons_map[layer_number].end();
    for (auto poly_it = polygons_map[layer_number].begin(); poly_it != poly_end; poly_it++)
    {
        main.BOUNDARY.push_back(drawBoundary( layer_number , poly_it->x, poly_it->y));
    }
    my_polygons.setSTR(main);
    my_polygons.write("data/"+main.name+".gds");
}

//Converts specified path to GDS output file for debugging
void MODEL3D::three_dim_model::clipper_path_to_gds(const ClipperLib::Path &in_path, const std::string gds_name)
{
    gdscpp my_polygons;
    gdsSTR main;
    main.name = "Debug_polygon";
    if (gds_name.size()>0)
      main.name = gds_name;
    //convert path to dual vectors of co_ord
    std::vector<int> cor_x;
    std::vector<int> cor_y;
    auto in_end = in_path.end();
    for (auto i = in_path.begin(); i != in_end; i++)
    {
        cor_x.push_back(i->X);
        cor_y.push_back(i->Y);
    }
    main.BOUNDARY.push_back(drawBoundary( 1 , cor_x, cor_y));
    my_polygons.setSTR(main);
    my_polygons.write("data/"+main.name+".gds");
}

//Converts entire polygon map to gds output file.
void MODEL3D::three_dim_model::polygon_map_to_gds()
{
    gdscpp my_polygons;
    gdsSTR main;
    main.name = "Polygon_Map";
    auto poly_map_end = polygons_map.end();
    // For each layer
    for (auto layer_it = polygons_map.begin(); layer_it != poly_map_end; layer_it++)
    {
        //For each polygon
        auto poly_end = layer_it->second.end();
        for (auto poly_it = layer_it->second.begin(); poly_it != poly_end; poly_it++)
        {
            main.BOUNDARY.push_back(drawBoundary( layer_it->first , poly_it->x, poly_it->y));
        }
    }
    my_polygons.setSTR(main);
    my_polygons.write("data/"+main.name+".gds");
}

void MODEL3D::three_dim_model::calculate_all_layer_bounds()
{
    initialize_layer_boundary_map();
    bool empty[4] = {true, true, true, true};
    //scan through each layer
    auto layers_end = polygons_map.end();
    for (auto i = polygons_map.begin(); i != layers_end; i++)
    {
        //scan through each polygon
        auto polygons_vector_end = i->second.end();
        for (auto j = i->second.begin(); j != polygons_vector_end; j++)
        {
            //scan through each x point
            auto k_end = j->x.end();
            for (auto k = j->x.begin(); k != k_end; k++)
            {
                //min  x check
                if (empty[0]==true)
                {
                    layer_boundary_map[i->first][0] = *k;
                    empty[0] = false;
                }
                else if(*k < layer_boundary_map[i->first][0])
                {
                    layer_boundary_map[i->first][0] = *k;
                }
                //max x check
                if (empty[2]==true)
                {
                    layer_boundary_map[i->first][2] = *k;
                    empty[2] = false;
                }
                else if(*k > layer_boundary_map[i->first][2])
                {
                    layer_boundary_map[i->first][2] = *k;
                }
            }
            //scan through each y point
            auto n_end = j->y.end();
            for (auto n = j->y.begin(); n != n_end; n++)
            {
                //min  y check
                if (empty[1]==true)
                {
                    layer_boundary_map[i->first][1] = *n;
                    empty[1] = false;
                }
                else if(*n < layer_boundary_map[i->first][1])
                {
                    layer_boundary_map[i->first][1] = *n;
                }
                //max y check
                if (empty[3]==true)
                {
                    layer_boundary_map[i->first][3] = *n;
                    empty[3] = false;
                }
                else if(*n > layer_boundary_map[i->first][3])
                {
                    layer_boundary_map[i->first][3] = *n;
                }
            }
        }
    }
}

void MODEL3D::three_dim_model::initialize_layer_boundary_map()
{
    layer_boundary_map.clear();
    auto i_end = polygons_map.end();
    std::vector<int> spots = {0,0,0,0};
    for (auto i = polygons_map.begin(); i != i_end; i++)
    {
        layer_boundary_map.insert({i->first, spots});
    }
}

void MODEL3D::three_dim_model::print_polygons_map()
{
    auto pm_end = polygons_map.end();
    for (auto i = polygons_map.begin(); i != pm_end; i++)
    {
        std::cout<<"Layer "<<std::to_string(i->first)<<"------------"<<std::endl;
        auto poly_end = i->second.end();
        int poly_count = 0;
        for (auto j = i->second.begin(); j != poly_end; j++)
        {
            std::cout<<"Polygon "<<std::to_string(++poly_count)<<"~~~~~~~~~~~~"<<std::endl;
            auto k_end = j->x.end();
            auto k_2 = j->y.begin();
            int point_count = 0;
            for (auto k = j->x.begin(); k != k_end; k++)
            {
                std::cout << "P" << std::to_string(++point_count)
                << ": X=" << *k << "|Y=" << *k_2 << std::endl;
                k_2++;
            }
        }
    }
}

void MODEL3D::three_dim_model::clipper_paths_to_gds(    const ClipperLib::Paths &target_paths,
                                                        const std::string &gds_name,
                                                        const int &layer_number )
{
    gdscpp my_polygons;
    gdsSTR main;
    main.name = gds_name;
    std::vector<co_ord> polygons;
    auto paths_it_end = target_paths.end();
    for (auto path_it = target_paths.begin(); path_it != paths_it_end; path_it++)
    {
        co_ord p_hold_poly;
        path_to_co_ord(*path_it, p_hold_poly);
        polygons.push_back(p_hold_poly);
    }
    auto polygons_it_end = polygons.end();
    for (auto polygon_it = polygons.begin(); polygon_it != polygons_it_end; polygon_it++)
    {
        main.BOUNDARY.push_back(drawBoundary( layer_number , polygon_it->x, polygon_it->y));
    }
    my_polygons.setSTR(main);
    my_polygons.write("data/"+main.name+".gds");
}

void MODEL3D::three_dim_model::close_paths(ClipperLib::Paths &paths_vec)
{
    //for each path
    auto path_it_end = paths_vec.end();
    for (auto path_it = paths_vec.begin(); path_it != path_it_end; path_it++)
    {
        //copy beginning to end
        auto point = *path_it->begin();
        path_it->push_back(point);
    }
}


bool MODEL3D::three_dim_model::clipper_path_is_equal(
const ClipperLib::Path &a,
const ClipperLib::Path &b)
{
    if( (a.empty())||(b.empty()))//safeguard
        return false;
    if(a.size()==b.size())
    {
        auto a_it_end = a.end();
        for ( auto a_it=a.begin(); a_it!=a_it_end; a_it++)
        {
            int index = std::distance(a.begin(), a_it);
            if( (a[index].X != b[index].X)||(a[index].Y != b[index].Y))
                return false;
        }
        return true;
    }
    else
        return false;
}

// For each path in primary paths:
// Take first point on path and compare against all other paths with point-
// in-polygon function. If the point lies in another polygon, move path in
// question out of primary paths and into secondary paths. It is a hole.
bool MODEL3D::three_dim_model::filter_paths(
ClipperLib::Paths &primary_paths,
ClipperLib::Paths &secondary_paths)
{
    auto outer_it_end = primary_paths.end();
    for ( auto outer_it = primary_paths.begin();
         outer_it != outer_it_end;
         outer_it++)
    {
        //compare against every other polygon
        auto inner_it_end = primary_paths.end();
        for ( auto inner_it = primary_paths.begin();
             inner_it != inner_it_end;
             inner_it++)
        {
            if(inner_it!=outer_it)
            {
                ClipperLib::IntPoint check;
                // Pick a point of the polygon
                check.X = inner_it->begin()->X;
                check.Y = inner_it->begin()->Y;
                if(ClipperLib::PointInPolygon(check, *outer_it)==1)
                    secondary_paths.push_back(*inner_it);
            }
        }
    }
    // remove the identified paths from the main vector
    if(!secondary_paths.empty())
    {
        for ( auto holes_it = secondary_paths.begin();
        holes_it != secondary_paths.end();
        holes_it++)
        {
            auto main_end = primary_paths.end();
            for ( auto main_it = primary_paths.begin(); main_it != primary_paths.end(); main_it++)
            {
                if(clipper_path_is_equal(*holes_it, *main_it))
                {
                    primary_paths.erase(main_it);
                    //break the loop as this shape was found
                    main_it = std::prev(primary_paths.end(),1);
                }
            }
        }
        return true;
    }
    return false;
}

int  MODEL3D::three_dim_model::load_etch_contour_map(const std::string &cont_path)
{
    //std::map<int, Layer_prop> proc_info;
    //std::map<int, std::vector<GEO::point>> etch_contour_map;
    auto proc_map_end = proc_info_map.end();
    for ( auto proc_map_it = proc_info_map.begin();
         proc_map_it != proc_map_end;
         proc_map_it++)
    {
        // get layer thickness
        int thickness = proc_map_it->second.thickness;
        std::string thickness_string = std::to_string(thickness);
        std::string contour_path = cont_path;
        if(*contour_path.rbegin()=='/')
            contour_path.pop_back();
        contour_path = cont_path + "/" + thickness_string + "_contour.geo";
        // std::string contour_path = "../data/3DGen/"+ thickness_string +"_contour.geo";//TODO: Fix this to a user input
        // if file exists open else return error
        GEO::geofile contour;
        if(contour.import_geofile(contour_path)==EXIT_SUCCESS)
        {
            std::vector<GEO::point> contour_vec;
            contour.pull_points(contour_vec);
            etch_contour_map.insert({proc_map_it->first, contour_vec});
        }
        else
        {
            std::cout<<"Error: Contour import failed. ["<<contour_path<<"]"<<std::endl;
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

/**
 * [build_polyhedron - Creates 3D polyhedron based off the following]
 * @param   contour             [The base shape outline which the
 *                              etch-contour is attached to]
 * @param   layer_number        [The layer number which the
 *                              polyhedron belongs to]
 * @param   unique_ID           [The ID of the polyhedron on this layer]
 * @param   is_positive         [Dictates whether the shape is additive or
 *                              subtractive in the greater shape]
 * @param   outward_facing      [Dictates whether the etch countour should
 *                              be placed towards the outside of shape]
 * @param   generated_geofile   [the GEO::geofile containing the generated polyhedron.
 * @return                      [0 - Exit Success or
 *                              1 - Exit Failure]
 */
int MODEL3D::three_dim_model::build_polyhedron(
const ClipperLib::Path &contour,
const int &layer_number,
const int &unique_ID,
const bool &is_positive,
const bool &outward_facing,
GEO::geofile &generated_geofile)
{
    if(contour.size()<3)
    {
        std::cout << "Error: Invalid polyhedron supplied." << std::endl;
        return EXIT_FAILURE;
    }
    GEO::geofile polyhedron;
    std::map<int, GEO::point> poly_points_map;
    std::map<int, GEO::line> poly_lines_map;
    std::map<int, std::vector<int>> poly_curve_loops_map;
    std::map<int, int> poly_plane_surfaces_map;
    std::map<int, std::vector<int>> poly_surface_loops_map;
    std::map<int, int> poly_volumes_map;
    std::vector<int> roof_line_vector;
    int hp_point = pnt_cnt;//highest point from previous shape
    int hp_line = ln_cnt;
    int hp_curve_loop = cl_cnt;
    int hp_plane_sur = ps_cnt;
    int hp_sur_loop = sl_cnt;
    int hp_vol_cnt = vol_cnt;
    int point_counter = hp_point;// these counter values are changed from 0 for run-on testing
    int line_counter = hp_line;//0
    int curve_loop_counter = hp_curve_loop;//0
    int plane_surfaces_counter = hp_plane_sur;//0
    int surface_loops_counter = hp_sur_loop;//0
    int volumes_counter = hp_vol_cnt;//0
    // Insert base points & lines ----------------------------
    auto c_end = std::prev(contour.end(),1);
    for ( auto c_it = contour.begin(); c_it != c_end; c_it++)
    {
        GEO::point in_point;
        GEO::line in_line;
        in_point.x = c_it->X;
        in_point.y = c_it->Y;
        in_point.z = proc_info_map[layer_number].z_start;
        in_point.char_len = 1e3;
        in_line.start = ++point_counter;
        poly_points_map.insert({point_counter, in_point});
        in_line.end = point_counter+1;
        poly_lines_map.insert({++line_counter,in_line});
    }
    GEO::point in_point;
    GEO::line in_line;
    in_point.x = c_end->X;
    in_point.y = c_end->Y;
    in_point.z = proc_info_map[layer_number].z_start;
    in_point.char_len = 1e3;
    in_line.start = ++point_counter;
    poly_points_map.insert({point_counter, in_point});
    in_line.end = hp_point+1;//start of points for this polyhedron
    poly_lines_map.insert({++line_counter,in_line});
    //--------------------------------------------------------//
    // Create base surface -----------------------------------//
    auto plm_end = poly_lines_map.end();
    std::vector<int> base_loop;
    for ( auto plm_it = poly_lines_map.begin(); plm_it != plm_end; plm_it++)
        base_loop.push_back(plm_it->first);
    poly_curve_loops_map.insert({++curve_loop_counter, base_loop});
    poly_plane_surfaces_map.insert({ ++plane_surfaces_counter,
                                     plane_surfaces_counter});
    //--------------------------------------------------------//
    // Etch profile creation ---------------------------------//
    auto ppm_end = poly_points_map.end();
    std::map<int, GEO::point> contour_points;
    std::vector<int> right_angled_points;
    for ( auto ppm_it = poly_points_map.begin();
          ppm_it != ppm_end; ppm_it++)
    {
        GEO::point prev_point, current_point, next_point;
        // First Case
        if(ppm_it == poly_points_map.begin())
        {
            prev_point      = std::prev(ppm_end,1)->second;
            current_point   = ppm_it->second;
            next_point      = std::next(ppm_it,1)->second;
        }
        // Last Case
        else if(ppm_it == std::prev(poly_points_map.end(),1))
        {
            prev_point      = std::prev(ppm_it,1)->second;
            current_point   = ppm_it->second;
            next_point      = poly_points_map.begin()->second;
        }
        // Normal Case
        else
        {
            prev_point     = std::prev(ppm_it,1)->second;
            current_point  = ppm_it->second;
            next_point     = std::next(ppm_it,1)->second;
        }
        // Prime the contour for insertion
        double theta = calculate_theta(prev_point, current_point, next_point)*180/PI;
        //create a point at poly points map it, + 5 nm
        std::vector<GEO::point> test_point_v;
        GEO::point test_point;
        test_point.x = (int)ppm_it->second.x+5;
        test_point.y = (int)ppm_it->second.y;
        test_point.z = 0;
        test_point_v.push_back(test_point);
        //rotate point by theta
        rotate_vector(test_point.x-5, test_point.y, theta, test_point_v);
        //convert to clipper compatible
        ClipperLib::IntPoint cl_test_point;
        cl_test_point.X = (int)test_point_v[0].x;
        cl_test_point.Y = (int)test_point_v[0].y;
        // do point in polygon check. depending on setting, adjust theta
        if(outward_facing==true)
        {
            if(ClipperLib::PointInPolygon(cl_test_point, contour)==1)
                theta = theta + 180;
        }
        else
        {
            if(ClipperLib::PointInPolygon(cl_test_point, contour)==0)
                theta = theta + 180;
        }
        // get all but first point of contour
        std::vector<GEO::point> contour_instance;
        contour_instance.insert( contour_instance.end(),
                                 std::next(etch_contour_map[layer_number].begin(),1),
                                 etch_contour_map[layer_number].end() );
        rotate_vector(0, 0, theta, contour_instance);
        translate_vector(contour_instance, current_point.x, current_point.y, current_point.z);
        int cont_inst_size = contour_instance.size();
        // test for 90deg elbow
        // 90 degree elbow check
        // if 90 degree elbow reverse by 45 degrees and then place 6 every 15 degrees
        if (is_elbow(prev_point, current_point, next_point)==true)
        {
            int eldist = std::distance(poly_points_map.begin(), ppm_it) + 1+hp_point;
            right_angled_points.push_back(eldist);
            rotate_vector(current_point.x, current_point.y, -45, contour_instance);
            for ( int i = 0; i < 7; i++)
            {
                for ( auto j = contour_instance.begin(); j != contour_instance.end(); j++)// TODO: REVIEW
                {
                    contour_points.insert({++point_counter, *j});
                }
                rotate_vector(current_point.x, current_point.y, 15, contour_instance);
            }
        }
        // Otherwise, place like normal.
        else
        {
            for ( auto i = contour_instance.begin(); i != contour_instance.end(); i++)
            {
                contour_points.insert({++point_counter, *i});
            }
        }
    }
    poly_points_map.insert(contour_points.begin(),contour_points.end());
    //--------------------------------------------------------//
    // Plane surface creation --------------------------------//
    int base_points_count = contour.size();
    auto layer_count = etch_contour_map[layer_number].size();
    // for each edge of base polygon
    int panel_count = 0;
    for ( int n = hp_point+1; n < (base_points_count+hp_point+1); n++)
    {
        int gain = n + contour.size()-(hp_point+1);
        int depth = etch_contour_map[layer_number].size()-1;
        bool r_angle_flag = false;
        if (std::find(right_angled_points.begin(),right_angled_points.end(),n)!=right_angled_points.end())
            r_angle_flag = true;
        if (r_angle_flag==true)
        {
            add_right_plane( n, gain, depth,
                            poly_lines_map,
                            poly_curve_loops_map,
                            poly_plane_surfaces_map,
                            base_points_count,
                            panel_count,
                            roof_line_vector,
                            hp_curve_loop,
                            hp_plane_sur,
                            hp_point);
        }
        else
        {
            add_normal_plane( n, gain, depth,
                              poly_lines_map,
                              poly_curve_loops_map,
                              poly_plane_surfaces_map,
                              base_points_count,
                              panel_count,
                              roof_line_vector,
                              hp_curve_loop,
                              hp_plane_sur,
                              hp_point);
        }

    }
    // Create roof surface -----------------------------------//
    curve_loop_counter = std::prev(poly_curve_loops_map.end(),1)->first;
    plane_surfaces_counter = std::prev(poly_plane_surfaces_map.end(),1)->first;
    poly_curve_loops_map.insert({++curve_loop_counter, roof_line_vector});
    poly_plane_surfaces_map.insert({++plane_surfaces_counter, curve_loop_counter});
    //--------------------------------------------------------//
    polyhedron.insert_points_map(poly_points_map);
    polyhedron.insert_lines_map(poly_lines_map);
    polyhedron.insert_curve_loops_map(poly_curve_loops_map);
    polyhedron.insert_plane_surfaces_map(poly_plane_surfaces_map);
    polyhedron.make_coherent(false);
    // Make surface loop out of all plane surfaces
    std::vector<int> polyhedron_surface;
    auto slm_it_end = poly_plane_surfaces_map.end();
    for ( auto slm_it = poly_plane_surfaces_map.begin(); slm_it != slm_it_end; slm_it++)
    {
    polyhedron_surface.push_back(slm_it->first);
    }
    poly_surface_loops_map.insert({++surface_loops_counter, polyhedron_surface});
    // Make volume out of surface loop
    poly_volumes_map.insert({++volumes_counter, surface_loops_counter});
    // insert volume and surface loops maps into geo file.
    polyhedron.insert_surface_loops_map(poly_surface_loops_map); //TODO: uncomment
    polyhedron.insert_volumes_map(poly_volumes_map);             //TODO: Uncomment
    polyhedron.switch_to_OpenCASCADE();
    polyhedron.disable_char_len();
    //update global counters for next shape
    pnt_cnt = std::prev(poly_points_map.end(),1)->first;
    ln_cnt = std::prev(poly_lines_map.end(),1)->first;
    cl_cnt = std::prev(poly_curve_loops_map.end(),1)->first;
    ps_cnt = std::prev(poly_plane_surfaces_map.end(),1)->first;
    sl_cnt = std::prev(poly_surface_loops_map.end(),1)->first;
    vol_cnt = std::prev(poly_volumes_map.end(),1)->first;
    // std::string polyhedron_type = (is_positive==true)? ("P") : ("N");
    // std::string polyhedron_name = "data/3DGen/"
    // + std::to_string(layer_number) + "_" + polyhedron_type + "_"
    // + std::to_string(unique_ID) + ".geo";
    // std::cout << "Exporting " << polyhedron_name << std::endl; // TODO: Remove once stable
    // polyhedron.export_geofile(polyhedron_name);
    generated_geofile = polyhedron;
    return EXIT_SUCCESS;
}

// Returns the theta angle to rotate contour by (in radians)
double MODEL3D::three_dim_model::calculate_theta(const GEO::point &prev,
                                                 const GEO::point &curr,
                                                 const GEO::point &next )
{
    // Set up the vectors
    euclid_vector a, b, c;
    a.i = curr.x - prev.x;
    a.j = curr.y - prev.y;
    a.k = 0;
    b.i = curr.x - next.x;
    b.j = curr.y - next.y;
    b.k = 0;
    // Normalize them
    double a_dist = std::sqrt( pow((double)a.i,2) + pow((double)a.j,2) );
    double b_dist = std::sqrt( pow((double)b.i,2) + pow((double)b.j,2) );
    a.i = a.i/a_dist;
    a.j = a.j/a_dist;
    b.i = b.i/b_dist;
    b.j = b.j/b_dist;
    // Calculate the outward directional vector
    c.i = a.i + b.i;
    c.j = a.j + b.j;
    c.k = 0;
    // Calculate the angle of rotation
    double theta = atan2(c.j, c.i);
    return theta;
}

/**
 *  @brief Rotates the specified vector of points
 *  @param cx x_offset
 *  @param cy y_offset
 *  @param angle angle to rotate vector by. Angle must be counter-clockwise from
 * x-axis and in degrees.
 */
void MODEL3D::three_dim_model::rotate_vector(
double cx, double cy, double angle,
std::vector<GEO::point> &subject) // POINT &subject
{
  angle = (angle * PI) / ((double)180); // convert angle to radians
  double s = sin(angle);
  double c = cos(angle);
  auto revert_point_to_origin = [&cx, &cy](GEO::point &n) {
    n.x -= cx;
    n.y -= cy;
  };
  auto rotate_point = [&c, &s](GEO::point &n) {
    int n_x_orig = n.x;
    n.x = n.x * c - n.y * s;
    n.y = n_x_orig * s + n.y * c;
  };
  auto translate_back = [&cx, &cy](GEO::point &n) {
    n.x = n.x + cx;
    n.y = n.y + cy;
  };
  for_each(subject.begin(), subject.end(),
           revert_point_to_origin); // translate points back to origins
  for_each(subject.begin(), subject.end(), rotate_point); // rotate points
  for_each(subject.begin(), subject.end(),
           translate_back); // translate points back
}

void MODEL3D::three_dim_model::translate_vector
( std::vector<GEO::point> &point_vec,
const double &delta_x,
const double &delta_y,
const double &delta_z )
{
    auto translate_pt = [&delta_x, &delta_y, &delta_z](GEO::point &n) {
    n.x = n.x + delta_x;
    n.y = n.y + delta_y;
    n.z = n.z + delta_z;};
    for_each( point_vec.begin(), point_vec.end(),
              translate_pt); // translate points back

}

bool MODEL3D::three_dim_model::is_elbow(
const GEO::point &prev,
const GEO::point &curr,
const GEO::point &next)
{
    GEO::point elbow_test_prev = prev;
    GEO::point elbow_test_current = curr;
    GEO::point elbow_test_next = next;
    // move all points to "origin" relative to curr
    elbow_test_prev.x       = elbow_test_prev.x    - elbow_test_current.x;
    elbow_test_prev.y       = elbow_test_prev.y    - elbow_test_current.y;
    elbow_test_prev.z       = elbow_test_prev.z    - elbow_test_current.z;
    elbow_test_next.x       = elbow_test_next.x    - elbow_test_current.x;
    elbow_test_next.y       = elbow_test_next.y    - elbow_test_current.y;
    elbow_test_next.z       = elbow_test_next.z    - elbow_test_current.z;
    elbow_test_current.x    = elbow_test_current.x - elbow_test_current.x;
    elbow_test_current.y    = elbow_test_current.y - elbow_test_current.y;
    elbow_test_current.z    = elbow_test_current.z - elbow_test_current.z;
    //elbow test prev and current are now vectors
    auto dot = elbow_test_prev.x*elbow_test_next.x + elbow_test_prev.y*elbow_test_next.y;
    auto det = elbow_test_prev.x*elbow_test_next.y - elbow_test_prev.y*elbow_test_next.x;
    auto angle = atan2(det, dot)*180/PI;
    angle = std::round(std::abs(angle));
    if (angle==90)
        return true;
    else
        return false;
}

void MODEL3D::three_dim_model::add_normal_plane(
int &n, int &gain, int &depth,
std::map<int, GEO::line> &lines_map,
std::map<int, std::vector<int>> &curve_loops_map,
std::map<int, int> &plane_surfaces_map,
const int &base_points_count,
int &panel_count,
std::vector<int> &roof_line_vector,
const int &prev_cl_count,
const int &prev_pls_count,
const int &prev_pnt_count)
{
    // Setting up the counter values ========================================
    int line_count = std::prev(lines_map.end(),1)->first;
    int curve_loop_count;
    if (!curve_loops_map.empty())
        curve_loop_count = std::prev(curve_loops_map.end(),1)->first;
    else
        curve_loop_count = prev_cl_count;
    int plane_surfaces_count;
    if(!plane_surfaces_map.empty())
        plane_surfaces_count = std::prev(plane_surfaces_map.end(),1)->first;
    else
        plane_surfaces_count = prev_pls_count;
    // Creating the ground-level plane surface ===============================
    //c<--b
    //v + ^                             <-- Loop to form plane surface
    //d-->a
    GEO::line a, b, c, d, diag;
    a.start = n;                        // initial point of plane surface
    if(n==base_points_count+prev_pnt_count)            // end of the line case
    {
        a.end       = base_points_count + 1 + prev_pnt_count + depth * panel_count; //7 points per 90deg bend
        b.start     = a.end;
        b.end       = 1 + prev_pnt_count + base_points_count;
        c.start     = b.end;
        c.end       = 1 + prev_pnt_count;
        d.start     = c.end;
        d.end       = a.start;
    }
    else
    {
        a.end       = base_points_count + 1 + prev_pnt_count + depth * panel_count;
        b.start     = a.end;
        b.end       = b.start + depth;
        c.start     = b.end;
        c.end       = n + 1;
        d.start     = c.end;
        d.end       = a.start;
    }
    diag.start      = b.end;
    diag.end        = a.start;
    lines_map.insert({++line_count, a});
    lines_map.insert({++line_count, b});
    lines_map.insert({++line_count, diag});
    lines_map.insert({++line_count, c});
    lines_map.insert({++line_count, d});
    curve_loops_map.insert({++curve_loop_count, {line_count-4, line_count-3, line_count-2} } );
    plane_surfaces_map.insert({++plane_surfaces_count, curve_loop_count});
    curve_loops_map.insert({++curve_loop_count, {(-1*(line_count-2)), line_count-1, line_count} } );
    plane_surfaces_map.insert({++plane_surfaces_count, curve_loop_count});
    // Creating the higher plane surfaces ===============================
    for ( int level_deep = 1; level_deep < depth; level_deep++)
    {
        a.start     = a.end;
        a.end       = a.start + 1;
        b.start     = a.end;
        b.end       = (n==base_points_count+prev_pnt_count) ? (1+base_points_count+prev_pnt_count+level_deep) : (b.start + depth);
        c.start     = b.end;
        c.end       = c.start-1;
        d.start     = c.end;
        d.end       = a.start;
        diag.start  = b.end;
        diag.end    = a.start;
        lines_map.insert({++line_count, a});
        lines_map.insert({++line_count, b});
        if(level_deep==(depth-1))
            roof_line_vector.push_back(line_count);
        lines_map.insert({++line_count, diag});
        lines_map.insert({++line_count, c});
        lines_map.insert({++line_count, d});
        curve_loops_map.insert( {++curve_loop_count, {line_count-4, line_count-3, line_count-2} } );
        plane_surfaces_map.insert({++plane_surfaces_count, curve_loop_count});
        curve_loops_map.insert( {++curve_loop_count, {(-1*(line_count-2)), line_count-1, line_count} } );
        plane_surfaces_map.insert({++plane_surfaces_count, curve_loop_count});
    }
    // Increment the panel counter
    panel_count++;
}

void MODEL3D::three_dim_model::add_right_plane(
int &n, int &gain, int &depth,
std::map<int, GEO::line> &lines_map,
std::map<int, std::vector<int>> &curve_loops_map,
std::map<int, int> &plane_surfaces_map,
const int &base_points_count,
int &panel_count,
std::vector<int> &roof_line_vector,
const int &prev_cl_count,
const int &prev_pls_count,
const int &prev_pnt_count)
{
    // Setting up the counter values ========================================
    int line_count = std::prev(lines_map.end(),1)->first;
    int curve_loop_count;
    if (!curve_loops_map.empty())
        curve_loop_count = std::prev(curve_loops_map.end(),1)->first;
    else
        curve_loop_count = prev_cl_count;
    int plane_surfaces_count;
    if(!plane_surfaces_map.empty())
        plane_surfaces_count = std::prev(plane_surfaces_map.end(),1)->first;
    else
        plane_surfaces_count = prev_pls_count;
    // Loop through six panels
    for ( int corner_cnt = 0; corner_cnt < 6; corner_cnt++)
    {
        // Creating the ground-level plane surface ===============================
        //c<--b
        //v + ^                             <-- Loop to form plane surface
        //d-->a
        GEO::line a, b, c, d, diag;
        a.start = n;                        // initial point of plane surface
        a.end   = base_points_count + 1 + prev_pnt_count + depth * panel_count;
        b.start = a.end;
        b.end   = b.start + depth;
        c.start = b.end;
        c.end   = a.start;
        lines_map.insert({++line_count, a});
        lines_map.insert({++line_count, b});
        lines_map.insert({++line_count, c});
        curve_loops_map.insert(
        {++curve_loop_count,
        {line_count-2, line_count-1,
        line_count} } );
        plane_surfaces_map.insert({++plane_surfaces_count, curve_loop_count});
        // Creating the higher plane surfaces ===============================
        for ( int level_deep = 1; level_deep < depth; level_deep++)
        {
            a.start           = a.end;
            a.end          = a.end + 1;
            b.start           = a.end;
            b.end          = b.start + depth;
            c.start          = b.end;
            c.end           = c.start - 1;
            d.start          = c.end;
            d.end           = a.start;
            diag.start       = b.end;
            diag.end        = a.start;
            lines_map.insert({++line_count, a});
            lines_map.insert({++line_count, b});
            if(level_deep==(depth-1))
                roof_line_vector.push_back(line_count);
            lines_map.insert({++line_count, diag});
            lines_map.insert({++line_count, c});
            lines_map.insert({++line_count, d});
            curve_loops_map.insert( {++curve_loop_count,
                                    {(line_count-4), (line_count-3), (line_count-2)} } );
            plane_surfaces_map.insert({++plane_surfaces_count, curve_loop_count});
            curve_loops_map.insert( {++curve_loop_count,
                                    {(-1*(line_count-2)), (line_count-1), (line_count)} } );
            plane_surfaces_map.insert({++plane_surfaces_count, curve_loop_count});
        }
        panel_count++;
    }
    //treat the follow-up panel as a normal plane
    add_normal_plane(   n, gain, depth,
                        lines_map,
                        curve_loops_map,
                        plane_surfaces_map,
                        base_points_count,
                        panel_count,
                        roof_line_vector,
                        prev_cl_count,
                        prev_pls_count,
                        prev_pnt_count);
}

void MODEL3D::three_dim_model::build_layer_box(
 const int &layer_number,
const ClipperLib::IntPoint &a,
const ClipperLib::IntPoint &b,
const ClipperLib::IntPoint &c,
const ClipperLib::IntPoint &d,
GEO::geofile &generated_geofile )
{
    int base_height = proc_info_map[layer_number].z_start;
    int thickness = proc_info_map[layer_number].thickness;
    GEO::geofile base_shape;
    base_shape.switch_to_OpenCASCADE();
    std::map<int, GEO::point> points_map;
    std::map<int, GEO::line> lines_map;
    std::map<int, std::vector<int>> curve_loops_map;
    std::map<int, int> plane_surfaces_map;
    std::map<int, std::vector<int>> surface_loops_map;
    std::map<int, int> volumes_map;
    GEO::point one, two, three, four, five, six, seven, eight;
    int point_count = pnt_cnt;
    int line_count = ln_cnt;
    int plane_sur_count = ps_cnt;
    int cur_loop_count = cl_cnt;
    int sur_loop_count = sl_cnt;
    int vol_count = vol_cnt;
    //------- bottom --------
    one.x   = a.X;
    one.y   = a.Y;
    one.z   = base_height;
    one.char_len = model_default_char_length;
    points_map.insert({++point_count, one});
    two.x   = b.X;
    two.y   = b.Y;
    two.z   = base_height;
    two.char_len = model_default_char_length;
    points_map.insert({++point_count, two});
    three.x = c.X;
    three.y = c.Y;
    three.z = base_height;
    three.char_len = model_default_char_length;
    points_map.insert({++point_count, three});
    four.x  = d.X;
    four.y  = d.Y;
    four.z  = base_height;
    four.char_len = model_default_char_length;
    points_map.insert({++point_count, four});
    GEO::line in_line;
    in_line.start = point_count-3;
    in_line.end = point_count-2;
    lines_map.insert({++line_count, in_line});
    in_line.start = point_count-2;
    in_line.end = point_count-1;
    lines_map.insert({++line_count, in_line});
    in_line.start = point_count-1;
    in_line.end = point_count;
    lines_map.insert({++line_count, in_line});
    in_line.start = point_count;
    in_line.end = point_count-3;
    lines_map.insert({++line_count, in_line});
    curve_loops_map.insert({++cur_loop_count, {(point_count-3), (point_count-2), (point_count-1), point_count}});
    plane_surfaces_map.insert({++plane_sur_count, plane_sur_count});
    //-------- top ---------
    five.x   = a.X;
    five.y   = a.Y;
    five.z   = base_height + thickness;
    five.char_len = model_default_char_length;
    points_map.insert({++point_count, five});
    six.x    = b.X;
    six.y    = b.Y;
    six.z    = base_height + thickness;
    six.char_len = model_default_char_length;
    points_map.insert({++point_count, six});
    seven.x  = c.X;
    seven.y  = c.Y;
    seven.z  = base_height + thickness;
    seven.char_len = model_default_char_length;
    points_map.insert({++point_count, seven});
    eight.x  = d.X;
    eight.y  = d.Y;
    eight.z  = base_height + thickness;
    eight.char_len = model_default_char_length;
    points_map.insert({++point_count, eight});
    in_line.start = point_count-3;
    in_line.end = point_count-2;
    lines_map.insert({++line_count, in_line});
    in_line.start = point_count-2;
    in_line.end = point_count-1;
    lines_map.insert({++line_count, in_line});
    in_line.start = point_count-1;
    in_line.end = point_count;
    lines_map.insert({++line_count, in_line});
    in_line.start = point_count;
    in_line.end = point_count-3;
    lines_map.insert({++line_count, in_line});
    curve_loops_map.insert({++cur_loop_count, {point_count-3,point_count-2,point_count-1,point_count}});
    plane_surfaces_map.insert({++plane_sur_count, plane_sur_count});
    // side 1;
    in_line.start = point_count-7;
    in_line.end = point_count-3;
    lines_map.insert({++line_count, in_line});
    in_line.start = point_count-6;
    in_line.end = point_count-2;
    lines_map.insert({++line_count, in_line});
    curve_loops_map.insert({++cur_loop_count, {line_count-9,line_count,-(line_count-5),-(line_count-1)}});
    plane_surfaces_map.insert({++plane_sur_count, plane_sur_count});
    // side 2;
    in_line.start = point_count-5;
    in_line.end = point_count-1;
    lines_map.insert({++line_count, in_line});
    curve_loops_map.insert({++cur_loop_count, {line_count-1,line_count-5,-line_count,-(line_count-9)}});
    plane_surfaces_map.insert({++plane_sur_count, plane_sur_count});
    // side 3;
    in_line.start = point_count-4;
    in_line.end = point_count;
    lines_map.insert({++line_count, in_line});
    curve_loops_map.insert({++cur_loop_count, {line_count-1,line_count-5,-line_count,-(line_count-9)}});
    plane_surfaces_map.insert({++plane_sur_count, plane_sur_count});
    // side 4;
    curve_loops_map.insert({++cur_loop_count, {line_count-8,line_count-3,-(line_count-4),-line_count}});
    plane_surfaces_map.insert({++plane_sur_count, plane_sur_count});
    // enclose
    surface_loops_map.insert({++sur_loop_count, {plane_sur_count-5,plane_sur_count-4,
                                                 plane_sur_count-3,plane_sur_count-2,
                                                 plane_sur_count-1, plane_sur_count}});
    volumes_map.insert({++vol_count, sur_loop_count});
    base_shape.insert_points_map(points_map);
    base_shape.insert_lines_map(lines_map);
    base_shape.insert_curve_loops_map(curve_loops_map);
    base_shape.insert_plane_surfaces_map(plane_surfaces_map);
    base_shape.insert_surface_loops_map(surface_loops_map);
    base_shape.insert_volumes_map(volumes_map);
    base_shape.disable_char_len();
    generated_geofile = base_shape;
    pnt_cnt = point_count;
    ln_cnt = line_count;
    cl_cnt = cur_loop_count;
    ps_cnt = plane_sur_count;
    sl_cnt = sur_loop_count;
    vol_cnt = vol_count;
    // std::string output_path = "data/3DGen/" +
    // std::to_string(layer_number) + "_P_0.geo";
    // base_shape.export_geofile(output_path); //For debugging
    // __asm("nop");
}

void MODEL3D::three_dim_model::initialize_python_script()
{
    py_script.clear();
    py_script.push_back("\"\"\"Python Circuit TCAD Script Generated with Katana\"\"\"");
    py_script.push_back("# Initialization ================================================");
    py_script.push_back("import FreeCAD, Part, time");
    if(enable_multiprocessing==true)
        py_script.push_back("import concurrent.futures");
    py_script.push_back("from FreeCAD import Base");
    py_script.push_back("My_doc = FreeCAD.newDocument(\""+cell_name+"\")");
    py_script.push_back("");
    py_script.push_back("# Set up functions ==============================================");
    py_script.push_back("class Ingredient:");
    py_script.push_back("    def __init__(self, gp, ct, ps, sb, of):");
    py_script.push_back("        self.gp = gp                        # Ground Plane");
    py_script.push_back("        self.ct = ct                        # Contour");
    py_script.push_back("        self.ps = ps                        # Position");
    py_script.push_back("        self.sb = sb                        # Start Base (True/False");
    py_script.push_back("        self.of = of 				         # Overlap Factor");
    py_script.push_back("");
    py_script.push_back("def create_pss(f_ingr):");
    py_script.push_back("    #convert base and contour to freecad vector");
    py_script.push_back("    ts = time.perf_counter()");
    py_script.push_back("    fc_gp = [FreeCAD.Vector(i) for i in f_ingr.gp]");
    py_script.push_back("    fc_ct = [FreeCAD.Vector(i) for i in f_ingr.ct]");
    py_script.push_back("    fc_ps = FreeCAD.Vector(f_ingr.ps)");
    py_script.push_back("    #create the shape");
    py_script.push_back("    wire = Part.makePolygon(fc_gp)");
    py_script.push_back("    gwire = Part.Wire(wire)");
    py_script.push_back("    last_p = len(fc_ct)-1");
    py_script.push_back("    last_p_z = fc_ct[last_p].z");
    py_script.push_back("    spline = Part.BSplineCurve(fc_ct)");
    py_script.push_back("    if (f_ingr.sb==True): # not inverted");
    py_script.push_back("        if ((f_ingr.of>1)and(f_ingr.of<=2)):#has overlap scale");
    py_script.push_back("            offset_lower_pt = FreeCAD.Vector(-fc_ct[last_p].x*f_ingr.of, fc_ct[0].y, fc_ct[0].z)");
    py_script.push_back("            offset_upper_pt = FreeCAD.Vector(offset_lower_pt.x, fc_ct[last_p].y, fc_ct[last_p].z)");
    py_script.push_back("            offset_ln = Part.LineSegment(spline.StartPoint, offset_lower_pt)");
    py_script.push_back("            vert_ln = Part.LineSegment(offset_lower_pt, offset_upper_pt)");
    py_script.push_back("            horiz_ln = Part.LineSegment(offset_upper_pt, spline.EndPoint)");
    py_script.push_back("            profile = Part.Shape([offset_ln, vert_ln, horiz_ln, spline])");
    py_script.push_back("        elif ( f_ingr.of == 1 ):#no overlap scale");
    py_script.push_back("            upper_point = FreeCAD.Vector(spline.StartPoint.x, spline.StartPoint.y, spline.EndPoint.z)");
    py_script.push_back("            vertical_ln = Part.LineSegment(spline.StartPoint, upper_point)");
    py_script.push_back("            horiz_ln = Part.LineSegment(upper_point, spline.EndPoint)");
    py_script.push_back("            profile = Part.Shape([vertical_ln, horiz_ln, spline])");
    py_script.push_back("        else:");
    py_script.push_back("            raise Exception('Overlap factor must be within the range [1, 2]')");
    py_script.push_back("        profile_wire = Part.Wire(profile.Edges)");
    py_script.push_back("        pl = FreeCAD.Placement(FreeCAD.Vector(0,0,0), FreeCAD.Rotation(FreeCAD.Vector(0,0,1), 180))");
    py_script.push_back("        profile_wire.Placement = pl");
    py_script.push_back("    else: # inverted");
    py_script.push_back("        if ((f_ingr.of>1)and(f_ingr.of<=2)): #has overlap scale");
    py_script.push_back("            spline_end = fc_ct[last_p]");
    py_script.push_back("            offset_point = FreeCAD.Vector( (fc_ct[last_p].x*f_ingr.of), fc_ct[last_p].y, fc_ct[last_p].z )");
    py_script.push_back("            offset_upper = Part.LineSegment(spline_end, offset_point)");
    py_script.push_back("            base_point = FreeCAD.Vector(offset_point.x,0,0)");
    py_script.push_back("            vert_ln = Part.LineSegment(offset_point, base_point)");
    py_script.push_back("            horiz_ln = Part.LineSegment(base_point, spline.StartPoint)");
    py_script.push_back("            profile = Part.Shape([spline, offset_upper, vert_ln, horiz_ln])");
    py_script.push_back("        elif (f_ingr.of==1):#no overlap scale");
    py_script.push_back("            lower_point = FreeCAD.Vector(spline.EndPoint.x, spline.EndPoint.y, 0)");
    py_script.push_back("            vert_ln = Part.LineSegment(spline.EndPoint, lower_point)");
    py_script.push_back("            horiz_ln = Part.LineSegment(lower_point, spline.StartPoint)");
    py_script.push_back("            profile = Part.Shape([spline, vert_ln, horiz_ln])");
    py_script.push_back("        else:");
    py_script.push_back("            raise Exception('Overlap factor must be within the range [1, 2]')");
    py_script.push_back("        profile_wire = Part.Wire(profile.Edges)");
    py_script.push_back("        pl = FreeCAD.Placement(FreeCAD.Vector(-fc_ct[last_p].x,0,0), FreeCAD.Rotation(FreeCAD.Vector(0,0,1), 0))");
    py_script.push_back("        profile_wire.Placement = pl");
    py_script.push_back("    makeSolid=True");
    py_script.push_back("    isFrenet=True");
    py_script.push_back("    TransitionMode=2#rounded edges");
    py_script.push_back("    os = gwire.makePipeShell([profile_wire],makeSolid,isFrenet, TransitionMode)");
    py_script.push_back("    os=os.removeSplitter()");
    py_script.push_back("    if os.isNull()==True:");
    py_script.push_back("        raise Exception('Prismatic Spline Sweep creation failed: sweep is null.')");
    py_script.push_back("    face = Part.Face(wire)");
    py_script.push_back("    prism = face.extrude(FreeCAD.Vector(0,0,last_p_z))");
    py_script.push_back("    final_pss = os.fuse(prism)");
    py_script.push_back("    if final_pss.isNull()==True:");
    py_script.push_back("        raise Exception('Prismatic Spline Sweep creation failed at merge.')");
    py_script.push_back("    final_pss = final_pss.removeSplitter()");
    py_script.push_back("    pl = FreeCAD.Placement(fc_ps, FreeCAD.Rotation(FreeCAD.Vector(0,0,1), 0))");
    py_script.push_back("    final_pss.Placement = pl");
    py_script.push_back("    if final_pss.isNull()==True:");
    py_script.push_back("        raise Exception('Prismatic Spline Sweep creation failed: return shape is null.')");
    py_script.push_back("    else:");
    py_script.push_back("        te = time.perf_counter() - ts");
    py_script.push_back("        fmt = format(te, '.2f')");
    py_script.push_back("        print('PSS created in '+fmt+' seconds.')");
    py_script.push_back("    return final_pss");
    py_script.push_back("");
    // Prism for Al layers
    py_script.push_back("def create_prism(f_ingr):");
    py_script.push_back("    ts = time.perf_counter()");
    py_script.push_back("    fc_gp = [FreeCAD.Vector(i) for i in f_ingr.gp]");
    py_script.push_back("    fc_ct = [FreeCAD.Vector(i) for i in f_ingr.ct]");
    py_script.push_back("    fc_ps = FreeCAD.Vector(f_ingr.ps)");
    py_script.push_back("    wire = Part.makePolygon(fc_gp)");
    py_script.push_back("    gwire = Part.Wire(wire)");
    py_script.push_back("    face = Part.Face(gwire)");
    py_script.push_back("    last_p = len(fc_ct)-1");
    py_script.push_back("    last_p_z = fc_ct[last_p].z");
    py_script.push_back("    prism = face.extrude(FreeCAD.Vector(0,0,last_p_z))");
    py_script.push_back("    pl = FreeCAD.Placement(fc_ps, FreeCAD.Rotation(FreeCAD.Vector(0,0,1), 0))");
    py_script.push_back("    prism.Placement = pl");
    py_script.push_back("    if prism.isNull()==True:");
    py_script.push_back("        raise Exception('Prism creation failed: return shape is null.')");
    py_script.push_back("    else:");
    py_script.push_back("        te = time.perf_counter() - ts");
    py_script.push_back("        fmt = format(te, '.2f')");
    py_script.push_back("        print('Prism created in '+fmt+' seconds.')");
    py_script.push_back("    return prism");
    py_script.push_back("# Generated cell description ====================================");
    py_script.push_back("# Line for MS Windows compatibility");
    py_script.push_back("if __name__ == '__main__':");
    py_script.push_back("    time_start = time.perf_counter()");
}

std::string to_freecad_vec(const double &x, const double &y, const double &z)
{
    std::string output = "FreeCAD.Vector("
    + std::to_string(x) + ","
    + std::to_string(y) + ","
    + std::to_string(z) + ")";
    return output;
}

//generate a fill layer with freecad and add it to the python script
void MODEL3D::three_dim_model::generate_freecad_fill(
const ClipperLib::Path &outline,
const ClipperLib::Paths &primary,
const ClipperLib::Paths &secondary,
const int &layer_number)
{
    // Create the list of subtractive shapes
    py_script.push_back("    subtractive_ingredients_"+std::to_string(layer_number)+" = []");
    auto primary_end = primary.end();
    for (auto primary_it = primary.begin();
         primary_it != primary_end;
         primary_it++)
    {
        push_freecad_pss(*primary_it, true, layer_number, false, subtractive_overlap_factor);
    }
    // Build them in python as a list
    build_subtractive_shapes(layer_number);
    // Execute the boolean cut.
    py_script.push_back("    subtractive_cut_"
    +std::to_string(layer_number)
    +"= additive_block_"
    +std::to_string(layer_number)
    +".cut(subtractives_"
    +std::to_string(layer_number)+")");
    // if additive shapes exist
    // Create the list of additive shapes
    if(secondary.size()>0)
    {
        py_script.push_back("    additive_ingredients_"+std::to_string(layer_number)+" = []");
        auto secondary_end = secondary.end();
        for (auto secondary_it = secondary.begin();
            secondary_it != secondary_end;
            secondary_it++)
        {
            push_freecad_pss(*secondary_it, false, layer_number, true, island_overlap_factor);
        }
        // Generate the additive shapes
        py_script.push_back("    additives_"
        +std::to_string(layer_number)+
        " = list(pool.map(create_pss, additive_ingredients_"
        +std::to_string(layer_number)+"))");
        // Execute the boolean fuse
        py_script.push_back("    layer_"
        +std::to_string(layer_number)
        +"=subtractive_cut_"
        +std::to_string(layer_number)
        +".fuse(additives_"
        +std::to_string(layer_number)+")");
    }
    else
    {
        py_script.push_back("    layer_"
        +std::to_string(layer_number)
        +"=subtractive_cut_"
        +std::to_string(layer_number) );
    }
    // py_script.push_back("    Part.show(layer_"
    // +std::to_string(layer_number)+")");
    py_script.push_back("    lyr_"
    +std::to_string(layer_number)+" = My_doc.addObject(\"Part::Feature\",\"layer_"
    +std::to_string(layer_number)+"\")");
    py_script.push_back("    lyr_"+std::to_string(layer_number)
    +".Shape = layer_"+std::to_string(layer_number));
}

//Creates the layer based on the supplied primary and secondary paths and mask type
void MODEL3D::three_dim_model::generate_freecad_solid(
const ClipperLib::Paths &primary,
const ClipperLib::Paths &secondary,
const bool &has_holes,
const int &layer_number,
const int &mask_type)
{
  // Create the list of additive shapes
    py_script.push_back("    additive_ingredients_"+std::to_string(layer_number)+" = []");
    auto primary_end = primary.end();
    for (auto primary_it = primary.begin();
         primary_it != primary_end;
         primary_it++)
    {
        bool bot_strt = false;//type 0 and 3
        double o_f = additive_overlap_factor;
        if(mask_type==1)// 0 = Standard /^##^\ , 1 = Inverted \_##_/
            bot_strt = true;
        // // Don't overlap on the counter-electrode layer or layer 55 because
        // // layers with circular shapes bug out in the fuse operation of
        // // create_pss. TODO: Still looking for a solution to this.
        // // Layer 51 and 55 have circular shapes
        if((mask_type==3)/*||(layer_number==55)*/)
            o_f = 1;
        push_freecad_pss(*primary_it, bot_strt, layer_number, true, o_f);
    }
    // Build them in python as a list
    build_additive_shapes(layer_number, true);
    // Execute the boolean fuse
    py_script.push_back("    additive_fuse_"    //taking first element of the list
    +std::to_string(layer_number)
    +"= additives_"+std::to_string(layer_number)
    +"[0]");
    py_script.push_back("    additive_fuse_"
    +std::to_string(layer_number)
    +" = additive_fuse_"
    +std::to_string(layer_number)
    +".fuse(additives_"+std::to_string(layer_number)+")");
    // Create a list of subtractive shapes if they exist
    if(secondary.size()>0)
    {
        py_script.push_back("    subtractive_ingredients_"+std::to_string(layer_number)+" = []");
        auto secondary_end = secondary.end();
        for (auto secondary_it = secondary.begin();
            secondary_it != secondary_end;
            secondary_it++)
        {
            bool bot_strt = true;// swapped around from above because placing back
            if(mask_type==1)// 0 = Standard /^##^\ , 1 = Inverted \_##_/
                bot_strt = false;
            push_freecad_pss(*secondary_it, bot_strt, layer_number, false, subtractive_overlap_factor);
        }
        // Build subtractive shapes in python as a list
        build_subtractive_shapes(layer_number);
        // Execute the boolean cut and name it layer
        py_script.push_back("    subtractive_cut_"
        +std::to_string(layer_number)
        +"= additive_fuse_"
        +std::to_string(layer_number)
        +".cut(subtractives_"
        +std::to_string(layer_number)+")");
        py_script.push_back("layer_"
        +std::to_string(layer_number)
        +" = subtractive_cut_"
        + std::to_string(layer_number));
    }
    else// Otherwise name the layer boolean fuse
    {
        py_script.push_back("    layer_"
        +std::to_string(layer_number)
        +"=additive_fuse_"
        +std::to_string(layer_number));
    }
    // py_script.push_back("    Part.show(layer_"
    // +std::to_string(layer_number)+")");
    py_script.push_back("    lyr_"
    +std::to_string(layer_number)+" = My_doc.addObject(\"Part::Feature\",\"layer_"
    +std::to_string(layer_number)+"\")");
    py_script.push_back("    lyr_"+std::to_string(layer_number)
    +".Shape = layer_"+std::to_string(layer_number));
    // Extra work for junction layer case
    if(mask_type==3)
    {
        // Create list of all Al shapes.
        std::string sub_layer = std::to_string(layer_number)+"_Al";
        py_script.push_back("    additive_ingredients_"+sub_layer+" = []");
        auto primary_end = primary.end();
        for (auto primary_it = primary.begin();
            primary_it != primary_end;
            primary_it++)
        {
            //offset the path in question by the contour-width to make flush with counter-electrode
            ClipperLib::Paths adjusted_paths;
            ClipperLib::ClipperOffset co;
            co.Clear();
            co.AddPath(*primary_it, ClipperLib::jtSquare, ClipperLib::etClosedPolygon);
            double offset_value = std::prev(etch_contour_map[layer_number].end(),1)->x;
            co.Execute(adjusted_paths, offset_value);
            push_freecad_prism( adjusted_paths[0],
                                true,
                                (proc_info_map[layer_number].z_start-9), 8, sub_layer);
        }
        // Generate all Al shapes
        build_additive_sub_layer(sub_layer);
        // Merge all AL shapes into sub-layer
        py_script.push_back("    layer_"+std::to_string(layer_number)+"_Al = "+
        "additives_"+sub_layer+"[0].fuse(additives_"+sub_layer+")");
        // py_script.push_back("    Part.show(Layer_"+std::to_string(layer_number)+"_Al)");
        py_script.push_back("    lyr_"
        +std::to_string(layer_number)+"_Al = My_doc.addObject(\"Part::Feature\",\"layer_"
        +std::to_string(layer_number)+"_Al\")");
        py_script.push_back("    lyr_"+std::to_string(layer_number)
        +"_Al.Shape = layer_"+std::to_string(layer_number)+"_Al");
        // Create list of all AlOX shapes
        sub_layer = std::to_string(layer_number)+"_AlOx";
        py_script.push_back("    additive_ingredients_"+sub_layer+" = []");
        for (auto primary_it = primary.begin();
            primary_it != primary_end;
            primary_it++)
        {
            ClipperLib::Paths adjusted_paths;
            ClipperLib::ClipperOffset co;
            co.Clear();
            co.AddPath(*primary_it, ClipperLib::jtSquare, ClipperLib::etClosedPolygon);
            double offset_value = std::prev(etch_contour_map[layer_number].end(),1)->x;
            co.Execute(adjusted_paths, offset_value);
            push_freecad_prism( adjusted_paths[0],
                                true,
                                (proc_info_map[layer_number].z_start-1), 1, sub_layer);
        }
        // Generate all AlOx shapes
        build_additive_sub_layer(sub_layer);
        // Merge AlOx shapes into sub-layer
        py_script.push_back("    layer_"+sub_layer+" = additives_"
        +sub_layer+"[0].fuse(additives_"+sub_layer+")");
        //py_script.push_back("    Part.show(Layer_" + sub_layer + ")");
        py_script.push_back("    lyr_"
        +sub_layer+" = My_doc.addObject(\"Part::Feature\",\"layer_"
        +sub_layer+"\")");
        py_script.push_back("    lyr_"+sub_layer
        +".Shape = layer_"+sub_layer);
    }
}

void MODEL3D::three_dim_model::push_freecad_prism(
const ClipperLib::Path &shape_outline,
const bool &is_additive,
const int &z_start,
const int &height,
const std::string &sublayer_name)
{
    // Populate the ingredients list
    int gp_last = shape_outline.size()+1;//last will be appended to close loop
    if(gp_last>2)
    {
        //first arrange the path for FreeCAD
        ClipperLib::Path present_path = shape_outline;
        std::vector<int> offset;
        arrange_path(present_path, offset);
        present_path.push_back(*present_path.begin());//repeat last element to close shape
        int gp_counter = 0;//minimum polygon size is a triangle
        std::ostringstream line_stream;
        py_script.push_back("    curr_ingr = Ingredient( [ ["   //first
        +std::to_string(present_path[gp_counter].X)+","
        +std::to_string(present_path[gp_counter].Y)+","
        +std::to_string(0)+"]");
        gp_counter++;
        while( gp_counter != gp_last)
        {
            //push back 1
            line_stream << "    ,["+std::to_string(present_path[gp_counter].X) + ","
            +std::to_string(present_path[gp_counter].Y)+","
            +std::to_string(0)+"]";
            gp_counter++;
            if (gp_counter == gp_last)
            {
                py_script.push_back(line_stream.str());
                break;
            }
            //push back 2
            line_stream << ",["+std::to_string(present_path[gp_counter].X) + ","
            +std::to_string(present_path[gp_counter].Y)+","
            +std::to_string(0)+"]";
            gp_counter++;
            if (gp_counter == gp_last)
            {
                py_script.push_back(line_stream.str());
                break;
            }
            //push back 3
            line_stream << ",["+std::to_string(present_path[gp_counter].X) + ","
            +std::to_string(present_path[gp_counter].Y)+","
            +std::to_string(0)+"]";
            gp_counter++;
            py_script.push_back(line_stream.str());
            line_stream.str("");
            line_stream.clear();
        }
        //do final close off of the ground path. Move onto the height
        line_stream.str("");
        line_stream.clear();
        line_stream << "    ], [" ;
        line_stream << "[" << std::to_string(0) << ", ";
        line_stream << std::to_string(0) << ", ";
        line_stream << std::to_string(0) << "],";
        line_stream << "[" << std::to_string(0) << ", ";
        line_stream << std::to_string(0) << ", ";
        line_stream << std::to_string(height) << "]";
        std::string debug_str = line_stream.str();
        py_script.push_back(line_stream.str());
        line_stream.str("");
        line_stream.clear();
        //position
        line_stream << "    ], [" ;// end of contour, start of offset
        line_stream << std::to_string(offset[0]) << ", ";
        line_stream << std::to_string(offset[1]) << ", ";
        line_stream << std::to_string(z_start) << "], ";// end offset
        py_script.push_back(line_stream.str());
        std::string botstart = "    True, 1)";//not used for prism function
        py_script.push_back(botstart);
        if(is_additive==false)
            py_script.push_back("    subtractive_ingredients_"+sublayer_name+".append(curr_ingr)");
        else
            py_script.push_back("    additive_ingredients_"+sublayer_name+".append(curr_ingr)");
    }
    else
        std::cout<<"Error: \"Polygon\" with less than 2 vertices detected."<< std::endl;
}

// Pushes back PSS to additive or subtractive list for later creation
void MODEL3D::three_dim_model::push_freecad_pss(
const ClipperLib::Path &shape_outline,
const bool             &bottom_start,
const int              &layer_number,
const bool             &is_additive,
const double           &overlap_factor)
{
    // Populate the ingredients list
    int gp_last = shape_outline.size()+1;//last will be appended to close loop
    if(gp_last>2)
    {
        //first arrange the path for FreeCAD
        ClipperLib::Path present_path = shape_outline;
        std::vector<int> offset;
        arrange_path(present_path, offset);
        // if(bottom_start==false)
        //     path_to_gds(present_path);
        present_path.push_back(*present_path.begin());//repeat last element to close shape
        double zs = (double)proc_info_map[layer_number].z_start;
        // start with the ground path
        int gp_counter = 0;//minimum polygon size is a triangle
        std::ostringstream line_stream;
        py_script.push_back("    curr_ingr = Ingredient( [ ["   //first
        +std::to_string(present_path[gp_counter].X)+","
        +std::to_string(present_path[gp_counter].Y)+","
        +std::to_string(0)+"]");
        gp_counter++;
        while( gp_counter != gp_last)
        {
            //push back 1
            line_stream << "    ,["+std::to_string(present_path[gp_counter].X) + ","
            +std::to_string(present_path[gp_counter].Y)+","
            +std::to_string(0)+"]";
            gp_counter++;
            if (gp_counter == gp_last)
            {
                py_script.push_back(line_stream.str());
                break;
            }
            //push back 2
            line_stream << ",["+std::to_string(present_path[gp_counter].X) + ","
            +std::to_string(present_path[gp_counter].Y)+","
            +std::to_string(0)+"]";
            gp_counter++;
            if (gp_counter == gp_last)
            {
                py_script.push_back(line_stream.str());
                break;
            }
            //push back 3
            line_stream << ",["+std::to_string(present_path[gp_counter].X) + ","
            +std::to_string(present_path[gp_counter].Y)+","
            +std::to_string(0)+"]";
            gp_counter++;
            py_script.push_back(line_stream.str());
            line_stream.str("");
            line_stream.clear();
        }
        //do final close off of the ground path. Move onto the contour
        line_stream.str("");
        line_stream.clear();
        auto etch_it = etch_contour_map[layer_number].begin();
        line_stream << "    ], [" ;
        line_stream << "[" << std::to_string(etch_it->x) << ", ";
        line_stream << std::to_string(etch_it->y) << ", ";
        line_stream << std::to_string(etch_it->z) << "]";
        etch_it++;
        auto etch_it_end = etch_contour_map[layer_number].end();
        while (etch_it != etch_it_end)
        {
            line_stream << "    , [" << std::to_string(etch_it->x) << ", ";
            line_stream << std::to_string(etch_it->y) << ", ";
            line_stream << std::to_string(etch_it->z) << "]";
            etch_it++;
            if (etch_it == etch_it_end)
            {
                py_script.push_back(line_stream.str());
                break;
            }
            line_stream << ", [" << std::to_string(etch_it->x) << ", ";
            line_stream << std::to_string(etch_it->y) << ", ";
            line_stream << std::to_string(etch_it->z) << "]";
            etch_it++;
            if (etch_it == etch_it_end)
            {
                py_script.push_back(line_stream.str());
                break;
            }
            line_stream << ", [" << std::to_string(etch_it->x) << ", ";
            line_stream << std::to_string(etch_it->y) << ", ";
            line_stream << std::to_string(etch_it->z) << "]";
            py_script.push_back(line_stream.str());
            etch_it++;
            line_stream.str("");
            line_stream.clear();
        }
        line_stream.str("");
        line_stream.clear();
        line_stream << "    ], [" ;// end of contour, start of offset
        line_stream << std::to_string(offset[0]) << ", ";
        line_stream << std::to_string(offset[1]) << ", ";
        line_stream << std::to_string(zs) << "], ";// end offset
        py_script.push_back(line_stream.str());
        std::string botstart = (bottom_start==true)?("    True, "):("    False, ");
        py_script.push_back(botstart);
        py_script.push_back("    "+std::to_string(overlap_factor)+")");
        if(is_additive==false)
            py_script.push_back("    subtractive_ingredients_"+std::to_string(layer_number)+".append(curr_ingr)");
        else
            py_script.push_back("    additive_ingredients_"+std::to_string(layer_number)+".append(curr_ingr)");
    }
    else
        std::cout<<"Error: \"Polygon\" with less than 2 vertices detected."<< std::endl;
}

// Puts the path in correct orientation for FreeCAD.
// The path is made to run clockwise around the positive Z-axis.
// The shape is also returned to the origin, and therefore must be shifted
// by the offset at a later stage.
void MODEL3D::three_dim_model::arrange_path(ClipperLib::Path &to_arrange, std::vector<int> &offset)
{
    if ( ClipperLib::Orientation(to_arrange)==true )
        ClipperLib::ReversePath(to_arrange);

    // find left-most, bottom-most point
    int x_lowest = to_arrange[0].X;
    auto i_end = to_arrange.end();
    for ( auto i = to_arrange.begin(); i!= i_end; i++ )
    {
        if ( i->X < x_lowest)
            x_lowest = i->X;
    }
    ClipperLib::Path xlow_vec;
    for ( auto i = to_arrange.begin(); i!=i_end; i++ )
    {
        if ( i->X == x_lowest )
        {
            ClipperLib::IntPoint pt;
            pt.X = i->X;
            pt.Y = i->Y;
            xlow_vec.push_back(pt);
        }
    }
    int y_lowest = xlow_vec[0].Y;
    auto j_end = xlow_vec.end();
    for ( auto j = xlow_vec.begin(); j!= j_end; j++ )
    {
        if ( j->Y < y_lowest)
            y_lowest = j->Y;
    }
    ClipperLib::IntPoint target;
    target.X = x_lowest;
    target.Y = y_lowest;
    auto loc = std::find(to_arrange.begin(), to_arrange.end(), target);
    ClipperLib::Path arranged;
    arranged.insert( arranged.end(), loc, to_arrange.end());
    arranged.insert( arranged.end(), to_arrange.begin(), loc);
    offset.push_back(target.X);
    offset.push_back(target.Y);
    auto arranged_end = arranged.end();
    for(auto arranged_it = arranged.begin(); arranged_it != arranged_end; arranged_it++)
    {
        arranged_it->X = arranged_it->X - x_lowest;
        arranged_it->Y = arranged_it->Y - y_lowest;
    }
    to_arrange = arranged;
}

void MODEL3D::three_dim_model::python_script_write(std::string output_path)
{
    std::ofstream python_file;
    python_file.open(output_path);
    auto pyscript_end = py_script.end();
    for (auto pyscript_it = py_script.begin();
         pyscript_it != py_script.end();
         pyscript_it++)
        python_file << *pyscript_it << "\n";
    python_file.close();
    std::cout << "File successfully saved as \""+output_path+"\" " << std::endl;
}

// Places the command that tells python to  create a list of results
void MODEL3D::three_dim_model::build_subtractive_shapes(const int &layer_number)
{
    py_script.push_back("    # Create all pushed shapes.");
    py_script.push_back("    t1 = time.perf_counter()");
    if(enable_multiprocessing==true)
    {
        py_script.push_back("    pool = concurrent.futures.ProcessPoolExecutor()");
        py_script.push_back("    subtractives_"+std::to_string(layer_number)+
        " = list(pool.map(create_pss, subtractive_ingredients_"
        +std::to_string(layer_number)+"))");
    }
    else
    {
        py_script.push_back("    subtractives_"+std::to_string(layer_number)+
        " = list(map(create_pss, subtractive_ingredients_"
        +std::to_string(layer_number)+"))");
    }
    py_script.push_back("    t2 = time.perf_counter() - t1");
    py_script.push_back("    fmt = format(t2, '.2f')");
    py_script.push_back("    print('Generating subtractive shapes took '+fmt+' seconds.')");
}

// Places the command that tells python to  create a map of additive results
// Shapes created are Prismatic Spline Sweeps
void MODEL3D::three_dim_model::build_additive_shapes(const int &layer_number, const bool &is_pss)
{
    py_script.push_back("    # Create all pushed shapes.");
    py_script.push_back("    t1 = time.perf_counter()");
    std::string shape_type = (is_pss==true)?("create_pss"):("create_prism");
    if(enable_multiprocessing==true)
    {
        py_script.push_back("    pool = concurrent.futures.ProcessPoolExecutor()");
        py_script.push_back("    additives_"+std::to_string(layer_number)+
        " = list(pool.map("+shape_type+", additive_ingredients_"
        +std::to_string(layer_number)+"))");
    }
    else
    {
        py_script.push_back("    additives_"+std::to_string(layer_number)+
        " = list(map("+shape_type+", additive_ingredients_"
        +std::to_string(layer_number)+"))");
    }
    py_script.push_back("    t2 = time.perf_counter() - t1");
    py_script.push_back("    fmt = format(t2, '.2f')");
    py_script.push_back("    print('Generating additive shapes took '+fmt+' seconds.')");
}

// Places the command that tells python to  create a map of additive sub_layer
// Shapes created are prisms
void MODEL3D::three_dim_model::build_additive_sub_layer(const std::string &sub_layer_name)
{
    py_script.push_back("    # Create all pushed shapes.");
    py_script.push_back("    t1 = time.perf_counter()");
    if(enable_multiprocessing==true)
    {
        py_script.push_back("    pool = concurrent.futures.ProcessPoolExecutor()");
        py_script.push_back("    additives_"+sub_layer_name+
        " = list(pool.map(create_prism, additive_ingredients_"
        +sub_layer_name+"))");
    }
    else
    {
        py_script.push_back("    additives_"+sub_layer_name+
        " = list(map(create_prism, additive_ingredients_"
        +sub_layer_name+"))");
    }
    py_script.push_back("    t2 = time.perf_counter() - t1");
    py_script.push_back("    fmt = format(t2, '.2f')");
    py_script.push_back("    print('Generating additive shapes took '+fmt+' seconds.')");
}

void MODEL3D::three_dim_model::python_script_conclude()
{
    py_script.push_back("    My_doc.recompute()");
    py_script.push_back("    time_end = time.perf_counter() - time_start");
    py_script.push_back("    fmt = format(time_end, '.2f')");
    py_script.push_back("    print(f'Layer generation took '+fmt+' seconds.')");
    py_script.push_back("    My_doc.saveAs(u\""+cell_name+".FCStd\")");
}

void MODEL3D::three_dim_model::tidy_paths(ClipperLib::Paths &input_paths)
{
    auto paths_end = input_paths.end();
    for(auto paths_it = input_paths.begin(); paths_it != paths_end; paths_it++)
    {
        tidy_path(*paths_it);
    }
}

//Removes points from path if they lie within 1% of the perimeter's distance between each other
void MODEL3D::three_dim_model::tidy_path(ClipperLib::Path &input_path)
{
    //determine perimeter of path
    double path_perimeter = 0;
    auto perim_start = std::next(input_path.begin(),1);
    auto path_end = input_path.end();
    for (auto path_it = perim_start; path_it != path_end; path_it++)
    {
        ClipperLib::IntPoint prev_pt = *std::prev(path_it);
        ClipperLib::IntPoint curr_pt = *path_it;
        double delta_x_sqr = std::pow((curr_pt.X - prev_pt.X), 2);
        double delta_y_sqr = std::pow((curr_pt.Y - prev_pt.Y), 2);
        double dist = std::sqrt(delta_x_sqr+delta_y_sqr);
        path_perimeter = path_perimeter + dist;
    }
    // last to first
    double delta_x_sqr = std::pow((std::prev(input_path.end(),1)->X - input_path.begin()->X), 2);
    double delta_y_sqr = std::pow((std::prev(input_path.end(),1)->Y - input_path.begin()->Y), 2);
    double dist = std::sqrt(delta_x_sqr+delta_y_sqr);
    path_perimeter = path_perimeter + dist;
    double tolerance = std::ceil(0.01*PI*path_perimeter);//1% of perimeter tolerance
    if(tolerance>min_distance_between_points)
        tolerance = min_distance_between_points;
    //remove bunched points
    for (auto path_it = perim_start; path_it != path_end; path_it++)
    {
        ClipperLib::IntPoint prev_pt = *std::prev(path_it);
        ClipperLib::IntPoint curr_pt = *path_it;
        double delta_x_sqr = std::pow((curr_pt.X - prev_pt.X), 2);
        double delta_y_sqr = std::pow((curr_pt.Y - prev_pt.Y), 2);
        double dist = std::sqrt(delta_x_sqr+delta_y_sqr);
        if(dist<tolerance)
        {
            //delete value, then move one back because loop steps you forward again
            path_it = std::prev(input_path.erase(path_it),1);
            path_end = input_path.end();
        }
    }
}

void MODEL3D::three_dim_model::repair_circles(ClipperLib::Paths &target_paths)
{
    auto paths_end = target_paths.end();
    for (auto path_it = target_paths.begin(); path_it!= paths_end; path_it++)
    {
        if(circle_test(*path_it, 0.1)==true)
        {
            reconstruct_circle(*path_it);
        }
    }
}

// Calculate average distance to centroid.
// if shape falls outside <tolerance> of average distance return false
// return true if all shapes pass the test.
bool MODEL3D::three_dim_model::circle_test(ClipperLib::Path &target_path, const double &tolerance)
{
    ClipperLib::IntPoint centroid = calculate_centroid(target_path);
    auto tp_end = target_path.end();
    double sigma_distance = 0;
    double avg_distance = 0;
    double lower_bound = 0;
    double upper_bound = 0;
    for (auto tp_it = target_path.begin(); tp_it != tp_end; tp_it++)
    {
        sigma_distance = sigma_distance + calculate_distance(centroid, *tp_it);
    }
    avg_distance = sigma_distance/target_path.size();
    lower_bound = avg_distance*(1-tolerance);
    upper_bound = avg_distance*(1+tolerance);
    for (auto tp_it = target_path.begin(); tp_it != tp_end; tp_it++)
    {
        double current_distance = calculate_distance(centroid, *tp_it);
        if ( (current_distance < lower_bound) || (current_distance > upper_bound) )
            return false;
    }
    //passed tolerance check, now checking area
    double polygon_area = ClipperLib::Area(target_path);
    int circle_area = (int)(std::round( PI * std::pow(avg_distance,2) ) ); //Average distance is radius.
    lower_bound = circle_area*(1-tolerance);
    upper_bound = circle_area*(1+tolerance);
    if ( (polygon_area < lower_bound) || (polygon_area > upper_bound) )
        return false;
    return true;
}

void MODEL3D::three_dim_model::calc_bounding_box(const ClipperLib::Path &target_path, ClipperLib::Path &bounding_box)
{
    auto path_end = target_path.end();
    ClipperLib::IntPoint bl = *target_path.begin();
    ClipperLib::IntPoint tr = *target_path.begin();
    for (auto path_it = target_path.begin(); path_it != path_end; path_it++)
    {
        if ( path_it->X < bl.X)
            bl.X = path_it->X;
        if ( path_it->Y < bl.Y)
            bl.Y = path_it->Y;
        if ( path_it->X > tr.X)
            tr.X = path_it->X;
        if ( path_it->Y > tr.Y)
            tr.Y = path_it->Y;
    }
    bounding_box.clear();
    bounding_box.insert(bounding_box.end(), {bl,tr});
}

ClipperLib::IntPoint MODEL3D::three_dim_model::calculate_centroid(const ClipperLib::Path &target_polygon)
{
    int vertex_count = target_polygon.size();
    double centroid[] = {0, 0};
    ClipperLib::IntPoint result;
    double signed_area = 0.0;
    double x_0 = 0.0; // Current vertex X
    double y_0 = 0.0; // Current vertex Y
    double x_1 = 0.0; // Next vertex X
    double y_1 = 0.0; // Next vertex Y
    double a = 0.0;  // Partial signed area

    // For all vertices except last
    int i=0;
    for (i=0; i<vertex_count-1; ++i)
    {
        x_0 = target_polygon[i].X;
        y_0 = target_polygon[i].Y;
        x_1 = target_polygon[i+1].X;
        y_1 = target_polygon[i+1].Y;
        a = x_0*y_1 - x_1*y_0;
        signed_area += a;
        centroid[0] += (x_0 + x_1)*a;
        centroid[1] += (y_0 + y_1)*a;
    }
    x_0 = target_polygon[i].X;
    y_0 = target_polygon[i].Y;
    x_1 = target_polygon[0].X;
    y_1 = target_polygon[0].Y;
    a = x_0*y_1 - x_1*y_0;
    signed_area += a;
    centroid[0] += (x_0 + x_1)*a;
    centroid[1] += (y_0 + y_1)*a;
    signed_area *= 0.5;
    centroid[0] /= (6.0*signed_area);
    centroid[1] /= (6.0*signed_area);
    result.X = std::round(centroid[0]);
    result.Y = std::round(centroid[1]);
    return result;
}

double MODEL3D::three_dim_model::calculate_distance
(const ClipperLib::IntPoint &a, const ClipperLib::IntPoint &b)
{
    double del_xp2 = std::pow(b.X-a.X,2);
    double del_yp2 = std::pow(b.Y-a.Y,2);
    double dist = std::round(sqrt(del_xp2+del_yp2));
    return dist;
}

// Once a shape has been identified as a circle, replaces it with a clean
// circle. Uses 32 points.
void MODEL3D::three_dim_model::reconstruct_circle(ClipperLib::Path &circle)
{
    ClipperLib::IntPoint midpoint = calculate_centroid(circle);
    double area = ClipperLib::Area(circle);
    double radius = std::sqrt(area/PI);
    double theta = -11.25; //360 degrees / 32 (sign for clockwise)
    ClipperLib::IntPoint start_point;
    start_point.X = midpoint.X - (int)std::round(radius);
    start_point.Y = midpoint.Y;
    circle.clear();
    circle.push_back(start_point);
    for (int i = 1; i < 32; i++)
    {
        double angle = theta*i;
        ClipperLib::IntPoint current_point =  rotate_IntPoint(midpoint, start_point, angle);
        circle.push_back(current_point);
    }
}

ClipperLib::IntPoint MODEL3D::three_dim_model::rotate_IntPoint
(const ClipperLib::IntPoint &origin, const ClipperLib::IntPoint &subject,
 const double &theta )
{
    ClipperLib::IntPoint p = subject;
    double t = theta * PI / 180; // convert angle to radians
    double s = sin(t);
    double c = cos(t);

    // translate point back to origin:
    p.X -= origin.X;
    p.Y -= origin.Y;

    // rotate point
    double xnew = p.X * c - p.Y * s;
    double ynew = p.X * s + p.Y * c;

    // translate point back:
    p.X = (xnew + origin.X);
    p.Y = (ynew + origin.Y);
    return p;
}