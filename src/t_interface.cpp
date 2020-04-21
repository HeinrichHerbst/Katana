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
#include "geofile_operations.hpp"
#include "flooxs_gen.hpp"
#include "ldf_process.hpp"
#include "stitching.hpp"
#include "t_interface.hpp"
#include "meshfile_operations.hpp"
#include "new_meshfile_operations.hpp"

//----------------------------------------------------------
namespace ARG_MODE
{
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
}
void UI::argument_mode(int &argc, char *argv[])
{
    print_welcome();
    ARG_MODE::string_code input_arg = ARG_MODE::hashit(argv[1]);
    switch (input_arg)
    {
    case ARG_MODE::eHelp:
        print_help();
        break;
    case ARG_MODE::eMeshops:
        gather_meshops_arg(argc, argv);
        break;
    case ARG_MODE::eModeling:
        gather_modeling_arg(argc, argv);
        break;
    case ARG_MODE::eSlice:
        gather_slice_arg(argc, argv);
        break;
    case ARG_MODE::eVersion:
        break;
    default:
        print_ln("Unrecognized input argument. Try \"-help\" or interactive mode.");
        break;
    }
}
//----------------------------------------------------------
void UI::interactive_mode()
{
    print_welcome();
    print_ln(" Katana can be quickly run with input arguments or be run interactively.");
    print_ln(" Katana is currently running in interactive mode.");
    skip_ln();
    main_menu();
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
    bool accepted = false;
    while (accepted == false)
    {
        switch (choice)
        {
        case 1:
            gather_slice_interactive();
            accepted = true;
            break;
        case 2:
            gather_modeling_interactive();
            accepted = true;
            break;
        case 3:
            gather_mesh_interactive();
            accepted = true;
            break;
        case 4:
            print_help();
            accepted = true;
            break;
        default:
            clear_screen();
            print_ln("Unrecognized input. Please try again.");
            print_ln(" 1 - Generate FLOOXS input script.");
            print_ln(" 2 - Manipulate model geometry.");
            print_ln(" 3 - Mesh file operations.");
            print_ln(" 4 - Help with Katana.");
            choice = get_choice();
            break;
        }
    }
}

//Quilt ----------------------------------------------------
enum quilt_code
{
    eProblem,
    eMerge_simple,
    eSimple_append,
    eTranslate,
    eRotate
};
quilt_code hash_quilt (std::string const& inString)
{
    if (inString == "-m")   return eMerge_simple;
    if (inString == "-r")   return eRotate;
    if (inString == "-sa")  return eSimple_append;
    if (inString == "-t")   return eTranslate;
    return eProblem;
}
void UI::gather_modeling_arg(int &argc, char *argv[])
{
    if (argc>3)
    {
        quilt_code second_arg = hash_quilt(argv[2]);
        switch (second_arg)
        {
            case eMerge_simple:
                execute_simple_merge(argc, argv);
                break;
            case eRotate:
                execute_rotate(argc,argv);
                break;
            case eSimple_append:
                execute_simple_append(argc, argv);
                break;
            case eTranslate:
                execute_translate(argc, argv);
                break;
            default:
                break;
        }
    }
    else
    {
        print_ln("Error: Invalid argument count for modeling.");
    }
}
//----------------------------------------------------------

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

int UI::get_choice()
{
    char mychar;
    scanf(" %c", &mychar);
    int choice = mychar - '0';
    return choice;
}

