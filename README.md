# Documentation Under Construction
Last updated on 31 March, 2020.
Katana is currently at version Alpha (0.8T). Commits will be published to the github page:
[Katana Github](https://github.com/HeinrichHerbst/Katana)

# Overview
Katana is an open-source process-modeling assistant & sectioning program. Katana allows a designer to peer into GDS files and generate two-dimensional Integrated Circuit (IC) cross-sections with the assistance of layer definition information. Additionally, Katana is capable of automatically generating a [FLOOXS](flooxs.ece.ufl.edu/) process-modeling input script. Katana can also combine multiple two-dimensional Gmsh .geo files and convert .msh files back into .geo files (mesh removal).

[![Katana Demonstration](https://img.youtube.com/vi/CsfjJpd8BOQ/0.jpg)](https://www.youtube.com/watch?v=CsfjJpd8BOQ)

# Usage
Katana can be executed with input arguments run interactively.
If Katana is run with no input arguments, interactive mode is started.
The following input arguments may be used:
```
    -help
        Print the help information.

    -version
        Print the current version of Katana.

    -slice
        Generate a 2D cross-sectional slice through the IC.
        The GDSII format layout file, as well as a layer
        definition file are required as input parameters.
        Katana also requires the x,y co-ordinate pair of
        where to create the slice.

        Format: <Katana> <slice> <gds path> <ldf path> <x1> <y1> <x2> <y2>

        e.g:    ./Katana -slice jj.gds mitllsfq5ee.ldf 500 0 500 1000

        Two files are produced; a geometry file and .tcl file.
        The TCL file should be run by FLOOXS to generate a
        cross-section.

    -modeling -sa
        Simple append: Joins two 2D geo-files. Append the second
        to the first. Support for points and lines only.

        Format: ./Katana <modeling> <simple append> <first .geo>
        <second.geo> <output.geo> <char. len. override>(optional)

        e.g.    ./Katana -modeling -sa left.geo right.geo both.geo
        The optional arguments overrides the characteristic length
        of all points. See char. length in Gmsh documentation.

    -meshops
        Meshfile operations. Currently, the only meshfile operation
        is conversion of a Gmsh version 2 .msh file back into a .geo file.
        Replaces all mesh triangles with a physical surface. Maintains
        boundaries and contours.

        Format: ./Katana <mesh file operations>
        <mesh file path> <geo output path>

        e.g. ./Katana -meshops data/flooxs_out.msh data/flooxs_out.geo
```


# Dependencies
Katana makes use of [Boost](https://www.boost.org/doc/libs/1_66_0/more/getting_started/unix-variants.html) functions and therefore requires the library present before building.

Katana is built in C++ with [CMake](https://cmake.org/install/) and utilizes [the clipper library](http://www.angusj.com/delphi/clipper.phpBoost)  for gds path construction. For your convenience, the relevant source files are included in this repository.

Katana makes use of the [Gdscpp](https://github.com/judefdiv/gdscpp) library in order to read GDS files. Four your convenience, the relevant source files are included in this repository.

Katana was coded with elements of C++17 and therefore requires a suitable compiler. In order to build on CentOS 7, I recommend using Software Collections Developer Toolset 8. The toolset allows you to avoid having to build a gcc version greater than 7 from source as this process takes around 4 hours on a modern computer. Once Katana is built you can simply turn the toolset off again.

# Installation on CentOS 7
```
sudo yum -y install centos-release-scl-rh devtoolset-8 boost-devel git
git clone git@github.com:HeinrichHerbst/Katana.git
cd Katana
wget https://github.com/Kitware/CMake/releases/download/v3.17.0/cmake-3.17.0-Linux-x86_64.tar.gz
tar xf cmake-3.17.0-Linux-x86_64.tar.gz
rm cmake-3.17.0-Linux-x86_64.tar.gz
mkdir release
cd release
scl enable devtoolset-8 bash
../cmake-3.17.0-Linux-x86_64/bin/cmake -DCMAKE_BUILD_TYPE=Release ..
make
exit
```
# Installation on Ubuntu 19.10
Same as the CentOS install except you can simply install the required software with the native package manager.
```
sudo apt install cmake gcc git libboost-all-dev
git clone git@github.com:HeinrichHerbst/Katana.git
cd Katana
mkdir release
cd release
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```