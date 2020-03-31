/**
 * Author:      H.F. Herbst
 * Origin:      Stellenbosch University
 * For:         IARPA SuperTools Project
 * license:     MIT
 * Description: Function definitions for user interface of Katana terminal edition
 */
#include <iostream>
#include <ctime>
#include "drawing.hpp"
#include "gdsCpp.hpp"
#include "flooxs_gen.hpp"
#include "ldf_process.hpp"
#include "stitching.hpp"
#include "t_interface.hpp"
#include "meshfile_operations.hpp"

void UI::print_welcome()
{
    print_ln("# ------------------------------------------------------------------------ #");
    print_ln("#                        _                                                 #");
    print_ln("#              _________| |___________________________________             #");
    print_ln("#             [_x_x_x_x_   ______________Katana______________/             #");
    print_ln("#                       |_|                                                #");
    print_ln("#                                                                          #");
    print_ln("#                          Katana Terminal Edition                         #");
    print_ln("#        TCAD Tools for the Superconducting Electronics Community          #");
    print_ln("#                                                                          #");
    print_ln("# FLOOXS is distrubuted and developed separately by University of Florida. #");
    print_ln("#                          www.flooxs.ece.ufl.edu                          #");
    print_ln("#                                                                          #");
    print_ln("#       FLOOXS and Katana development are funded by IARPA SuperTools       #");
    print_ln("# ------------------------------------------------------------------------ #");
    skip_ln();
}
void UI::print_version()
{
    std::cout << "Katana is currently at version " << (CURRENT_VERSION) << "T." << std::endl;
}

void UI::main_menu()
{
    print_ln(" Please enter a number followed by return:");
    skip_ln();
    print_ln(" 1 - Generate FLOOXS input script.");
    print_ln(" 2 - Manipulate model geometry.");
    print_ln(" 3 - Mesh file operations.");
    print_ln(" 4 - Help with Katana.");
    int choice = get_choice();
    switch (choice)
    {
    case 1:
        gather_slice_interactive();
        break;
    case 2:
        gather_modeling_interactive();
        break;
    case 3:
        gather_mesh_interactive();
        break;
    case 4:
        print_help();
        break;
    default:
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        print_ln("Unrecognized input. Please try again.");
        main_menu();
        break;
    }
}

int UI::get_choice()
{
    char mychar;
    scanf(" %c", &mychar);
    int choice = mychar - '0';
    return choice;
}

void UI::interactive_mode()
{
    print_welcome();
    print_ln(" Katana can be quickly run with input arguments or be run interactively.");
    print_ln(" Katana is currently running in interactive mode.");
    skip_ln();
    main_menu();
}

