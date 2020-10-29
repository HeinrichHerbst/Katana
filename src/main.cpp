/**
 * Author:      H.F. Herbst
 * Origin:      Stellenbosch University
 * For:         IARPA SuperTools Project
 * license:     MIT
 * Description: Main interface for terminal version of Katana.
 */
#include <iostream>
#include "t_interface.hpp"

int main(int argc, char *argv[])
{
    //create data folder if doesn't exist
    std::filesystem::create_directory("data");
    std::filesystem::create_directory("data/3DGen");
    if (argc==1)
    {
        UI::interactive_mode();
    }
    else
    {
        UI::argument_mode(argc, argv);
    }
    UI::print_ln("Katana Exited.");
    return 0;
}

