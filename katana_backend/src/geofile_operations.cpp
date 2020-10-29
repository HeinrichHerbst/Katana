/**
 * Author:      H.F. Herbst
 * Origin:      Stellenbosch University
 * For:         IARPA SuperTools Project
 * license:     MIT
 * Description: Function definitions for FLOOXS for Gmsh .geo geometry file manipulation.
 */
#include <cmath>
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <set>
#include <boost/algorithm/string.hpp>
#include "geofile_operations.hpp"

//Initialization of object.
GEO::geofile::geofile(/* args */)
{
    mesh_spacing = 0;
}
GEO::geofile::~geofile()
{
}

bool GEO::compare_point(const point &first, const point &second)
{
    if (first.x==second.x)
    {
        if (first.y==second.y)
        {
            if (first.z==second.z)
            {
                return true;
            }
        }
    }
    return false;
}

// Returns 1 if lines are identical
// Returns 2 if lines are reversed
// Returns 3 if lines are different
int GEO::compare_line(const line &first, const line &second)
{
    if (first.start==second.start)
    {
        if (first.end==second.end)
        {
            return 1;
        }
    }
    else if (first.start==second.end)
        return 2;
    return 3;
}

bool GEO::check_same_vec(const std::vector<int> &first, const std::vector<int> &second)
{
    if( first.size() == second.size() )
    {
        auto first_end = first.end();
        for ( auto first_it = first.begin(); first_it != first_end; first_it++)
        {
            auto second_end = second.end();
            for ( auto second_it = second.begin(); second_it != second_end; second_it++)
            {
                if (*first_it!=*second_it)
                    return  false;
            }
        }
        return true;
    }
    return false;
}
// Returns 1 if vectors are identical
// Returns 2 if vectors are reversed
// Returns 3 if vectors are different
int GEO::compare_vector(const std::vector<int> &first, const std::vector<int> &second)
{
    if (check_same_vec(first,second))
        return 1;
    else
    {
        auto reversed = second;
        std::reverse(reversed.begin(),reversed.end());
        if (check_same_vec(first,reversed))
            return 2;
        else
            return 3;
    }
}