void UI::print_help()
{
    print_ln("-- Help --");
    skip_ln();
    print_version();
    print_ln("Documentation can be found in the included README.md");
    print_ln("or at heinrichherbst.github.io/Katana");
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
    print_ln("                  cross-section. Both files are saved in the same");
    print_ln("                  directory as the Katana executable.");
    print_ln("                  The geometry file is named cross_section_output.geo");
    print_ln("                  The FLOOXS input script is called katana_generated.tcl");
    skip_ln();
    print_ln(" \"-modeling\"      Geometrical manipulations module. Extra arguments allow");
    print_ln("                  for direct file processing.");
    skip_ln();
    print_ln(" ^ -m             Merges two specified geometry files in the following manner:");
    print_ln("                    1: Imports both files into memory.");
    print_ln("                    2: Performs coherence check on both file data sets.");
    print_ln("                    3: Simplifies both file data sets.");
    print_ln("                    4: Merges both data sets together.");
    print_ln("                    5: Performs coherence check on the merged data.");
    print_ln("                    6: Simplifies the merged data.");
    print_ln("                    7: Writes merged data to specified output file.");
    skip_ln();
    print_ln("                  Refer to manual for the Katana definition of coherence");
    print_ln("                  and simplification and why they are necessary.");
    skip_ln();
    print_ln("                  Format <Katana> <modeling> <merge command>");
    print_ln("                  <first file> <second file> <specified output>");
    skip_ln();
    print_ln("                  e.g. ./Katana -modeling -m data/left.geo data/right.geo");
    print_ln("                  -data/combined.geo");
    skip_ln();
    print_ln(" ^ -sa            Simple append: Joins two 2D geo-files. Append the second");
    print_ln("                  to the first. Support for points and lines only.");
    skip_ln();
    print_ln("                  Format: ./Katana <modeling> <simple append> <first .geo>");
    print_ln("                  <second.geo> <output.geo> <char. len. override>(optional)");
    print_ln("                  e.g.    ./Katana -modeling -sa left.geo right.geo both.geo");
    print_ln("                  The optional arguments overrides the characteristic length");
    print_ln("                  of all points. See char. length in Gmsh documentation.");
    skip_ln();
    print_ln(" ^ -t             Translate entire .geo file. Also perform coherence");
    print_ln("                  optimization and file simplification.");
    skip_ln();
    print_ln("                  Format <Katana> <modeling> <translate command>");
    print_ln("                  <target file> <delta x> <delta y> <delta z>");
    skip_ln();
    print_ln("                  e.g. ./Katana -modeling -t data/shape.geo ");
    print_ln("                  1000 0 1000");
    skip_ln();
    print_ln(" ^ -r             Rotate entire .geo file. Also perform coherence");
    print_ln("                  optimization and file simplification. Angles in");
    print_ln("                  degrees.");
    skip_ln();
    print_ln("                  Format <Katana> <modeling> <rotate command>");
    print_ln("                  <target file> <origin x> <origin y> <origin z>");
    print_ln("                  <theta x> <theta y> <theta z>");
    skip_ln();
    print_ln("                  e.g. ./Katana -modeling -r data/shape.geo");
    print_ln("                  0 0 0 30 30 30");
    skip_ln();
    print_ln(" \"-meshops\"       Meshfile manipulations module.");
    skip_ln();
    print_ln(" ^ -s {Legacy}    Convert FLOOXS exported 2D msh. to .geo contour.");
    print_ln("                  This function converts 2D mesh surfaces into");
    print_ln("                  physical surfaces with the mesh triangles removed.");
    print_ln("                  This is a legacy function since FLOOXS now supports");
    print_ln("                  Gmsh .geo contour exporting.");
    skip_ln();
    print_ln("                  Format <Katana> <mesh file operations> <Silver Lining Command>");
    print_ln("                  <target mesh> <geo output path> ");
    skip_ln();
    print_ln("                  e.g. ./Katana -meshops -s data/f_out.msh data/convert.geo");
    skip_ln();
    print_ln(" ^ -v             Calculate all Gmsh .msh \"Physical Volume\" values in mesh file.");
    skip_ln();
    print_ln("                  Format <Katana> <mesh file operations> <Volume Command>");
    skip_ln();
    print_ln("                  e.g. ./Katana -meshops -v data/example.msh");
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

void UI::clear_screen()
{
    for (auto i = 0; i < 15; i++)
    {
        skip_ln();
    }
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

void UI::gather_modeling_interactive()
{
    print_ln("What would you like to do?");
    print_ln("Please enter a number followed by return:");
    print_ln(" 1 - Append one 2D geofile to another.");
    print_ln(" 2 - Merge two geo files.");
    print_ln(" 3 - Translate entire geo file.");
    print_ln(" 4 - Rotate entire geo file.");
    bool answered = false;
    while (answered == false)
    {
        int choice = get_choice();
        switch (choice)
        {
        case 1:
            gather_append_interactive();
            answered = true;
            break;
        case 2:
            gather_merge_interactive();
            answered = true;
            break;
        case 3:
            gather_translate_interactive();
            answered = true;
            break;
        case 4:
            gather_rotate_interactive();
            answered = true;
            break;
        default:
            clear_screen();
            print_ln("Unrecognized input");
            break;
        }
    }
}

void UI::gather_mesh_interactive()
{
    print_ln("What would you like to do?");
    print_ln("Please enter a number followed by return:");
    print_ln(" 1 - Convert FLOOXS 2D mesh output into .geo contour.");
    print_ln(" 2 - Compute volumes of .msh \"Physical Volumes\".");
    bool answered = false;
    while (answered==false)
    {
        int choice = get_choice();
        switch (choice)
        {
        case 1:
            gather_silver_linings_interactive();
            answered = true;
            break;
        case 2:
            gather_volume_calc_interactive();
            answered = true;
            break;
        default:
            clear_screen();
            print_ln("Unrecognized input");
            break;
        }
    }
}

void UI::gather_volume_calc_interactive()
{
    std::string msh_in;
    print_ln("Please enter path to .msh file:");
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
    getline(std::cin, msh_in);
    print_ln("Using \"" + msh_in + "\" as mesh file path.");
    execute_volume_calculator(msh_in);
}

void UI::gather_rotate_interactive()
{
    std::string geo_in, geo_out, ori_x, ori_y, ori_z, theta_x, theta_y, theta_z;
    print_ln("Please enter path to .geo file:");
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
    getline(std::cin, geo_in);
    print_ln("Using \"" + geo_in + "\" as mesh file path.");
    skip_ln();
    print_ln("Please enter output path:");
    print_ln("(e.g. data/output.geo)");
    getline(std::cin, geo_out);
    std::cout<<"Using \"" << geo_out << "\" as output file path."<< std::endl;
    print_ln("Please enter X co-ordintate of origin of rotation:");
    getline(std::cin, ori_x);
    print_ln("Please enter Y co-ordintate of origin of rotation:");
    getline(std::cin, ori_y);
    print_ln("Please enter Z co-ordintate of origin of rotation:");
    getline(std::cin, ori_z);
    print_ln("Enter degrees to rotate about X-axis:");
    getline(std::cin, theta_x);
    print_ln("Enter degrees to rotate about Y-axis:");
    getline(std::cin, theta_y);
    print_ln("Enter degrees to rotate about Z-axis:");
    getline(std::cin, theta_z);
    if ( GEO::is_e_notation(ori_x)&&
         GEO::is_e_notation(ori_y)&&
         GEO::is_e_notation(ori_z)&&
         GEO::is_e_notation(theta_x)&&
         GEO::is_e_notation(theta_y)&&
         GEO::is_e_notation(theta_z) )
    {
        double d_ox, d_oy, d_oz, d_tx, d_ty, d_tz;
        d_ox = std::stod(ori_x);
        d_oy = std::stod(ori_y);
        d_oz = std::stod(ori_z);
        d_tx = std::stod(theta_x);
        d_ty = std::stod(theta_y);
        d_tz = std::stod(theta_z);
        GEO::geofile input_geo;
        if (input_geo.import_geofile(geo_in)==EXIT_SUCCESS)
        {
            input_geo.make_coherent();
            input_geo.simplify_data();
            if (input_geo.rotate_data(d_ox, d_oy, d_oz, d_tx, d_ty, d_tz)==EXIT_SUCCESS)
            {
                print_ln("Rotate succeeded.");
                if (input_geo.export_geofile(geo_out)==EXIT_SUCCESS)
                    print_ln("Export succeeded.");
                else
                    print_ln("Error: Export failed.");
            }
            else
                print_ln("Error: Rotate failed.");
        }
        else
            print_ln("Error: Import failed.");
    }
    else
        print_ln("Error: Invalid rotation input.");
}

void UI::gather_silver_linings_interactive()
{
    std::string mesh_in, geo_out;
    print_ln("Please enter path to FLOOX exported .msh file:");
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

void UI::gather_merge_interactive()
{
    std::string first_geo, second_geo, output_geo;
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
    GEO::geofile primary_geofile, secondary_geofile;
        if ( primary_geofile.import_geofile(first_geo) == EXIT_SUCCESS )
        {
            print_ln("Imported primary geometry file successfully.");
            if(secondary_geofile.import_geofile(second_geo) == EXIT_SUCCESS)
            {
                print_ln("Imported secondary geometry file successfully.");
                if (primary_geofile.merge_with(secondary_geofile)==EXIT_SUCCESS)
                {
                    std::string export_path = output_geo;
                    if(primary_geofile.export_geofile(export_path)==EXIT_SUCCESS)
                        print_ln("Export returned success.");
                    else
                        print_ln("Export returned failure.");
                }
                else
                {
                    print_ln("Error: Merge Operation Failed.");
                }
            }
            else
            {
                print_ln("Error: Import of second file failed.");
            }
        }
        else
        {
            print_ln("Error: Import of first file failed.");
        }
}

void UI::gather_translate_interactive()
{
    std::string first_geo, output_geo, del_x, del_y, del_z;
    print_ln("Please enter path to .geo file:");
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
    getline(std::cin, first_geo);
    std::cout<<"Using \"" << first_geo << "\" as .geo file path."<< std::endl;
    skip_ln();
    print_ln("Please enter export .geo path:");
    getline(std::cin, output_geo);
    std::cout<<"Using \"" << output_geo << "\" as export file path."<< std::endl;
    print_ln("Please enter x-offset");
    getline(std::cin, del_x);
    print_ln("Please enter y-offset");
    getline(std::cin, del_y);
    print_ln("Please enter z-offset");
    getline(std::cin, del_z);
    print_ln("Using \"" + del_x + "\" as x offset.");
    print_ln("Using \"" + del_y + "\" as y offset.");
    print_ln("Using \"" + del_z + "\" as z offset.");
    if( ( GEO::is_e_notation(del_x) )&&
        ( GEO::is_e_notation(del_y) )&&
        ( GEO::is_e_notation(del_y) ) )
    {
        GEO::geofile input;
        if (input.import_geofile(first_geo)==EXIT_SUCCESS)
        {
            print_ln("Import succeeded.");
            double delta_x = std::stod(del_x);
            double delta_y = std::stod(del_y);
            double delta_z = std::stod(del_z);
            if (input.translate_data(delta_x, delta_y, delta_z)==EXIT_SUCCESS)
            {
                if(input.export_geofile(output_geo)==EXIT_SUCCESS)
                    print_ln("Export succeeded.");
                else
                    print_ln("Error: Export failed.");
            }
            else
            {
                print_ln("Error: Translation failed.");
            }
        }
        else
        {
            print_ln("Error: Import failed.");
        }
    }
    else
    {
        print_ln("Error: Incorrect number format.");
    }
}

void UI::execute_simple_append_interactive( std::string first_geo,
                                            std::string second_geo,
                                            std::string output_geo,
                                            std::string char_len)
{
    simple_append(first_geo, second_geo, output_geo, char_len);
}

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

void UI::execute_simple_merge(int &argc, char *argv[])
{
    if ( argc == 6 )
    {
        GEO::geofile primary_geofile, secondary_geofile;
        if ( primary_geofile.import_geofile(argv[3]) == EXIT_SUCCESS )
        {
            print_ln("Imported primary geometry file successfully.");
            if(secondary_geofile.import_geofile(argv[4]) == EXIT_SUCCESS)
            {
                print_ln("Imported secondary geometry file successfully.");
                if (primary_geofile.merge_with(secondary_geofile)==EXIT_SUCCESS)
                {
                    std::string export_path = argv[5];
                    if(primary_geofile.export_geofile(export_path)==EXIT_SUCCESS)
                        print_ln("Export returned success.");
                    else
                        print_ln("Export returned failure.");
                }
                else
                {
                    print_ln("Error: Merge Operation Failed.");
                }
            }
            else
            {
                print_ln("Error: Import of second file failed.");
            }
        }
        else
        {
            print_ln("Error: Import of first file failed.");
        }
    }
    else
    {
        std::cout << "Error: Expected 5 arguments, got " << argc << "." << std::endl;
    }

}

//Silver linings -----------------------------------
// -s for silver linings
// -v for volume calculation
// 0        1       2   3
// katana -meshops -v data/cube_house_v2.msh
void UI::gather_meshops_arg(int &argc, char *argv[])
{
    if (argc > 3)
    {
        std::string arg_check = argv[2];
        if (arg_check=="-s")
        {
            if (argc>4)
            {
                std::string in_msh = argv[3], out_geo = argv[4];
                execute_silver_linings(in_msh,  out_geo);
            }
            else
            {
                print_ln("Error: Incorrect argument count.");
            }
        }
        else if(arg_check=="-v")
        {
            std::string in_msh = argv[3];
            execute_volume_calculator(in_msh);
        }
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
    std::string geo_outpath = "cross_section_output.geo";
    std::string tcl_outpath = "katana_generated.tcl";
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

// ./Katana -modeling -t data/input.geo data/output.geo 1 1 1
void UI::execute_translate(int &argc, char *argv[])
{
    if (argc!=8)
    {
        print_ln("Error: incorrect argument count.");
    }
    else
    {
        std::string input_path   = argv[3];
        std::string outfile_path = argv[4];
        GEO::geofile primary_geofile;
        if ((GEO::is_e_notation(argv[5]))&&
            (GEO::is_e_notation(argv[6]))&&
            (GEO::is_e_notation(argv[7])))
        {
            double del_x = std::stod(argv[5]);
            double del_y = std::stod(argv[6]);
            double del_z = std::stod(argv[7]);
            if(primary_geofile.import_geofile(input_path)==EXIT_SUCCESS)
            {
                primary_geofile.make_coherent();
                primary_geofile.simplify_data();
                if(primary_geofile.translate_data(del_x, del_y, del_z)==EXIT_SUCCESS)
                {
                    print_ln("Translation completed.");
                    if(primary_geofile.export_geofile(outfile_path)==EXIT_SUCCESS)
                        print_ln(("File exported as "+outfile_path));
                    else
                        print_ln("Error: Export failed.");
                }
                else
                    print_ln("Error: Translation failed.");
            }
            else
                print_ln("Import of geofile failed.");
        }
        else
            print_ln("Error: Shifting parameters unrecognized. Terminating.");
    }
}

//      ./Katana -modeling -r in.geo out.geo 0 0 0 10 10 10
//      1        2          3 4      5       6 7 8 9  10 11
//argv  0        1          2 3      4       5 6 7 8  9  10
void UI::execute_rotate(int &argc, char *argv[])
{
    if (argc!=11)
    {
        print_ln("Error: incorrect argument count.");
    }
    else
    {
        std::string input_path   = argv[3];
        std::string outfile_path = argv[4];
        GEO::geofile primary_geofile;
        if ((GEO::is_e_notation(argv[5]))&&
            (GEO::is_e_notation(argv[6]))&&
            (GEO::is_e_notation(argv[7]))&&
            (GEO::is_e_notation(argv[8]))&&
            (GEO::is_e_notation(argv[9]))&&
            (GEO::is_e_notation(argv[10])))
        {
            double ori_x = std::stod(argv[5]);
            double ori_y = std::stod(argv[6]);
            double ori_z = std::stod(argv[7]);
            double theta_x = std::stod(argv[8]);
            double theta_y = std::stod(argv[9]);
            double theta_z = std::stod(argv[10]);
            if(primary_geofile.import_geofile(input_path)==EXIT_SUCCESS)
            {
                if(primary_geofile.rotate_data( ori_x,   ori_y,   ori_z,
                                                theta_x, theta_y, theta_z)==EXIT_SUCCESS)
                {
                    print_ln("Rotation completed.");
                    if(primary_geofile.export_geofile(outfile_path)==EXIT_SUCCESS)
                        print_ln(("File exported as "+outfile_path));
                    else
                        print_ln("Error: Export failed.");
                }
                else
                    print_ln("Error: Translation failed.");
            }
            else
                print_ln("Import of geofile failed.");
        }
        else
            print_ln("Error: Shifting parameters unrecognized. Terminating.");
    }
}

void UI::execute_volume_calculator(const std::string &mesh_in)
{
    MSH::mesh_file in_mesh;
    if (in_mesh.import_meshfile(mesh_in)) print_ln("Error: Mesh Import failed.");
    in_mesh.print_volumes();
    print_ln("Volume calculation ended.");
}