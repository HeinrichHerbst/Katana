# Documentation Under Construction
Katana is currently pre-release. Open-source code under the MIT License will be accessible by July 2020. Commits will be published to the github page:
[Katana Github](https://github.com/HeinrichHerbst/Katana)

# Overview
Katana is an open-source process-modeling assistant & sectioning program. Katana allows a designer to peer into GDS files and generate two-dimensional Integrated Circuit (IC) cross-sections with the assistance of layer definition information. Additionally, Katana is capable of automatically generating a [FLOOXS](flooxs.ece.ufl.edu/) process-modeling input script. There are four main modules of Katana.
<img src="images/Overview.png" alt="Katana Overview" class="inline"/>

# SilverLinings
Silverlinings is a module of Katana which takes a two-dimensional FLOOXS mesh file and identifies the boundaries of each material segment. Silverlinings uses this information to generate a .geo file which can be edited in Gmsh and used to modify and re-mesh the FLOOXS output. The module is broken into three cpp files: main.cpp, meshfile operations.cpp and boundary_identification.cpp.

# Main file
The main.cpp file holds the code which allows the user to enter the input and output files. The program by default searches for the input file in the root directory. Once the input file has been succesfully identified, SilverLinings begins to process the meshfile.

# Meshfile Operations
The meshfile_operations.cpp file... TBC