GEO::geo_argument GEO::hashit(std::string const& inString)
{
    if (inString == "Point")            return GEO::ePoint;
    if (inString == "cl__1")            return GEO::eChar_Len;
    if (inString == "Line")             return GEO::eLine;
    if (inString == "Line Loop")        return GEO::eCurve_Loop;
    if (inString == "Curve Loop")       return GEO::eCurve_Loop;
    if (inString == "Plane Surface")    return GEO::ePlane_surface;
    if (inString == "Surface")          return GEO::ePlane_surface;
    if (inString == "Surface Loop")     return GEO::eSurface_loop;
    if (inString == "Volume")           return GEO::eVolume;
    if (inString == "Physical Point")   return GEO::ePhysical_Point;
    if (inString == "Physical Line")    return GEO::ePhysical_Curve;
    if (inString == "Physical Curve")   return GEO::ePhysical_Curve;
    if (inString == "Physical Surface") return GEO::ePhysical_Surface;
    if (inString == "Physical Volume")  return GEO::ePhysical_Volume;
    if (inString == "MeshSpac")         return GEO::eMesh_Spacing;
    if (inString == "SetFactory")       return GEO::eOpenCASCADE;
    if (inString == "")                 return GEO::eBlank_Space;
    return GEO::eDefault;
}
int GEO::geofile::import_geofile(std::string import_path)
{
    std::fstream geo_file;
    std::string file_buffer;
    geo_file.open(import_path, std::ios::in);
    if (geo_file.is_open())
    {
        int line_number =1;
        while (getline(geo_file, file_buffer))
        {
            std::vector<std::string> split_string_vector;
            boost::split(split_string_vector, file_buffer, boost::is_any_of("(),{}="));
            trim_string_vector(split_string_vector);
            geo_argument command = hashit(split_string_vector[0]);
            switch (command)
            {
            case ePoint:
                if(import_point(split_string_vector, import_path, line_number)==EXIT_FAILURE)
                    return EXIT_FAILURE;
                break;
            case eLine:
                if(import_line(split_string_vector, import_path, line_number)==EXIT_FAILURE)
                    return EXIT_FAILURE;
                break;
            case eCurve_Loop:
                if(import_curve_loop(split_string_vector, import_path, line_number)==EXIT_FAILURE)
                    return EXIT_FAILURE;
                break;
            case ePlane_surface:
                if(import_plane_surface(split_string_vector, import_path, line_number)==EXIT_FAILURE)
                    return EXIT_FAILURE;
                break;
            case eMesh_Spacing:
                if(import_mesh_spacing(split_string_vector)==EXIT_FAILURE)
                    return EXIT_FAILURE;
                break;
            case eOpenCASCADE:
                switch_to_OpenCASCADE();
                break;
            case eSurface_loop:
                if(import_surface_loop(split_string_vector, import_path, line_number)==EXIT_FAILURE)
                    return EXIT_FAILURE;
                break;
            case eVolume:
                if(import_volume(split_string_vector, import_path, line_number)==EXIT_FAILURE)
                    return EXIT_FAILURE;
                break;
            case ePhysical_Point:
                if(import_physical_entity(  split_string_vector, import_path,
                                            line_number, command)
                                            == EXIT_FAILURE )
                    return EXIT_FAILURE;
                break;
            case ePhysical_Curve:
                if(import_physical_entity(  split_string_vector, import_path,
                                            line_number, command)
                                            == EXIT_FAILURE )
                    return EXIT_FAILURE;
                break;
            case ePhysical_Surface:
                if(import_physical_entity(  split_string_vector, import_path,
                                            line_number, command)
                                            == EXIT_FAILURE )
                    return EXIT_FAILURE;
                break;
            case ePhysical_Volume:
                if(import_physical_entity(  split_string_vector, import_path,
                                            line_number, command)
                                            == EXIT_FAILURE )
                    return EXIT_FAILURE;
                break;
            case eBlank_Space:
                break;
            case eChar_Len:
                if(import_mesh_spacing(split_string_vector)==EXIT_FAILURE)
                    return EXIT_FAILURE;
                break;
            default:
                if (!file_buffer.rfind("//", 0) == 0) {
                    std::cout   << "Error: Unrecognized input in "
                                << import_path << " at line "
                                << line_number << "." << std::endl;
                    std::cout   << "Violation: "
                                << file_buffer << std::endl;
                    return EXIT_FAILURE;
                }
                break;
            }
            line_number++;
        }
        geo_file.close();
    }
    else
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

int GEO::geofile::import_point( std::vector<std::string> &split_string_vector,
                                std::string &import_path,
                                int &line_number)
{
    if ( (is_integer(split_string_vector[1])==true)&&
         (is_e_notation(split_string_vector[2])==true)&&
         (is_e_notation(split_string_vector[3])==true)&&
         (is_e_notation(split_string_vector[4])==true)&&
         ( (is_e_notation(split_string_vector[5])==true)||
           (split_string_vector[5]=="MeshSpac")||
           (split_string_vector[5]=="cl__1") )||
           (split_string_vector[5]==";") )
    {
        point input_point;
        input_point.x = std::stod(split_string_vector[2]);
        input_point.y = std::stod(split_string_vector[3]);
        input_point.z = std::stod(split_string_vector[4]);
        if( (split_string_vector[5]=="MeshSpac")||
            (split_string_vector[5]=="cl__1") )
        {
            input_point.char_len = mesh_spacing;
        }
        else
        {
            if (split_string_vector[5]!=";")
                input_point.char_len = std::stod(split_string_vector[5]);
            else
                input_point.char_len = 0;
        }
        points_map.insert({std::stoi(split_string_vector[1]), input_point});
        return EXIT_SUCCESS;
    }
    else
    {
        std::cout << "Error: Invalid value (line "
        << line_number << " of "
        << import_path << ")" << std::endl;
        return EXIT_FAILURE;
    }
}

int GEO::geofile::import_line( std::vector<std::string> &split_string_vector,
                                std::string &import_path,
                                int &line_number)
{
    if ((is_integer(split_string_vector[1])==true)&&
        (is_integer(split_string_vector[2])==true)&&
        (is_integer(split_string_vector[3])==true))
    {
        line input_line;
        auto start = std::stoi(split_string_vector[2]);
        auto end   = std::stoi(split_string_vector[3]);
        input_line.start = start;
        input_line.end = end;
        lines_map.insert({std::stoi(split_string_vector[1]), input_line});
        return EXIT_SUCCESS;
    }
    else
    {
        std::cout << "Error: Invalid value (line "
        << line_number << " of "
        << import_path << ")" << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int GEO::geofile::import_curve_loop( std::vector<std::string> &split_string_vector,
                                std::string &import_path,
                                int &line_number)
{
    auto it = split_string_vector.begin();
    it++;
    int key = std::stod(*it);
    it++;
    bool proceed = true;
    auto ssv_end = std::prev(split_string_vector.end(),1);//don't check semicolon.
    while (it!=ssv_end)
    {
        if (!is_integer(*it)) proceed = false;
        it++;
    }
    if (proceed==false)
    {
        std::cout << "Error: Invalid value (line "
        << line_number << " of "
        << import_path << ")" << std::endl;
        return EXIT_FAILURE;
    }
    it = std::next(split_string_vector.begin(),2);
    std::vector<int> curve_references;
    while (it!=ssv_end)
    {
        curve_references.push_back(std::stod(*it));
        it++;
    }
    curve_loops_map.insert({key,curve_references});
    return EXIT_SUCCESS;
}

int GEO::geofile::import_plane_surface( std::vector<std::string> &split_string_vector,
                                std::string &import_path,
                                int &line_number)
{
    auto it = split_string_vector.begin();
    split_string_vector.erase(it);
    it = std::prev(split_string_vector.end(),1);
    split_string_vector.erase(it);
    it = split_string_vector.begin();
    bool proceed = true;
    auto ssv_end = split_string_vector.end();
    while (it!=ssv_end)
    {
        if (!is_integer(*it)) proceed = false;
        it++;
    }
    if (proceed==false)
    {
        std::cout << "Error: Invalid value (line "
        << line_number << " of "
        << import_path << ")" << std::endl;
        return EXIT_FAILURE;
    }
    it = split_string_vector.begin();
    int key = std::stod(*it);
    it++;
    plane_surfaces_map.insert({key,std::stod(*it)});
    return EXIT_SUCCESS;
}

int GEO::geofile::import_surface_loop(  std::vector<std::string> &split_string_vector,
                                        std::string &import_path,
                                        int &line_number)
{
    auto it = split_string_vector.begin();
    split_string_vector.erase(it);
    it = std::prev(split_string_vector.end(),1);
    split_string_vector.erase(it);
    it = split_string_vector.begin();
    bool proceed = true;
    auto ssv_end = split_string_vector.end();
    while (it!=ssv_end)
    {
        if (!is_integer(*it)) proceed = false;
        it++;
    }
    if (proceed==false)
    {
        std::cout << "Error: Invalid value (line "
        << line_number << " of "
        << import_path << ")" << std::endl;
        return EXIT_FAILURE;
    }
    it = split_string_vector.begin();
    int key = std::stod(*it);
    it++;
    std::vector<int> surface_references;
    while (it!=ssv_end)
    {
        surface_references.push_back(std::stod(*it));
        it++;
    }
    surface_loops_map.insert({key,surface_references});
    return EXIT_SUCCESS;
}

int GEO::geofile::import_volume(    std::vector<std::string> &split_string_vector,
                                    std::string &import_path,
                                    int &line_number)
{
    auto it = split_string_vector.begin();
    split_string_vector.erase(it);
    it= std::prev(split_string_vector.end(),1);
    split_string_vector.erase(it);
    it = split_string_vector.begin();
    bool proceed = true;
    while (it!=split_string_vector.end())
    {
        if (!is_integer(*it)) proceed = false;
        it++;
    }
    if (proceed==false)
    {
        std::cout << "Error: Invalid value (line "
        << line_number << " of "
        << import_path << ")" << std::endl;
        return EXIT_FAILURE;
    }
    it = split_string_vector.begin();
    int key = std::stod(*it);
    it++;
    volumes_map.insert({key,std::stod(*it)});
    return EXIT_SUCCESS;
}

int GEO::geofile::import_physical_entity(   std::vector<std::string> &split_string_vector,
                                            std::string &import_path,
                                            int &line_number,
                                            geo_argument &physical_type)
{
    auto it = split_string_vector.begin();
    split_string_vector.erase(it);
    boost::erase_all( *it, "\"" );
    it=std::prev(split_string_vector.end(),1);
    split_string_vector.erase(it);
    it = split_string_vector.begin();
    std::string group_name = *it++;
    std::vector<int> elements;
    auto ssv_end = split_string_vector.end();
    while (it != ssv_end)
    {
        if (is_integer(*it))
        {
            elements.push_back(std::stoi(*it++));
        }
        else
        {
            std::cout << "Error: Invalid value (line "
            << line_number << " of "
            << import_path << ")" << std::endl;
            return EXIT_FAILURE;
        }
    }
    switch (physical_type)
    {
    case ePhysical_Point:
        physical_points_map.insert({group_name, elements});
        break;
    case ePhysical_Curve:
        physical_curves_map.insert({group_name, elements});
        break;
    case ePhysical_Surface:
        physical_surfaces_map.insert({group_name, elements});
        break;
    case ePhysical_Volume:
        physical_volumes_map.insert({group_name, elements});
        break;
    default:
        std:: cout  << "Error: Unrecognized Physical Element at line "
                    << line_number << " of "
                    << import_path << "." << std::endl;
        return EXIT_FAILURE;
        break;
    }
    return EXIT_SUCCESS;
}

int GEO::geofile::import_mesh_spacing(std::vector<std::string> &split_string_vector)
{
    boost::erase_all(split_string_vector[1], ";");
    if (is_e_notation(split_string_vector[1]))
    {
        std::string msh_spc = split_string_vector[1];
        mesh_spacing = std::stod(msh_spc);
        return EXIT_SUCCESS;
    }
    else
        return EXIT_FAILURE;
}

bool GEO::is_integer(std::string const& n) noexcept
{
    if (std::isdigit(n[0]) || (n.size() > 1 && (n[0] == '-' || n[0] == '+')))
    {
        for (std::string::size_type i{ 1 }; i < n.size(); ++i)
            if (!std::isdigit(n[i]))
                return false;
        return true;
    }
    return false;
}

bool GEO::is_e_notation(std::string const& n) noexcept
{
    if (std::isdigit(n[0]) || (n.size() > 1 && (n[0] == '-' || n[0] == '+')))
        {
            for (std::string::size_type i{ 1 }; i < n.size(); ++i)
                if (!( ( std::isdigit(n[i]) ) ||
                       (n[i]=='e') ||
                       (n[i]=='.') ||
                       (n[i]=='+') ||
                       (n[i]=='-')    ) )
                    return false;
            return true;
        }
        return false;
}

//Remove whitespace
void GEO::trim_string_vector(std::vector<std::string> &invec)
{
    auto it = invec.begin();
    while (it != invec.end())
        boost::trim(*it++);
    remove_empty_strings(invec);
}

void GEO::remove_empty_strings(std::vector<std::string>& input_string)
{
  std::vector<std::string>::iterator it =
  remove_if(input_string.begin(), input_string.end(),std::mem_fun_ref(&std::string::empty));
  // erase the removed elements
  input_string.erase(it, input_string.end());
}

int GEO::geofile::export_geofile(std::string export_path)
{
    if(points_map.empty())
    {
        std::cout   << "Error: No points detected. Have you imported a .geo file?"
                    << std::endl;
        return EXIT_FAILURE;
    }
    std::ofstream gf_geometry_file;
    std::string file_output_name = export_path;
    time_t log_attempt_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    gf_geometry_file.open( file_output_name, std::ios::out);
    gf_geometry_file << "// -------------------------------------------------------------- //" << "\n";
    gf_geometry_file << "//                    _                                           //" << "\n";
    gf_geometry_file << "//          _________| |___________________________________       //" << "\n";
    gf_geometry_file << "//         [_x_x_x_x_   ______________Katana______________/       //" << "\n";
    gf_geometry_file << "//                   |_|                                          //" << "\n";
    gf_geometry_file << "//                                                                //" << "\n";
    gf_geometry_file << "//                      Katana Terminal Edition                   //" << "\n";
    gf_geometry_file << "//    TCAD Tools for the Superconducting Electronics Community    //" << "\n";
    gf_geometry_file << "// -------------------------------------------------------------- //" << "\n";
    gf_geometry_file << "// File creation date = " << std::ctime(&log_attempt_time);
    gf_geometry_file << "//\n";
    if(is_open_cascade==true){gf_geometry_file << "SetFactory(\"OpenCASCADE\");\n";}
    gf_geometry_file << "// ============================ POINTS ============================" << "\n";
    auto p_iter     = points_map.begin();
    auto p_iter_end = points_map.end();
    while (p_iter != p_iter_end)
    {
        if( (ignore_char_len==false) && (p_iter->second.char_len != 0) )
        {
            gf_geometry_file    << "Point(" << p_iter->first << ") = {"
                                << p_iter->second.x << ", "
                                << p_iter->second.y << ", "
                                << p_iter->second.z << ", "
                                << p_iter->second.char_len
                                << "};"             << "\n";
        }
        else
        {
            gf_geometry_file    << "Point(" << p_iter->first << ") = {"
                                << p_iter->second.x << ", "
                                << p_iter->second.y << ", "
                                << p_iter->second.z
                                << "};"             << "\n";
        }
        p_iter++;
    }
    gf_geometry_file << "// ============================ LINES =============================" << "\n";
    auto l_iter     = lines_map.begin();
    auto l_iter_end = lines_map.end();
    while (l_iter != l_iter_end)
    {
        // Line(5) = {1, 5};
            gf_geometry_file    << "Line("  << l_iter->first
                                << ") = {"
                                << l_iter->second.start
                                << ", "
                                << l_iter->second.end
                                << "};"     << "\n";
        l_iter++;
    }
    gf_geometry_file << "// ========================= CURVE LOOPS ==========================" << "\n";
    auto cl_iter     = curve_loops_map.begin();
    auto cl_iter_end = curve_loops_map.end();
    while (cl_iter != cl_iter_end)
    {
        std::ostringstream curve_loop;
        auto loop_iter      = cl_iter->second.begin();
        auto loop_iter_last  = --cl_iter->second.end();
        while (loop_iter != loop_iter_last)
        {
            curve_loop << *loop_iter++ << ", ";
        }
        curve_loop << *loop_iter;
        std::string loop_string = curve_loop.str();
        // Curve Loop(1) = {12, -3, -11, -9};
        gf_geometry_file    << "Curve Loop("  << cl_iter->first
                            << ") = {"
                            << loop_string
                            << "};"     << "\n";
        cl_iter++;
    }
    gf_geometry_file << "// ======================== PLANE SURFACES ========================" << "\n";
    auto pls_iter     = plane_surfaces_map.begin();
    auto pls_iter_end = plane_surfaces_map.end();
    while (pls_iter != pls_iter_end)
    {
        // Plane Surface(16) = {16};
        gf_geometry_file    << "Plane Surface("  << pls_iter->first
                            << ") = {"
                            << pls_iter->second
                            << "};"     << "\n";
        pls_iter++;
    }
    gf_geometry_file << "// ======================== SURFACE LOOPS =========================" << "\n";
    auto sl_iter     = surface_loops_map.begin();
    auto sl_iter_end = surface_loops_map.end();
    while (sl_iter != sl_iter_end)
    {
        std::ostringstream surface_loop;
        auto inside_loop_iter       = sl_iter->second.begin();
        auto inside_loop_iter_last  = --sl_iter->second.end();
        while (inside_loop_iter != inside_loop_iter_last)
        {
            surface_loop << *inside_loop_iter++ << ", ";
        }
        surface_loop << *inside_loop_iter;
        std::string sl_string = surface_loop.str();
        gf_geometry_file    << "Surface Loop("  << sl_iter->first
                            << ") = {"
                            << sl_string
                            << "};"     << "\n";
        sl_iter++;
    }
    gf_geometry_file << "// =========================== VOLUMES ============================" << "\n";
    //Volume(1) = {1};
    auto vol_iter     = volumes_map.begin();
    auto vol_iter_end = volumes_map.end();
    while (vol_iter != vol_iter_end)
    {
        gf_geometry_file    << "Volume("  << vol_iter->first
                            << ") = {"
                            << vol_iter->second
                            << "};"     << "\n";
        vol_iter++;
    }
    gf_geometry_file << "// ======================= PHYSICAL POINTS ========================" << "\n";
    auto pp_iter        = physical_points_map.begin();
    auto pp_iter_end    = physical_points_map.end();
    while (pp_iter != pp_iter_end)
    {
        std::ostringstream pp_loop;
        auto pp_inside_loop_iter       = pp_iter->second.begin();
        auto pp_inside_loop_iter_last  = --pp_iter->second.end();
        while (pp_inside_loop_iter != pp_inside_loop_iter_last)
        {
            pp_loop << *pp_inside_loop_iter++ << ", ";
        }
        pp_loop << *pp_inside_loop_iter;
        std::string pp_string = pp_loop.str();
        gf_geometry_file    << "Physical Point(\""  << pp_iter->first
                            << "\") = {"
                            << pp_string
                            << "};"     << "\n";
        pp_iter++;
    }
    gf_geometry_file << "// ======================= PHYSICAL CURVES ========================" << "\n";
    auto pc_iter        = physical_curves_map.begin();
    auto pc_iter_end    = physical_curves_map.end();
    while (pc_iter != pc_iter_end)
    {
        std::ostringstream pc_loop;
        auto pc_inside_loop_iter       = pc_iter->second.begin();
        auto pc_inside_loop_iter_last  = --pc_iter->second.end();
        while (pc_inside_loop_iter != pc_inside_loop_iter_last)
        {
            pc_loop << *pc_inside_loop_iter++ << ", ";
        }
        pc_loop << *pc_inside_loop_iter;
        std::string pc_string = pc_loop.str();
        gf_geometry_file    << "Physical Curve(\""  << pc_iter->first
                            << "\") = {"
                            << pc_string
                            << "};"     << "\n";
        pc_iter++;
    }
    gf_geometry_file << "// ====================== PHYSICAL SURFACES =======================" << "\n";
    auto ps_iter        = physical_surfaces_map.begin();
    auto ps_iter_end    = physical_surfaces_map.end();
    while (ps_iter != ps_iter_end)
    {
        std::ostringstream ps_loop;
        auto ps_inside_loop_iter       = ps_iter->second.begin();
        auto ps_inside_loop_iter_last  = --ps_iter->second.end();
        while (ps_inside_loop_iter != ps_inside_loop_iter_last)
        {
            ps_loop << *ps_inside_loop_iter++ << ", ";
        }
        ps_loop << *ps_inside_loop_iter;
        std::string ps_string = ps_loop.str();
        gf_geometry_file    << "Physical Surface(\""  << ps_iter->first
                            << "\") = {"
                            << ps_string
                            << "};"     << "\n";
        ps_iter++;
    }
    gf_geometry_file << "// ====================== PHYSICAL VOLUMES ========================" << "\n";
    auto pv_iter        = physical_volumes_map.begin();
    auto pv_iter_end    = physical_volumes_map.end();
    while (pv_iter != pv_iter_end)
    {
        std::ostringstream pv_loop;
        auto pv_inside_loop_iter       = pv_iter->second.begin();
        auto pv_inside_loop_iter_last  = --pv_iter->second.end();
        while (pv_inside_loop_iter != pv_inside_loop_iter_last)
        {
            pv_loop << *pv_inside_loop_iter++ << ", ";
        }
        pv_loop << *pv_inside_loop_iter;
        std::string pv_string = pv_loop.str();
        gf_geometry_file    << "Physical Volume(\""  << pv_iter->first
                            << "\") = {"
                            << pv_string
                            << "};"     << "\n";
        pv_iter++;
    }
    if(bool_diff_rest==true)
    {
        gf_geometry_file << "BooleanDifference{ Volume{1}; Delete; }{\n";
        auto vol_start = std::next(volumes_map.begin(),1);
        auto vol_end = volumes_map.end();
        int vol_count = 0;
        for(auto vol_it = vol_start; vol_it!=vol_end;vol_it++)
        {
            vol_count = std::distance(volumes_map.begin(),vol_it)+1;
            gf_geometry_file << "Volume{"<<vol_count<<"};\n";
        }
        gf_geometry_file << "Delete; }\n";
    }
    gf_geometry_file.close();
    return EXIT_SUCCESS;
}

// Shift higher indices into sequential gaps to remove gaps from data.
int GEO::geofile::simplify_data()
{
    if (points_map.empty())
    {
        std::cout   << "Error: No points detected. Have you imported a .geo file?"
                    << std::endl;
        return EXIT_FAILURE;
    }
    int status = EXIT_SUCCESS;
    for ( int current_simplify = 0;
        (current_simplify < 6)&&(!status);
        current_simplify++)
    {
        status = simplify_element(current_simplify);
    }
    return status;
}

int GEO::geofile::simplify_element(int &current_status)
{
    switch (current_status)
    {
    case 0:
        if(!points_map.empty())
            simplify_points();
        break;
    case 1:
        if(!lines_map.empty())
            simplify_lines();
        break;
    case 2:
        if(!curve_loops_map.empty())
            simplify_curve_loops();
        break;
    case 3:
        if(!plane_surfaces_map.empty())
            simplify_plane_surfaces();
        break;
    case 4:
        if(!surface_loops_map.empty())
            simplify_surface_loops();
        break;
    case 5:
        if(!volumes_map.empty())
            simplify_volumes();
        break;
    default:
        std::cout   << "Error: Unknown state in GEO::geofile::simplify_element."
                    << std::endl;
        return EXIT_FAILURE;
        break;
    }
    return EXIT_SUCCESS;
}

void GEO::geofile::perform_shift(   std::set<int> &gaps,
                                    std::set<int> &numbers,
                                    std::map<int, int>  &changes)
{
    auto a = numbers.begin();
    auto b = gaps.begin();
    while ((!gaps.empty())&&(*gaps.begin() < *std::prev(numbers.end(),1) ))
    {
        if (*a > *b)
        {
            int num_hold = *a;
            int gap_hold = *b;
            gaps.erase(gaps.find(gap_hold));
            numbers.erase(numbers.find(num_hold));
            gaps.insert(num_hold);
            numbers.insert(gap_hold);
            changes.insert({num_hold,gap_hold});
            //reassess lowest point and gap
            a = numbers.begin();
            b = gaps.begin();
        }
        else
        {
            a++;
        }
    }
}

int GEO::geofile::simplify_points()
{
    std::set<int>       gaps;
    std::set<int>       numbers;
    std::map<int, int>  changes;
    // Store all gaps
    // before start
    if (points_map.begin()->first>1)
    {
        int total_pre_gaps = points_map.begin()->first - 1;
        for (size_t i = 1; i <= total_pre_gaps; i++)
        {
            gaps.insert(i);
        }
    }
    auto point_it_end   = std::prev(points_map.end(),1);
    for (auto point_it = points_map.begin(); point_it!=point_it_end; point_it++)
    {
        auto look_ahead = std::next(point_it,1);
        for (int i = point_it->first+1; i < look_ahead->first; i++)
        {
            gaps.insert(i);
        }
    }
    if (!gaps.empty())
    {
        // Store all non-gaps
        point_it_end = points_map.end();
        for (auto point_it = points_map.begin(); point_it != point_it_end; point_it++)
        {
            numbers.insert(point_it->first);
        }
        perform_shift(gaps, numbers, changes);
        return adjust_points(changes);
    }
    return EXIT_SUCCESS;
}

int GEO::geofile::adjust_points(std::map<int,int> changes)
{
    auto change_end = changes.end();
    for (auto i = changes.begin(); i != change_end; i++)
    {
        auto nodeHandler = points_map.extract(i->first);
        if(!nodeHandler.empty())
            {
                nodeHandler.key() = i->second;
                points_map.insert(std::move(nodeHandler));
            }
    }
    // Adjust dependencies to mirror the changes.
    //Lines
    auto l_m_end  = lines_map.end();
    for (auto l_it = lines_map.begin(); l_it != l_m_end; l_it++)
    {
        for (auto i = changes.begin(); i != change_end; i++)
        {
            if (l_it->second.start == i->first)
                l_it->second.start = i->second;
            if (l_it->second.end == i->first)
                l_it->second.end = i->second;
        }
    }
    //Physical points
    auto phys_p_end = physical_points_map.end();
    for (auto phys_p_it = physical_points_map.begin(); phys_p_it != phys_p_end; phys_p_it++)
    {
        for (auto i = changes.begin(); i != change_end; i++)
        {
            auto vector_it_end = phys_p_it->second.end();
            for (auto vector_it = phys_p_it->second.begin(); vector_it != vector_it_end; vector_it++)
            {
                if(*vector_it==i->first)
                    *vector_it=i->second;
            }
        }
    }
    return EXIT_SUCCESS;
}

int GEO::geofile::simplify_lines()
{
    std::set<int>       gaps;
    std::set<int>       numbers;
    std::map<int, int>  changes;
    // Store all gaps
    // before start
    if (lines_map.begin()->first>1)
    {
        int total_pre_gaps = lines_map.begin()->first - 1;
        for (size_t i = 1; i <= total_pre_gaps; i++)
        {
            gaps.insert(i);
        }
    }
    // after start
    auto line_it_end   = std::prev(lines_map.end(),1);
    for (auto line_it = lines_map.begin(); line_it!=line_it_end; line_it++)
    {
        auto look_ahead = std::next(line_it,1);
        for (int i = line_it->first+1; i < look_ahead->first; i++)
        {
            gaps.insert(i);
        }
    }
    if (!gaps.empty())
    {
        // Store all non-gaps
        line_it_end = lines_map.end();
        for (auto line_it = lines_map.begin(); line_it != line_it_end; line_it++)
        {
            numbers.insert(line_it->first);
        }
        perform_shift(gaps, numbers, changes);
        return adjust_lines(changes);
    }
    return EXIT_SUCCESS;
}

int GEO::geofile::adjust_lines(std::map<int,int> changes)
{
    auto change_end = changes.end();
    for (auto i = changes.begin(); i != change_end; i++)
    {
        auto nodeHandler = lines_map.extract(i->first);
        if(!nodeHandler.empty())
        {
            nodeHandler.key() = i->second;
            lines_map.insert(std::move(nodeHandler));
        }
    }
    // Adjust dependencies to mirror the changes.
    // Curve loops
    auto c_l_m_end  = curve_loops_map.end();
    for (auto c_l_m_it = curve_loops_map.begin(); c_l_m_it != c_l_m_end; c_l_m_it++)
    {
        for (auto i = changes.begin(); i != change_end; i++)
        {
            auto vec_end = c_l_m_it->second.end();
            for (auto j = c_l_m_it->second.begin(); j != vec_end; j++)
            {
                if(*j==i->first)
                    *j=i->second;
                if(*j==-i->first)
                    *j=-i->second;
            }
        }
    }
    return EXIT_SUCCESS;
}

int GEO::geofile::simplify_curve_loops()
{
    std::set<int>       gaps;
    std::set<int>       numbers;
    std::map<int, int>  changes;
    // Store all gaps
    // before start
    if (curve_loops_map.begin()->first>1)
    {
        int total_pre_gaps = curve_loops_map.begin()->first - 1;
        for (size_t i = 1; i <= total_pre_gaps; i++)
        {
            gaps.insert(i);
        }
    }
    // after start
    auto curve_loop_it_end   = std::prev(curve_loops_map.end(),1);
    for (auto curve_loop_it = curve_loops_map.begin(); curve_loop_it!=curve_loop_it_end; curve_loop_it++)
    {
        auto look_ahead = std::next(curve_loop_it,1);
        for (int i = curve_loop_it->first+1; i < look_ahead->first; i++)
        {
            gaps.insert(i);
        }
    }
    if (!gaps.empty())
    {
        // Store all non-gaps
        curve_loop_it_end = curve_loops_map.end();
        for (auto curve_loop_it = curve_loops_map.begin(); curve_loop_it != curve_loop_it_end; curve_loop_it++)
        {
            numbers.insert(curve_loop_it->first);
        }
        perform_shift(gaps, numbers, changes);
        return adjust_curve_loops(changes);
    }
    return EXIT_SUCCESS;
}

int GEO::geofile::adjust_curve_loops(std::map<int,int> changes)
{
    auto change_end = changes.end();
    for (auto i = changes.begin(); i != change_end; i++)
    {
        auto nodeHandler = curve_loops_map.extract(i->first);
        if (!nodeHandler.empty())
        {
            nodeHandler.key() = i->second;
            curve_loops_map.insert(std::move(nodeHandler));
        }
    }
    // Adjust dependencies to mirror the changes.
    // Plane Surfaces
    auto pl_sur_map_end  = plane_surfaces_map.end();
    for (auto pl_sur_map_it = plane_surfaces_map.begin(); pl_sur_map_it != pl_sur_map_end; pl_sur_map_it++)
    {
        for (auto i = changes.begin(); i != change_end; i++)
        {
            // If plane surface references the original value
            // Then change it to the new value
            if(pl_sur_map_it->second==i->first)
                pl_sur_map_it->second=i->second;
        }
    }
    // Physical curves
    auto phys_s_end = physical_curves_map.end();
    for (auto phys_s_it = physical_curves_map.begin(); phys_s_it != phys_s_end; phys_s_it++)
    {
        for (auto i = changes.begin(); i != change_end; i++)
        {
            auto vector_it_end = phys_s_it->second.end();
            for (auto vector_it = phys_s_it->second.begin(); vector_it != vector_it_end; vector_it++)
            {
                if(*vector_it==i->first)
                    *vector_it=i->second;
            }
        }
    }
    return EXIT_SUCCESS;
}

int GEO::geofile::simplify_plane_surfaces()
{
    std::set<int>       gaps;
    std::set<int>       numbers;
    std::map<int, int>  changes;
    // Store all gaps
    // before start
    if (plane_surfaces_map.begin()->first>1)
    {
        int total_pre_gaps = plane_surfaces_map.begin()->first - 1;
        for (size_t i = 1; i <= total_pre_gaps; i++)
        {
            gaps.insert(i);
        }
    }
    // after start
    auto plane_surface_it_end   = std::prev(plane_surfaces_map.end(),1);
    for (auto pl_sur_it = plane_surfaces_map.begin(); pl_sur_it!=plane_surface_it_end; pl_sur_it++)
    {
        auto look_ahead = std::next(pl_sur_it,1);
        for (int i = pl_sur_it->first+1; i < look_ahead->first; i++)
        {
            gaps.insert(i);
        }
    }
    if (!gaps.empty())
    {
        // Store all non-gaps
        plane_surface_it_end = plane_surfaces_map.end();
        for (auto pl_sur_it = plane_surfaces_map.begin(); pl_sur_it != plane_surface_it_end; pl_sur_it++)
        {
            numbers.insert(pl_sur_it->first);
        }
        perform_shift(gaps, numbers, changes);
        return adjust_plane_surfaces(changes);
    }
    return EXIT_SUCCESS;
}

int GEO::geofile::adjust_plane_surfaces(std::map<int,int> changes)
{
    auto change_end = changes.end();
    for (auto i = changes.begin(); i != change_end; i++)
    {
        auto nodeHandler = plane_surfaces_map.extract(i->first);
        if(!nodeHandler.empty())
        {
            nodeHandler.key() = i->second;
            plane_surfaces_map.insert(std::move(nodeHandler));
        }
    }
    // Adjust dependencies to mirror the changes.
    // Surface Loops
    auto sur_loop_map_end  = surface_loops_map.end();
    for (auto sur_loop_map_it = surface_loops_map.begin(); sur_loop_map_it != sur_loop_map_end; sur_loop_map_it++)
    {
        for (auto i = changes.begin(); i != change_end; i++)
        {
            auto vector_it_end = sur_loop_map_it->second.end();
            for (auto vector_it = sur_loop_map_it->second.begin(); vector_it != vector_it_end; vector_it++)
            {
                if(*vector_it==i->first)
                    *vector_it=i->second;
            }
        }
    }
    // Physical surfaces
    auto phys_sur_end = physical_surfaces_map.end();
    for (auto phys_sur_it = physical_surfaces_map.begin(); phys_sur_it != phys_sur_end; phys_sur_it++)
    {
        for (auto i = changes.begin(); i != change_end; i++)
        {
            auto vector_it_end = phys_sur_it->second.end();
            for (auto vector_it = phys_sur_it->second.begin(); vector_it != vector_it_end; vector_it++)
            {
                if(*vector_it==i->first)
                    *vector_it=i->second;
            }
        }
    }
    return EXIT_SUCCESS;
}

int GEO::geofile::simplify_surface_loops()
{
    std::set<int>       gaps;
    std::set<int>       numbers;
    std::map<int, int>  changes;
    // Store all gaps
    // before start
    if (surface_loops_map.begin()->first>1)
    {
        int total_pre_gaps = surface_loops_map.begin()->first - 1;
        for (size_t i = 1; i <= total_pre_gaps; i++)
        {
            gaps.insert(i);
        }
    }
    // after start
    auto surf_loop_end   = std::prev(surface_loops_map.end(),1);
    for (auto sur_loop_it = surface_loops_map.begin(); sur_loop_it!=surf_loop_end; sur_loop_it++)
    {
        auto look_ahead = std::next(sur_loop_it,1);
        for (int i = sur_loop_it->first+1; i < look_ahead->first; i++)
        {
            gaps.insert(i);
        }
    }
    if (!gaps.empty())
    {
        // Store all non-gaps
        surf_loop_end = surface_loops_map.end();
        for (auto sur_loop_it = surface_loops_map.begin(); sur_loop_it != surf_loop_end; sur_loop_it++)
        {
            numbers.insert(sur_loop_it->first);
        }
        perform_shift(gaps, numbers, changes);
        return adjust_surface_loops(changes);
    }
    return EXIT_SUCCESS;
}

int GEO::geofile::adjust_surface_loops(std::map<int,int> changes)
{
    auto change_end = changes.end();
    for (auto i = changes.begin(); i != change_end; i++)
    {
        auto nodeHandler = surface_loops_map.extract(i->first);
        if(!nodeHandler.empty())
        {
            nodeHandler.key() = i->second;
            surface_loops_map.insert(std::move(nodeHandler));
        }
    }
    // Adjust dependencies to mirror the changes.
    // Volumes
    auto vol_map_end  = volumes_map.end();
    for (auto vol_map_it = volumes_map.begin(); vol_map_it != vol_map_end; vol_map_it++)
    {
        for (auto i = changes.begin(); i != change_end; i++)
        {
            if(vol_map_it->second==i->first)
                vol_map_it->second=i->second;
        }
    }
    return EXIT_SUCCESS;
}

int GEO::geofile::simplify_volumes()
{
    std::set<int>       gaps;
    std::set<int>       numbers;
    std::map<int, int>  changes;
    // Store all gaps
    // before start
    if (volumes_map.begin()->first>1)
    {
        int total_pre_gaps = volumes_map.begin()->first - 1;
        for (size_t i = 1; i <= total_pre_gaps; i++)
        {
            gaps.insert(i);
        }
    }
    // after start
    auto volumes_map_end = std::prev(volumes_map.end(),1);
    for (auto vol_map_it = volumes_map.begin(); vol_map_it != volumes_map_end; vol_map_it++)
    {
        auto look_ahead = std::next(vol_map_it,1);
        for (int i = vol_map_it->first+1; i < look_ahead->first; i++)
        {
            gaps.insert(i);
        }
    }
    if (!gaps.empty())
    {
        // Store all non-gaps
        volumes_map_end = volumes_map.end();
        for (auto vol_map_it = volumes_map.begin(); vol_map_it != volumes_map_end; vol_map_it++)
        {
            numbers.insert(vol_map_it->first);
        }
        perform_shift(gaps, numbers, changes);
        return adjust_volumes(changes);
    }
    return EXIT_SUCCESS;
}

int GEO::geofile::adjust_volumes(std::map<int,int> changes)
{
    auto change_end = changes.end();
    for (auto i = changes.begin(); i != change_end; i++)
    {
        auto nodeHandler = volumes_map.extract(i->first);
        if(!nodeHandler.empty())
        {
            nodeHandler.key() = i->second;
            volumes_map.insert(std::move(nodeHandler));
        }
    }
    // Adjust dependencies to mirror the changes.
    // Physical volumes
    auto phys_vol_map_end  = physical_volumes_map.end();
    for (auto phys_vol_map_it = physical_volumes_map.begin(); phys_vol_map_it != phys_vol_map_end; phys_vol_map_it++)
    {
        for (auto i = changes.begin(); i != change_end; i++)
        {
            auto vector_it_end = phys_vol_map_it->second.end();
            for (auto vector_it = phys_vol_map_it->second.begin(); vector_it != vector_it_end; vector_it++)
            {
                if(*vector_it==i->first)
                    *vector_it=i->second;
            }
        }
    }
    return EXIT_SUCCESS;
}

int GEO::geofile::merge_with(geofile &secondary_file)
{
// First simplify both files.
    make_coherent(true);
    secondary_file.make_coherent(true);
    simplify_data();
    secondary_file.simplify_data();
    if(!secondary_file.points_map.empty())
        merge_points(secondary_file);
    if(!secondary_file.lines_map.empty())
        merge_lines(secondary_file);
    if(!secondary_file.curve_loops_map.empty())
        merge_curve_loops(secondary_file);
    if(!secondary_file.plane_surfaces_map.empty())
        merge_plane_surfaces(secondary_file);
    if(!secondary_file.surface_loops_map.empty())
        merge_surface_loops(secondary_file);
    if(!secondary_file.volumes_map.empty())
        merge_volumes(secondary_file);
    merge_physical_entities(secondary_file);
    make_coherent(true);
    simplify_data();
    return EXIT_SUCCESS;
}

// For when you know there will be no conflicts. Simply appent map 2 to map 1
int GEO::geofile::quick_merge_with(geofile &secondary_file)
{
    points_map.insert(              secondary_file.points_map.begin(),
                                    secondary_file.points_map.end() );
    lines_map.insert(               secondary_file.lines_map.begin(),
                                    secondary_file.lines_map.end() );
    curve_loops_map.insert(         secondary_file.curve_loops_map.begin(),
                                    secondary_file.curve_loops_map.end() );
    plane_surfaces_map.insert(      secondary_file.plane_surfaces_map.begin(),
                                    secondary_file.plane_surfaces_map.end() );
    surface_loops_map.insert(       secondary_file.surface_loops_map.begin(),
                                    secondary_file.surface_loops_map.end() );
    volumes_map.insert(             secondary_file.volumes_map.begin(),
                                    secondary_file.volumes_map.end() );
    physical_points_map.insert(     secondary_file.physical_points_map.begin(),
                                    secondary_file.physical_points_map.end() );
    physical_curves_map.insert(     secondary_file.physical_curves_map.begin(),
                                    secondary_file.physical_curves_map.end() );
    physical_surfaces_map.insert(   secondary_file.physical_surfaces_map.begin(),
                                    secondary_file.physical_surfaces_map.end() );
    physical_volumes_map.insert(    secondary_file.physical_volumes_map.begin(),
                                    secondary_file.physical_volumes_map.end() );
    return EXIT_SUCCESS;
}

int GEO::geofile::merge_points(geofile &secondary_file)
{
    if (!points_map.empty())
    {
        int lowest_secondary    = secondary_file.points_map.begin()->first;
        int highest_primary     = std::prev(points_map.end())->first;
        int delta               = highest_primary - lowest_secondary + 1;
        std::map <int,int> changes;
        auto sec_point_map_end = secondary_file.points_map.end();
        for (   auto sec_point_map_it = secondary_file.points_map.begin();
                sec_point_map_it != sec_point_map_end;
                sec_point_map_it++)
        {
            changes.insert({ sec_point_map_it->first,
                            sec_point_map_it->first + delta});
        }
        adjust_points(changes);
        sec_point_map_end = secondary_file.points_map.end();
        for (   auto sec_point_map_it = secondary_file.points_map.begin();
                sec_point_map_it != sec_point_map_end;
                sec_point_map_it++)
        {
            points_map.insert({sec_point_map_it->first,sec_point_map_it->second});
        }
    }
    else
    {
        points_map = secondary_file.points_map;
    }
    return EXIT_SUCCESS;
}

int GEO::geofile::merge_lines(geofile &secondary_file)
{
    if (!lines_map.empty())
    {
        int lowest_secondary    = secondary_file.lines_map.begin()->first;
        int highest_primary     = std::prev(lines_map.end())->first;
        int delta               = highest_primary - lowest_secondary + 1;
        std::map <int,int> changes;
        auto sec_lines_map_end = secondary_file.lines_map.end();
        for (   auto sec_lines_map_it = secondary_file.lines_map.begin();
                sec_lines_map_it != sec_lines_map_end;
                sec_lines_map_it++)
        {
            changes.insert({ sec_lines_map_it->first,
                            sec_lines_map_it->first + delta});
        }
        adjust_lines(changes);
        sec_lines_map_end = secondary_file.lines_map.end();
        for (   auto sec_lines_map_it = secondary_file.lines_map.begin();
                sec_lines_map_it != sec_lines_map_end;
                sec_lines_map_it++)
        {
            lines_map.insert({sec_lines_map_it->first,sec_lines_map_it->second});
        }
    }
    else
    {
        lines_map = secondary_file.lines_map;
    }
    return EXIT_SUCCESS;
}

int GEO::geofile::merge_curve_loops(geofile &secondary_file)
{
    if (!curve_loops_map.empty())
    {
        int lowest_secondary    = secondary_file.curve_loops_map.begin()->first;
        int highest_primary     = std::prev(curve_loops_map.end())->first;
        int delta               = highest_primary - lowest_secondary + 1;
        std::map <int,int> changes;
        auto sec_c_l_map_end = secondary_file.curve_loops_map.end();
        for (   auto sec_c_l_it = secondary_file.curve_loops_map.begin();
                sec_c_l_it != sec_c_l_map_end;
                sec_c_l_it++)
        {
            changes.insert({ sec_c_l_it->first,
                            sec_c_l_it->first + delta});
        }
        adjust_curve_loops(changes);
        sec_c_l_map_end = secondary_file.curve_loops_map.end();
        for (   auto sec_c_l_it = secondary_file.curve_loops_map.begin();
                sec_c_l_it != sec_c_l_map_end;
                sec_c_l_it++)
        {
            curve_loops_map.insert({sec_c_l_it->first,sec_c_l_it->second});
        }
    }
    else
    {
        curve_loops_map = secondary_file.curve_loops_map;
    }
    return EXIT_SUCCESS;
}

int GEO::geofile::merge_plane_surfaces(geofile &secondary_file)
{
    if (!plane_surfaces_map.empty())
    {
        int lowest_secondary    = secondary_file.plane_surfaces_map.begin()->first;
        int highest_primary     = std::prev(plane_surfaces_map.end())->first;
        int delta               = highest_primary - lowest_secondary + 1;
        std::map <int,int> changes;
        auto sec_pl_sur_map_end = secondary_file.plane_surfaces_map.end();
        for (   auto sec_pl_sur_it = secondary_file.plane_surfaces_map.begin();
                sec_pl_sur_it != sec_pl_sur_map_end;
                sec_pl_sur_it++)
        {
            changes.insert({ sec_pl_sur_it->first,
                            sec_pl_sur_it->first + delta});
        }
        adjust_plane_surfaces(changes);
        sec_pl_sur_map_end = secondary_file.plane_surfaces_map.end();
        for (   auto sec_pl_sur_it = secondary_file.plane_surfaces_map.begin();
                sec_pl_sur_it != sec_pl_sur_map_end;
                sec_pl_sur_it++)
        {
            plane_surfaces_map.insert({sec_pl_sur_it->first,sec_pl_sur_it->second});
        }
    }
    else
    {
        plane_surfaces_map = secondary_file.plane_surfaces_map;
    }
    return EXIT_SUCCESS;
}

int GEO::geofile::merge_surface_loops(geofile &secondary_file)
{
    if (!surface_loops_map.empty())
    {
        int lowest_secondary    = secondary_file.surface_loops_map.begin()->first;
        int highest_primary     = std::prev(surface_loops_map.end())->first;
        int delta               = highest_primary - lowest_secondary + 1;
        std::map <int,int> changes;
        auto sec_sur_loop_map_end = secondary_file.surface_loops_map.end();
        for (   auto sec_sur_loop_it = secondary_file.surface_loops_map.begin();
                sec_sur_loop_it != sec_sur_loop_map_end;
                sec_sur_loop_it++)
        {
            changes.insert({ sec_sur_loop_it->first,
                            sec_sur_loop_it->first + delta});
        }
        adjust_surface_loops(changes);
        sec_sur_loop_map_end = secondary_file.surface_loops_map.end();
        for (   auto sec_sur_loop_it = secondary_file.surface_loops_map.begin();
                sec_sur_loop_it != sec_sur_loop_map_end;
                sec_sur_loop_it++)
        {
            surface_loops_map.insert({sec_sur_loop_it->first,sec_sur_loop_it->second});
        }
    }
    else
    {
        surface_loops_map = secondary_file.surface_loops_map;
    }
    return EXIT_SUCCESS;
}

int GEO::geofile::merge_volumes(geofile &secondary_file)
{
    if (!volumes_map.empty())
    {
        int lowest_secondary    = secondary_file.volumes_map.begin()->first;
        int highest_primary     = std::prev(volumes_map.end())->first;
        int delta               = highest_primary - lowest_secondary + 1;
        std::map <int,int> changes;
        auto sec_vol_map_end = secondary_file.volumes_map.end();
        for (   auto sec_vol_map_it = secondary_file.volumes_map.begin();
                sec_vol_map_it != sec_vol_map_end;
                sec_vol_map_it++)
        {
            changes.insert({ sec_vol_map_it->first,
                            sec_vol_map_it->first + delta});
        }
        adjust_volumes(changes);
        sec_vol_map_end = secondary_file.volumes_map.end();
        for (   auto sec_vol_map_it = secondary_file.volumes_map.begin();
                sec_vol_map_it != sec_vol_map_end;
                sec_vol_map_it++)
        {
            volumes_map.insert({sec_vol_map_it->first,sec_vol_map_it->second});
        }
    }
    else
    {
        volumes_map = secondary_file.volumes_map;
    }
    return EXIT_SUCCESS;
}

int GEO::geofile::merge_physical_entities(geofile &secondary_file)
{
    if(!secondary_file.physical_points_map.empty())
        merge_physical_points(secondary_file);
    if(!secondary_file.physical_curves_map.empty())
        merge_physical_curves(secondary_file);
    if(!secondary_file.physical_surfaces_map.empty())
        merge_physical_surfaces(secondary_file);
    if(!secondary_file.physical_volumes_map.empty())
        merge_physical_volumes(secondary_file);
    return EXIT_SUCCESS;
}

int GEO::geofile::merge_physical_points(geofile &secondary_file)
{
    if (!physical_points_map.empty())
    {
        if(!secondary_file.physical_points_map.empty())
        {
            for (auto i = secondary_file.physical_points_map.begin();
                 i != secondary_file.physical_points_map.end(); i++)
            {
                auto j = physical_points_map.find(i->first);
                auto j_end = physical_points_map.end();
                // Physical name already exists in first map.
                // Append to element.
                if (j!=j_end)
                {
                    j->second.insert(j->second.end(),i->second.begin(), i->second.end());
                }
                // Physical name not in first map
                // Append physical name to first map.
                else
                {
                    physical_points_map.insert({i->first,i->second});
                }
            }
        }
    }
    else
    {
        if (!secondary_file.physical_points_map.empty())
            physical_points_map = secondary_file.physical_points_map;
    }
    return EXIT_SUCCESS;
}

int GEO::geofile::merge_physical_curves(geofile &secondary_file)
{
    if (!physical_curves_map.empty())
    {
        if(!secondary_file.physical_curves_map.empty())
        {
            for (auto i = secondary_file.physical_curves_map.begin();
                 i != secondary_file.physical_curves_map.end(); i++)
            {
                auto j = physical_curves_map.find(i->first);
                auto j_end = physical_curves_map.end();
                // Physical name already exists in first map.
                // Append to element.
                if (j!=j_end)
                {
                    j->second.insert(j->second.end(),i->second.begin(), i->second.end());
                }
                // Physical name not in first map
                // Append physical name to first map.
                else
                {
                    physical_curves_map.insert({i->first,i->second});
                }
            }
        }
    }
    else
    {
        if (!secondary_file.physical_curves_map.empty())
            physical_curves_map = secondary_file.physical_curves_map;
    }
    return EXIT_SUCCESS;
}

int GEO::geofile::merge_physical_surfaces(geofile &secondary_file)
{
    if (!physical_surfaces_map.empty())
    {
        if(!secondary_file.physical_surfaces_map.empty())
        {
            for (auto i = secondary_file.physical_surfaces_map.begin();
                 i != secondary_file.physical_surfaces_map.end(); i++)
            {
                auto j = physical_surfaces_map.find(i->first);
                auto j_end = physical_surfaces_map.end();
                // Physical name already exists in first map.
                // Append to element.
                if (j!=j_end)
                {
                    j->second.insert(j->second.end(),i->second.begin(), i->second.end());
                }
                // Physical name not in first map
                // Append physical name to first map.
                else
                {
                    physical_surfaces_map.insert({i->first,i->second});
                }
            }
        }
    }
    else
    {
        if (!secondary_file.physical_surfaces_map.empty())
            physical_surfaces_map = secondary_file.physical_surfaces_map;
    }
    return EXIT_SUCCESS;
}

int GEO::geofile::merge_physical_volumes(geofile &secondary_file)
{
    if (!physical_volumes_map.empty())
    {
        if(!secondary_file.physical_volumes_map.empty())
        {
            for (auto i = secondary_file.physical_volumes_map.begin();
                 i != secondary_file.physical_volumes_map.end(); i++)
            {
                auto j = physical_volumes_map.find(i->first);
                auto j_end = physical_volumes_map.end();
                // Physical name already exists in first map.
                // Append to element.
                if (j!=j_end)
                {
                    j->second.insert(j->second.end(),i->second.begin(), i->second.end());
                }
                // Physical name not in first map
                // Append physical name to first map.
                else
                {
                    physical_volumes_map.insert({i->first,i->second});
                }
            }
        }
    }
    else
    {
        if (!secondary_file.physical_volumes_map.empty())
            physical_volumes_map = secondary_file.physical_volumes_map;
    }
    return EXIT_SUCCESS;
}

int GEO::geofile::translate_data(   const double &delta_x,
                                    const double &delta_y,
                                    const double &delta_z )
{
    make_coherent(true);
    simplify_data();
    auto end = points_map.end();
    for (auto i = points_map.begin(); i != end; i++)
    {
        translate_point(i->second, delta_x, delta_y, delta_z);
    }
    return EXIT_SUCCESS;
}

/**
 * [Rotate Data]
 * Rotates all points in the .geo file co-ordinates by reference.
 * @param  o_x      [ The x co-ordinate of the point to rotate around ]
 * @param  o_y      [ The y co-ordinate of the point to rotate around ]
 * @param  o_z      [ The z co-ordinate of the point to rotate around ]
 * @param theta_x   [ Angle in degrees around x-axis of {c}
 * @param theta_y   [ Angle in degrees around y-axis of {c}
 * @param theta_z   [ Angle in degrees around z-axis of {c}
 */
int GEO::geofile::rotate_data( const double &o_x,      const double &o_y,
                                const double &o_z,      const double &theta_x,
                                const double &theta_y,  const double &theta_z)
{
    make_coherent(true);
    simplify_data();
    auto pm_end = points_map.end();
    for (auto i = points_map.begin(); i != pm_end; i++)
    {
        rotate_point(o_x,o_y,o_z,theta_x,theta_y,theta_z,i->second);
    }
    return EXIT_SUCCESS;
}

int GEO::geofile::scale_data(const double factor)
{
    make_coherent(true);
    simplify_data();
    auto pm_end = points_map.end();
    for (auto i = points_map.begin(); i != pm_end; i++)
    {
        scale_point(i->second, factor);
    }
    return EXIT_SUCCESS;
}

void GEO::geofile::scale_point(point &in_point, const double factor)
{
    in_point.x = in_point.x * factor;
    in_point.y = in_point.y * factor;
    in_point.z = in_point.z * factor;
}

void GEO::geofile::translate_point(GEO::point &point,
                                  const double &delta_x,
                                  const double &delta_y,
                                  const double &delta_z )
{
    point.x = point.x + delta_x;
    point.y = point.y + delta_y;
    point.z = point.z + delta_z;
}

/**
 * [Rotate Point]
 * Rotates a specified POINT's co-ordinates by reference.
 * @param  o_x      [ The x co-ordinate of the point to rotate around ]
 * @param  o_y      [ The y co-ordinate of the point to rotate around ]
 * @param  o_z      [ The z co-ordinate of the point to rotate around ]
 * @param theta_x   [ Angle in degrees around x-axis of {c}
 * @param theta_y   [ Angle in degrees around y-axis of {c}
 * @param theta_z   [ Angle in degrees around z-axis of {c}
 * @param subject   [ The point which must be rotated]
 */
void GEO::geofile::rotate_point(const double &o_x,       const double &o_y,
                                const double &o_z,       const double &theta_x,
                                const double &theta_y,   const double &theta_z,
                                point &subject)
{
    point p = subject;
    double t_x = theta_x * 3.141592653589 / 180; // convert angle to radians
    double t_y = theta_y * 3.141592653589 / 180; // convert angle to radians
    double t_z = theta_z * 3.141592653589 / 180; // convert angle to radians

    // translate point back to origin:
    p.x -= o_x;
    p.y -= o_y;
    p.z -= o_z;

    // rotate point
    rotate_X(t_x, p);
    rotate_Y(t_y, p);
    rotate_Z(t_z, p);

    // translate point back:
    p.x = (p.x + o_x);
    p.y = (p.y + o_y);
    p.z = (p.z + o_z);
    subject = p;         // Overwrite the original point with the rotated one.
}

void GEO::geofile::rotate_X(const double &theta, point &in_point)
{
    double sin_theta = std::sin(theta);
    double cos_theta = std::cos(theta);
    double y = in_point.y * cos_theta - in_point.z * sin_theta;
    double z = in_point.z * cos_theta + in_point.y * sin_theta;
    in_point.y = y;
    in_point.z = z;
};

void GEO::geofile::rotate_Y(const double &theta, point &in_point)
{
    double sin_theta = std::sin(theta);
    double cos_theta = std::cos(theta);
    double x = in_point.x * cos_theta + in_point.z * sin_theta;
    double z = in_point.z * cos_theta - in_point.x * sin_theta;
    in_point.x = x;
    in_point.z = z;
};

void GEO::geofile::rotate_Z(const double &theta, point &in_point)
{
    double sin_theta = std::sin(theta);
    double cos_theta = std::cos(theta);
    double x = in_point.x * cos_theta - in_point.y * sin_theta;
    double y = in_point.y * cos_theta + in_point.x * sin_theta;
    in_point.x = x;
    in_point.y = y;
};

// fills a vector of GEO::point with all the points in the geofile
void GEO::geofile::pull_points(std::vector<GEO::point> &in_vector)
{
//std::map<int, point>            points_map;
    if (points_map.size()>0)
    {
        auto pm_end = points_map.end();
        for ( auto pm_it = points_map.begin();
             pm_it != pm_end; pm_it++)
        {
            in_vector.push_back(pm_it->second);
        }
    }
}

int GEO::geofile::insert_points_map(std::map<int, point> &in_points_map)
{
    if(points_map.empty()==true)
    {
        auto pm_end = in_points_map.end();
        for ( auto pm_it = in_points_map.begin(); pm_it != pm_end; pm_it++)
        {
            points_map.insert({pm_it->first, pm_it->second});
        }
        return EXIT_SUCCESS;
    }
    else
    {
        std::cout<<"Error: Cannot insert fresh points_map into existing GEO file."<<std::endl;
        std::cout<<"Rather create new geo file and merge."<<std::endl;
        return EXIT_FAILURE;
    }
}
int GEO::geofile::insert_lines_map(std::map<int, line> &in_lines_map)
{
    if(lines_map.empty()==true)
    {
        auto lm_end = in_lines_map.end();
        for ( auto lm_it = in_lines_map.begin(); lm_it != lm_end; lm_it++)
        {
            lines_map.insert({lm_it->first, lm_it->second});
        }
        return EXIT_SUCCESS;
    }
    else
    {
        std::cout<<"Error: Cannot insert fresh lines_map into existing GEO file."<<std::endl;
        std::cout<<"Rather create new geo file and merge."<<std::endl;
        return EXIT_FAILURE;
    }
}

int GEO::geofile::insert_curve_loops_map(const std::map<int, std::vector<int>> &in_curve_loops_map)
{
    if(curve_loops_map.empty()==true)
    {
        auto clm_end = in_curve_loops_map.end();
        for ( auto clm_it = in_curve_loops_map.begin(); clm_it != clm_end; clm_it++)
        {
            curve_loops_map.insert({clm_it->first, clm_it->second});
        }
        return EXIT_SUCCESS;
    }
    else
    {
        std::cout<<"Error: Cannot insert fresh curves loop map into existing GEO file."<<std::endl;
        std::cout<<"Rather create new geo file and merge."<<std::endl;
        return EXIT_FAILURE;
    }
}

int GEO::geofile::insert_plane_surfaces_map(const std::map<int, int> &in_plane_surfaces_map)
{
    if(plane_surfaces_map.empty()==true)
    {
        auto psm_end = in_plane_surfaces_map.end();
        for ( auto psm_it = in_plane_surfaces_map.begin(); psm_it != psm_end; psm_it++)
        {
            plane_surfaces_map.insert({psm_it->first, psm_it->second});
        }
        return EXIT_SUCCESS;
    }
    else
    {
        std::cout<<"Error: Cannot insert fresh plane_surfaces into existing GEO file."<<std::endl;
        std::cout<<"Rather create new geo file and merge."<<std::endl;
        return EXIT_FAILURE;
    }
}

int GEO::geofile::insert_surface_loops_map(const std::map<int, std::vector<int>> &in_surface_loops_map)
{
    if(surface_loops_map.empty()==true)
    {
        auto slm_end = in_surface_loops_map.end();
        for ( auto slm_it = in_surface_loops_map.begin(); slm_it != slm_end; slm_it++)
        {
            surface_loops_map.insert({slm_it->first, slm_it->second});
        }
        return EXIT_SUCCESS;
    }
    else
    {
        std::cout<<"Error: Cannot insert fresh surface loops into existing GEO file."<<std::endl;
        std::cout<<"Rather create new geo file and merge."<<std::endl;
        return EXIT_FAILURE;
    }
}

int GEO::geofile::insert_volumes_map(const std::map<int, int> &in_volumes_map)
{
    if(volumes_map.empty()==true)
    {
        auto vm_it_end = in_volumes_map.end();
        for ( auto vm_it = in_volumes_map.begin(); vm_it != vm_it_end; vm_it++)
        {
            volumes_map.insert({vm_it->first, vm_it->second});
        }
        return EXIT_SUCCESS;
    }
    else
    {
        std::cout<<"Error: Cannot insert fresh volumes into existing GEO file."<<std::endl;
        std::cout<<"Rather create new geo file and merge."<<std::endl;
        return EXIT_FAILURE;
    }
}

void GEO::geofile::switch_to_OpenCASCADE()
{
    is_open_cascade = true;
}
void GEO::geofile::switch_off_OpenCASCADE()
{
    is_open_cascade = false;
}

void GEO::geofile::disable_char_len()
{
    ignore_char_len = true;
}

void GEO::geofile::enable_char_len()
{
    ignore_char_len = false;
}

/**
 * [Make Coherent]
 * Removes all duplicated entities from the file
 * @param  detailed    [If set to true, searches through all elements and physical elements]
 *                      If set to false, only removes duplicate points, lines and redirects
 *                      curve loops to unique entries.
 */
void GEO::geofile::make_coherent(const bool &detailed)
{
    // start_timer();                                                                      //DEBUG
    coherent_points();
    // std::cout << std::endl;                                                             //DEBUG
    // std::cout << "Time: coherent_points = " << get_microseconds()/1e3 << std::endl;     //DEBUG
    // start_timer();                                                                      //DEBUG
    coherent_lines();
    // std::cout << "Time: coherent_lines = " << get_microseconds()/1e3 << std::endl;      //DEBUG
    // start_timer();                                                                      //DEBUG
    if ( detailed==true)
    {
        std::map<int,int> changes;
        // ------- Search for duplicated curve loops --------
        if (!curve_loops_map.empty())
        {
            auto main_cl_loop_end = std::prev(curve_loops_map.end());
            for (auto main_cl_loop_it = curve_loops_map.begin();
                main_cl_loop_it != main_cl_loop_end;
                main_cl_loop_it++)
            {
                auto inner_cl_loop_end = curve_loops_map.end();
                auto cl_p_holder = main_cl_loop_it; cl_p_holder++;
                for (auto inner_cl_loop_it = cl_p_holder;
                    inner_cl_loop_it != inner_cl_loop_end;
                    inner_cl_loop_it++)
                {
                    int curve_type = compare_vector(main_cl_loop_it->second, inner_cl_loop_it->second);
                    switch (curve_type)
                    {
                    case 1/*identical*/:
                        cl_p_holder = inner_cl_loop_it; cl_p_holder--;
                        changes.insert({inner_cl_loop_it->first,main_cl_loop_it->first});
                        curve_loops_map.erase(inner_cl_loop_it->first);
                        inner_cl_loop_it = cl_p_holder;
                        inner_cl_loop_end = curve_loops_map.end();
                        main_cl_loop_end = std::prev(curve_loops_map.end());
                        break;
                    case 2/*reversed*/:
                        cl_p_holder = inner_cl_loop_it; cl_p_holder--;
                        changes.insert({inner_cl_loop_it->first,-main_cl_loop_it->first});
                        curve_loops_map.erase(inner_cl_loop_it->first);
                        inner_cl_loop_it = cl_p_holder;
                        inner_cl_loop_end = curve_loops_map.end();
                        main_cl_loop_end = std::prev(curve_loops_map.end());
                        break;
                    default:
                        break;
                    }
                }
            }
            adjust_curve_loops(changes);
            changes.clear();
        }
        // ------ Search for duplicated plane surfaces ------
        if (!plane_surfaces_map.empty())
        {
            auto main_ps_loop_end = std::prev(plane_surfaces_map.end());
            for (auto main_ps_loop_it = plane_surfaces_map.begin();
                main_ps_loop_it != main_ps_loop_end;
                main_ps_loop_it++)
            {
                auto inner_ps_loop_end = plane_surfaces_map.end();
                auto ps_p_holder = main_ps_loop_it; ps_p_holder++;
                for (auto inner_ps_loop_it = ps_p_holder;
                    inner_ps_loop_it != inner_ps_loop_end;
                    inner_ps_loop_it++)
                {
                    if (main_ps_loop_it->second == inner_ps_loop_it->second)
                    {
                        ps_p_holder = inner_ps_loop_it; ps_p_holder--;
                        changes.insert({ inner_ps_loop_it->first, main_ps_loop_it->first });
                        plane_surfaces_map.erase(inner_ps_loop_it->first);
                        inner_ps_loop_it = ps_p_holder;
                        inner_ps_loop_end = plane_surfaces_map.end();
                        main_ps_loop_end = std::prev(plane_surfaces_map.end());
                    }
                }
            }
            adjust_plane_surfaces(changes);
            changes.clear();
        }
        // ------ Search for duplicated surface loops -------
        if (!surface_loops_map.empty())
        {
            auto main_sur_loop_end = std::prev(surface_loops_map.end());
            for (auto main_sur_loop_it = surface_loops_map.begin();
                main_sur_loop_it != main_sur_loop_end;
                main_sur_loop_it++)
            {
                auto inner_sur_loop_end = surface_loops_map.end();
                auto sur_p_holder = main_sur_loop_it; sur_p_holder++;
                for (auto inner_sur_loop_it = sur_p_holder;
                    inner_sur_loop_it != inner_sur_loop_end;
                    inner_sur_loop_it++)
                {
                    int curve_type = compare_vector(main_sur_loop_it->second, inner_sur_loop_it->second);
                    switch (curve_type)
                    {
                    case 1/*identical*/:
                        sur_p_holder = inner_sur_loop_it; sur_p_holder--;
                        changes.insert({inner_sur_loop_it->first,main_sur_loop_it->first});
                        curve_loops_map.erase(inner_sur_loop_it->first);
                        inner_sur_loop_it = sur_p_holder;
                        inner_sur_loop_end = surface_loops_map.end();
                        main_sur_loop_end = std::prev(surface_loops_map.end());
                        break;
                    case 2/*reversed*/:
                        sur_p_holder = inner_sur_loop_it; sur_p_holder--;
                        changes.insert({inner_sur_loop_it->first,-main_sur_loop_it->first});
                        curve_loops_map.erase(inner_sur_loop_it->first);
                        inner_sur_loop_it = sur_p_holder;
                        inner_sur_loop_end = surface_loops_map.end();
                        main_sur_loop_end = std::prev(surface_loops_map.end());
                        break;
                    default:
                        break;
                    }
                }
            }
            adjust_surface_loops(changes);
            changes.clear();
        }
        // --- Search for duplicated physical elemements ----
        // Physical Curves
        auto pc_m_end = physical_curves_map.end();
        for ( auto i = physical_curves_map.begin();
            i != pc_m_end;
            i++)
        {
            std::sort(i->second.begin(),i->second.end());
            i->second.erase( unique(i->second.begin(),i->second.end() ), i->second.end());
        }
        // Physical Surfaces
        auto ps_m_end = physical_surfaces_map.end();
        for ( auto i = physical_surfaces_map.begin();
            i != ps_m_end;
            i++)
        {
            std::sort(i->second.begin(),i->second.end());
            i->second.erase( unique(i->second.begin(),i->second.end() ), i->second.end());
        }
        // Physical Volumes
        auto pv_m_end = physical_volumes_map.end();
        for ( auto i = physical_volumes_map.begin();
            i != pv_m_end;
            i++)
        {
            std::sort(i->second.begin(),i->second.end());
            i->second.erase( unique(i->second.begin(),i->second.end() ), i->second.end());
        }
    }
}

// Perform coherence check on geofile points.
// Removes all duplicate points by inverting the points map.
void GEO::geofile::coherent_points()
{
    std::map<point, int> inv_point_map;
    std::map<int, int> point_adjustment_map;
    auto pm_it_end = points_map.end();
    for ( auto pm_it = points_map.begin(); pm_it != pm_it_end; pm_it++)
    {
        auto outcome = inv_point_map.insert({pm_it->second, pm_it->first});
        if (outcome.second==false)
        {
            int failed = pm_it->first;                      // get index that failed
            int reason = inv_point_map.find(pm_it->second)->second;
            point_adjustment_map.insert({failed, reason});  // add to list of duplicates
        }
    }
    update_points(inv_point_map, point_adjustment_map);
}


// If changes are detected:
// Clear the points map and repopulate with unique
// entries, then update dependencies
void GEO::geofile::update_points(const std::map<point, int> &inv_point_map,
                                 const std::map<int, int> &point_adjustment_map)
{
    if(!point_adjustment_map.empty())
    {
        points_map.clear();
        auto inv_map_end = inv_point_map.end();
        for (auto inv_map_it = inv_point_map.begin(); inv_map_it != inv_map_end; inv_map_it++)
        {
            points_map.insert({inv_map_it->second, inv_map_it->first});
        }
        redirect_physical_points(point_adjustment_map);
        redirect_lines(point_adjustment_map);
    }
}

void GEO::geofile::redirect_physical_points(const std::map<int, int> &point_adjustment_map)
{
    // Guard against waste of time
    if ( (!physical_points_map.empty()) && (!point_adjustment_map.empty() ) )
    {
        // Look through all physical point names
        auto phys_p_end = physical_points_map.end();
        for (auto phys_p_it = physical_points_map.begin(); phys_p_it != phys_p_end; phys_p_it++)
        {
            // Look through entire vector of references
            auto p_vec_begin = phys_p_it->second.begin();
            auto p_vec_end = phys_p_it->second.end();
            for ( auto p_vec = p_vec_begin; p_vec != p_vec_end; p_vec++)
            {
                auto j = point_adjustment_map.find(*p_vec);
                if (j != point_adjustment_map.end())
                    *p_vec = j->second;//redirect old reference to new
            }
            // Remove duplicates
            std::sort(p_vec_begin,p_vec_end);
            phys_p_it->second.erase(unique(p_vec_begin,p_vec_end), p_vec_end);
        }
    }
}

// looks through all lines for references to duplicate points.
// adjusts them to point to unique points
void GEO::geofile::redirect_lines( const std::map<int, int> &point_adjustment_map)
{
    if(!point_adjustment_map.empty())
    {
        auto lm_end = lines_map.end();
        for ( auto lm_it = lines_map.begin(); lm_it != lm_end; lm_it++)
        {
            auto result = point_adjustment_map.find(lm_it->second.start);
            if (result != point_adjustment_map.end())
                lm_it->second.start = result->second;
            result = point_adjustment_map.find(lm_it->second.end);
            if (result != point_adjustment_map.end())
                lm_it->second.end = result->second;
        }
    }
}

// Perform coherence check on geofile lines.
// Removes all duplicate points by inverting the points map.
void GEO::geofile::coherent_lines()
{
    std::map<opti_line, int> inv_directional_lines_map;
    std::map<int, int> lines_adjustment_map;
    // populate directional_lines_map from lines_map
    auto lm_end = lines_map.end();
    for ( auto lm_it = lines_map.begin(); lm_it != lm_end; lm_it++)
    {
        opti_line incoming_line;
        incoming_line.low       = lm_it->second.start;
        incoming_line.high      = lm_it->second.end;
        incoming_line.reversed  = false;
        int placeholder;
        if ( incoming_line.low > incoming_line.high )
        {
            placeholder = incoming_line.low;
            incoming_line.low = incoming_line.high;
            incoming_line.high = placeholder;
            incoming_line.reversed = true;
        }
        //try to insert the line.
        auto outcome = inv_directional_lines_map.insert({incoming_line, lm_it->first});
        if (outcome.second==false)
        {
            // high and low is the same. Is the directionality?
            auto existing_it = inv_directional_lines_map.find(incoming_line);
            auto existing_is_reversed = existing_it->first.reversed;
            int sign = (incoming_line.reversed == existing_is_reversed) ?  (1) : (-1) ;
            // insert the duplicate adjustment
            lines_adjustment_map.insert({lm_it->first,
                                         sign*existing_it->second});
        }
    }
    update_lines(lines_adjustment_map);
}

void GEO::geofile::update_lines( const std::map<int, int> &lines_adjustment_map)
{
    if(!lines_adjustment_map.empty())
    {
        // actually delete the duplicates from the main map
        auto lam_end = lines_adjustment_map.end();
        for ( auto lam_it = lines_adjustment_map.begin(); lam_it != lam_end; lam_it++)
        {
            lines_map.erase(lam_it->first);
        }
        // Change the curve loops to point to unique elements
        redirect_curve_loops(lines_adjustment_map);
    }
}

void GEO::geofile::redirect_curve_loops(const std::map<int, int> &lines_adjustment_map)
{
    if (!lines_adjustment_map.empty())
    {
        auto clm_end = curve_loops_map.end();
        for ( auto clm_it = curve_loops_map.begin(); clm_it != clm_end; clm_it++)
        {
            auto cli_end = clm_it->second.end();
            for ( auto cl_it = clm_it->second.begin(); cl_it != cli_end; cl_it++)
            {
                auto result = lines_adjustment_map.find(*cl_it);
                if(result != lines_adjustment_map.end())
                    *cl_it = result->second ;
            }
        }
    }
}

void GEO::geofile::start_timer()
{
 start_time = std::chrono::high_resolution_clock::now();
}

int GEO::geofile::get_microseconds()
{
    std::chrono::_V2::system_clock::time_point now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time);
    return duration.count();
}

void GEO::geofile::enable_bool_diff_rest_from_first()
{
    bool_diff_rest = true;
}

// curve physical curve must point to the correct curve loop
// void GEO::geofile::redirect_physical_curves(const std::map<int, int> &lines_adjustment_map)
// {
//     if((!physical_curves_map.empty())&&(!lines_adjustment_map.empty()))
//     {
//         auto p_c_m_end  = physical_curves_map.end();
//         for (auto p_c_m_it = physical_curves_map.begin(); p_c_m_it != p_c_m_end; p_c_m_it++)
//         {
//             auto curve_end = p_c_m_it->second.end();
//             for(auto curve_it = p_c_m_it->second.begin(); curve_it != curve_end; curve_it++)
//             {
//                 // lines_adjustment_map.find(*curve_it) TODO
//             }
//         }
//     }
// }