//----------------------------------------------------------
enum string_code {
    eDefault,
    eHelp,
    eMeshops,
    eModeling,
    eSlice,
    eVersion
};
string_code hashit (std::string const& inString) {
    if ((inString == "-Help")||(inString == "-help"))   return eHelp;
    if ((inString == "-Meshops")||(inString == "-meshops")) return eMeshops;
    if ((inString == "-Slice")||(inString == "-slice")) return eSlice;
    if ((inString == "-Modeling")||(inString == "-modeling")) return eModeling;
    if (    (inString == "-Version")
        ||  (inString == "-version")
        ||  (inString == "--version")
        ||  (inString == "-v") ) return eVersion;
    return eDefault;
}
void UI::argument_mode(int &argc, char *argv[])
{
    print_welcome();
    string_code input_arg = hashit(argv[1]);
    switch (input_arg)
    {
    case eHelp:
        print_help();
        break;
    case eMeshops:
        gather_meshops_arg(argc, argv);
        break;
    case eModeling:
        gather_modeling_arg(argc, argv);
        break;
    case eSlice:
        gather_slice_arg(argc, argv);
        break;
    case eVersion:
        break;
    default:
        print_ln("Unrecognized input argument. Try \"-help\" or interactive mode.");
        break;
    }
}
//----------------------------------------------------------
void UI::print_help()
{
    print_ln("-- Help --");
    skip_ln();
    print_version();
    print_ln("Documentation can be found in the included README.md");
    print_ln("or at heinrichherbst.github.io/Katana");
    skip_ln();
    print_ln("Katana is still under development. Some features are still To Be Implemented");
    print_ln("[TBI]. These will be available by the final release (ETA 31 May 2020).");
    skip_ln();
    print_ln("Katana can be executed with input arguments run interactively.");
    print_ln("If Katana is run with no input arguments, interactive mode is started.");
    skip_ln();
    print_ln("The following input arguments may be used:");
    skip_ln();
    print_ln(" \"-help\":         Print this information");
    skip_ln();
    print_ln(" \"-version\":      Print the current version of Katana");
    skip_ln();
    print_ln(" \"-slice\":        Generate a 2D cross-sectional slice through the IC.");
    print_ln("                  The GDSII format layout file, as well as a layer");
    print_ln("                  definition file are required as input parameters.");
    print_ln("                  Katana also requires the x,y co-ordinate pair of");
    print_ln("                  where to create the slice.");
    skip_ln();
    print_ln("                  Format:");
    print_ln("                  <Katana> <slice> <gds path> <ldf path> <x1> <y1> <x2> <y2>");
    print_ln("                  ./Katana -slice jj.gds mitllsfq5ee.ldf 500 0 500 1000");
    skip_ln();
    print_ln("                  Two files are produced; a geometry file and .tcl file.");
    print_ln("                  The TCL file should be run by FLOOXS to generate a");
    print_ln("                  cross-section.");
    skip_ln();
    print_ln(" \"-modeling\"      Geometrical manipulations module. Extra arguments allow");
    print_ln("                  for direct file processing.");
    skip_ln();
    print_ln(" ^ -sa                Simple append: Joins two 2D geo-files. Append the second");
    print_ln("                      to the first. Support for points and lines only.");
    print_ln("                      Format: ./Katana <modeling> <simple append> <first .geo>");
    print_ln("                       <second.geo> <output.geo> <char. len. override>(optional)");
    print_ln("                      e.g.    ./Katana -modeling -sa left.geo right.geo both.geo");
    print_ln("                      The optional arguments overrides the characteristic length");
    print_ln("                      of all points. See char. length in Gmsh documentation.");
    skip_ln();
    print_ln(" ^ -m                 Merge two geo files.                [TBI]");
    skip_ln();
    print_ln(" ^ -t                 Translate entire file.              [TBI]");
    skip_ln();
    print_ln(" ^ -r                 Rotate entire file.                 [TBI]");
    skip_ln();
    print_ln(" \"-meshops\"       Meshfile manipulations module.      [TBI]");
    skip_ln();
    print_ln(" ^ -rm                Remove mesh and preserve contours.  [TBI]");
    skip_ln();
    print_ln("If you have run into a bug or issue, please log it at");
    print_ln("github.com/HeinrichHerbst/Katana/issues");
    skip_ln();
}
void UI::print_ln(std::string line)
{
    std::cout << line << std::endl;
}
void UI::skip_ln()
{
    std::cout << std::endl;
}

// Gather the slice instructions from an argument.
void UI::gather_slice_arg(int &argc, char *argv[])
{
    if( (isinteger(argv[4]))
     && (isinteger(argv[5]))
     && (isinteger(argv[6]))
     && (isinteger(argv[7])) )
    {
        int x1 = std::stoi(argv[4]);
        int y1 = std::stoi(argv[5]);
        int x2 = std::stoi(argv[6]);
        int y2 = std::stoi(argv[7]);
        execute_slice(argv[2],argv[3],x1,y1,x2,y2);
    }
    else
    {
        print_ln("Invalid co-ordinates detected. Aborting.");
    }
}

//Quilt ----------------------------------------------------
enum quilt_code
{
    eProblem,
    eMerge_additional,
    eMerge_simple,
    eSimple_append,
    eTranslate,
    eRotate
};
quilt_code hash_quilt (std::string const& inString)
{
    if (inString == "-m")   return eMerge_simple;
    if (inString == "-ma")  return eMerge_additional;
    if (inString == "-r")   return eRotate;
    if (inString == "-sa")  return eSimple_append;
    if (inString == "-t")   return eTranslate;
    return eProblem;
}

