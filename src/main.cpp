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
    if (argc==1)
    {
        UI::interactive_mode();
    }
    else
    {
        UI::argument_mode(argc, argv);
    }

    return 0;
}