void UI::gather_modeling_interactive()
{
    print_ln("What would you like to do?");
    print_ln("Please enter a number followed by return:");
    print_ln(" 1 - Append one 2D geofile to another.");
    // print_ln(" 2 - Merge two geo files.");
    // print_ln(" 3 - Translate entire geo file");
    // print_ln(" 4 - Rotate entire geo file");
    int choice = get_choice();
    switch (choice)
    {
    case 1:
        gather_append_interactive();
        break;
    default:
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        print_ln("Unrecognized input");
        gather_modeling_interactive();
        break;
    }
}

void UI::gather_mesh_interactive()
{
    print_ln("What would you like to do?");
    print_ln("Please enter a number followed by return:");
    print_ln(" 1 - Convert Meshfile into .geo file.");
    int choice = get_choice();
    switch (choice)
    {
    case 1:
        gather_silver_linings_interactive();
        break;
    default:
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        skip_ln();
        print_ln("Unrecognized input");
        gather_mesh_interactive();
        break;
    }
}
void UI::gather_silver_linings_interactive()
{
    std::string mesh_in, geo_out;
    print_ln("Please enter path to .msh file:");
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
    getline(std::cin, mesh_in);
    std::cout<<"Using \"" << mesh_in << "\" as mesh file path."<< std::endl;
    skip_ln();
    print_ln("Please enter output path name:");
    print_ln("(e.g. data/output.geo)");
    getline(std::cin, geo_out);
    std::cout<<"Using \"" << geo_out << "\" as output file path."<< std::endl;
    execute_silver_linings(mesh_in,geo_out);
}

void UI::gather_append_interactive()
{
    std::string first_geo, second_geo, output_geo, char_len;
    print_ln("Please enter path to first .geo file:");
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
    getline(std::cin, first_geo);
    std::cout<<"Using \"" << first_geo << "\" as first .geo file path."<< std::endl;
    skip_ln();
    print_ln("Please enter path to second .geo file:");
    getline(std::cin, second_geo);
    std::cout<<"Using \"" << second_geo << "\" as second .geo file path."<< std::endl;
    skip_ln();
    print_ln("Please enter path name to output .geo file:");
    getline(std::cin, output_geo);
    std::cout<<"Using \"" << second_geo << "\" as output .geo file path."<< std::endl;
    skip_ln();
    print_ln("Please enter characteristic length override:");
    print_ln("(Press Return/Enter if you do not wish to override.)");
    getline(std::cin, char_len);
    int argc=6;
    if (char_len!="")
    {
        std::cout<<"Using \"" << char_len << "\" as characteristic length."<< std::endl;
        argc++;
    }
    execute_simple_append_interactive(first_geo,second_geo,output_geo,char_len);
}

void UI::execute_simple_append_interactive( std::string first_geo,
                                            std::string second_geo,
                                            std::string output_geo,
                                            std::string char_len)
{
    simple_append(first_geo, second_geo, output_geo, char_len);
}

void UI::gather_modeling_arg(int &argc, char *argv[])
{
    quilt_code second_arg = hash_quilt(argv[2]);
    switch (second_arg)
    {
    case eMerge_simple:
        print_ln("Simple merging is still [TBI]");
        break;
    case eMerge_additional:
        print_ln("Merge append is still [TBI]");
        break;
    case eRotate:
        print_ln("Geofile rotation is still [TBI]");
        break;
    case eSimple_append:
        execute_simple_append(argc, argv);
        break;
    case eTranslate:
        print_ln("Geofile translation is still [TBI]");
        break;
    default:
        break;
    }
}
//----------------------------------------------------------

void UI::execute_simple_append(int &argc, char *argv[])
{
    if ((argc!=6)&&(argc!=7))
    {
        print_ln("Error: incorrect argument count.");
    }
    else
    {
        std::string file_1_path  = argv[3];
        std::string file_2_path  = argv[4];
        std::string outfile_path = argv[5];
        std::string charac_len_override = "";
        if (argc==7)
        {
           charac_len_override = argv[6];
        }
        simple_append(file_1_path, file_2_path, outfile_path, charac_len_override);
    }
}

//Silver linings -----------------------------------
void UI::gather_meshops_arg(int &argc, char *argv[])
{
    if (argc == 4)
    {
        std::string in_msh = argv[2], out_geo = argv[3];
        execute_silver_linings(in_msh,  out_geo);
    }
    else
    {
        print_ln("Error: Incorrect argument count.");
    }
}

void UI::execute_silver_linings(std::string mesh_in, std::string geo_out)
{
    clock_t execution_time;
    execution_time = clock();
    print_ln("See logfile.txt for conversion information");
    print_ln("=================================== START ==================================");
    print_ln("-------------------------------- CONVERTING... -----------------------------");
    int outcome = begin_processing(mesh_in, geo_out);
    if (outcome!=0)
    {
        print_ln("Error: Conversion failed. See logfile.txt for why.");
    }
    else
    {
        execution_time = clock() - execution_time;
        std::cout << "Program execution took "
        << (float)execution_time / CLOCKS_PER_SEC
        << " seconds" << std::endl;
        print_ln("============================== CONVERSION END ==============================");
    }
}

// Gather the slice instructions interactively.
void UI::gather_slice_interactive()
{
    std::string gds_location, ldf_location, placeholder;
    std::vector<int> co_ords;
    int points_gathered = 0;
    print_ln("Please enter path to GDS file:");
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
    getline(std::cin, gds_location);
    std::cout<<"Using \"" << gds_location << "\" as GDS path."<< std::endl;
    skip_ln();
    print_ln("Please enter path to LDF file:");
    getline(std::cin, ldf_location);
    std::cout<<"Using \"" << ldf_location << "\" as LDF path."<< std::endl;
    skip_ln();
    print_ln("Co-ordinates must match database unit of GDS file; integers only.");
    process_co_ords(co_ords, points_gathered);
    execute_slice(gds_location, ldf_location, co_ords[0], co_ords[1], co_ords[2], co_ords[3]);
}

void UI::process_co_ords(std::vector<int> &co_ords, int &points_gathered)
{
    std::string placeholder;
    switch (points_gathered)
    {
    case 0:
        print_ln("Please enter X1 co_ordinate:");
        break;
    case 1:
        print_ln("Please enter Y1 co_ordinate:");
        break;
    case 2:
        print_ln("Please enter X2 co_ordinate:");
        break;
    default:
        print_ln("Please enter Y2 co_ordinate:");
        break;
    }
    getline(std::cin, placeholder);
    if (isinteger(placeholder))
    {
        co_ords.push_back(std::stoi(placeholder));
        if (points_gathered < 3)
        {
            points_gathered++;
            process_co_ords(co_ords, points_gathered);
        }
    }
    else
    {
        print_ln("The last value entered is not an integer.");
        process_co_ords(co_ords, points_gathered);
    }
}

bool UI::isinteger(std::string const& n) noexcept
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

void UI::execute_slice(std::string gds_path, std::string ldf_path, int x1, int y1, int x2, int y2)
{
    std::string geo_outpath = "data/cross_section_output.geo";
    std::string tcl_outpath = "data/katana_generated.tcl";
    int grid_spacing = 50;
    gdscpp gds_file;
    ldf ldf_data;
    section_data my_section;

    gds_file.import(gds_path);
    ldf_data.read_ldf(ldf_path);
    if (my_section.populate(x1, y1, x2, y2, gds_file) == EXIT_SUCCESS)
    {
      // my_section.polygon_to_gds(test_poly_outpath);commentout
      double gds_db_unit = gds_file.get_database_units();
      double db_unit_in_m = gds_file.get_database_units_in_m();
      draw_geofile(my_section, ldf_data, geo_outpath, gds_db_unit);
      draw_flooxs_file( my_section, ldf_data, tcl_outpath,
                        gds_db_unit,
                        db_unit_in_m, grid_spacing);
    }
}

