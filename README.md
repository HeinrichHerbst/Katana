# Katana EDA Process Modeling Assistant
Last updated on 23 April, 2020.
Katana is currently at version 0.9T. Commits will be published to the github page:
[Katana Github](https://github.com/HeinrichHerbst/Katana)

# Overview
Katana is an open-source process-modeling assistant & sectioning program. Katana allows a designer to peer into GDS files and generate two-dimensional Integrated Circuit (IC) cross-sections with the assistance of layer definition information. Additionally, Katana is capable of automatically generating a [FLOOXS](flooxs.ece.ufl.edu/) process-modeling input script. Katana can also combine multiple two-dimensional Gmsh .geo files and convert .msh files back into .geo files (mesh removal). The video below demonstrates typical use of the software. The video is of a pre-release version. See the Usage section for instructions on how to use the newest version.
[![Katana Demonstration](https://img.youtube.com/vi/CsfjJpd8BOQ/0.jpg)](https://www.youtube.com/watch?v=CsfjJpd8BOQ)

# Changelog
0.9.2 Release:
```
 # Added scale functionality to geometry operations.
 # Added support for Mesh spacing argument in geometry files.
```
0.9.1 Release:
```
 # Katana now successfully handles cross-sections submerged in a polygon.
 # Masks will no longer erroneously be generated over entire layer.
```
0.9 Release:
```
    1 - Merge two Gmsh .geo files together
    2 - Simplify .geo files
    3 - Perform coherence optimization on .geo files
    4 - Translate entire .geo file
    5 - Rotate entire .geo file.
    6 - Volume calculation of Gmsh .msh Physical Volumes
```
# Usage
Katana can be executed with input arguments run interactively.
If Katana is run with no input arguments, interactive mode is started.
The following input arguments may be used:
```
    -help
        Print the help information.

    -version
        Print the current version of Katana.
```

Katana is capable of generating cross sections of circuits from mask and process information:

```
    -slice
        Generate a 2D cross-sectional slice through the IC.
        The GDSII format layout file, as well as a layer
        definition file are required as input parameters.
        Katana also requires the x,y co-ordinate pair of
        where to create the slice.

        Format: <Katana> <slice> <gds path> <ldf path> <x1> <y1> <x2> <y2>

        e.g:    ./katana -slice jj.gds mitllsfq5ee.ldf 500 0 500 1000

        Two files are produced; a geometry file and .tcl file.
        The TCL file should be run by FLOOXS to generate a
        cross-section. Both files are saved in the same directory
        as the Katana executable. The geometry file is named
        "cross_section_output.geo". The FLOOXS input script is
        called "katana_generated.tcl".
```

Katana assists in combining multiple cross-sections in order to create a three-dimensional model. All modeling commands make use of the Gmsh .geo file format. In order to define the characteristic length of all points in the .geo file, add either of the following lines to the top of your .geo file:
```
MeshSpac = 1;
cl__1 = 1;
```
Any double can be used in space of the 1. Refer to desired points with that mesh spacing command. Katana currently only stores one characteristic length variable.
```
Point(1) = {1, 1, 1, MeshSpac};
Point(1) = {1, 1, 1, cl__1};
```
The modeling module has the following functionality:
```
    -modeling -m
        Merges two specified geometry files in the following manner:
            1: Imports both files into memory.
            2: Performs coherence check on both file data sets.
            3: Simplifies both file data sets.
            4: Merges both data sets together.
            5: Performs coherence check on the merged data.
            6: Simplifies the merged data.
            7: Writes merged data to specified output file.
        Please see the manual for the Katana definition of coherence
        and simplification and why they are necessary.

        Format <Katana> <modeling> <merge command>
               <first file> <second file> <specified output>

        e.g. ./katana -modeling -m data/left.geo data/right.geo
                      -data/combined.geo

    -modeling -sa
        (legacy. Replaced with more versatile merge above)
        Simple append: Joins two 2D geo-files. Append the second
        to the first. Support for points and lines only.

        Format: <Katana> <modeling> <simple append> <first .geo>
        <second.geo> <output.geo> <char. len. override>(optional)

        e.g.    ./katana -modeling -sa left.geo right.geo both.geo
        The optional arguments overrides the characteristic length
        of all points. See char. length in Gmsh documentation.

    -modeling -t
        Translate entire .geo file. Also perform coherence
        optimization and file simplification.

        Format <Katana> <modeling> <translate command>
        <target file> <delta x> <delta y> <delta z>

        e.g. ./katana -modeling -t data/shape.geo 1000 0 1000

    -modeling -r
        Rotate entire .geo file. Also perform coherence optimization
        and file simplification. Requires an origin of rotation and
        rotation angles in degrees per axis.

        Format <Katana> <modeling> <rotate command>
        <target file> <origin x> <origin y> <origin z>
        <theta x> <theta y> <theta z>

        e.g. ./katana -modeling -r data/shape.geo 0 0 0 30 30 30

    -modeling -scale
        Scale entire .geo file. Also perform coherence optimization
        and file simplification. Requires a scaling factor. Useful
        for when database units are different to expected.

        Format <Katana> <modeling> <scale command>
        <target file> <output file> <factor>

        e.g. ./katana -modeling -scale data/bigshape.geo
             data/smallshape.geo 1e-2
```

Katana has a Gmsh mesh module capable of mesh volume calculation, as well as conversion of FLOOXS meshes back into a geometry format representation (.geo).

```
    -meshops -s
        This is a legacy function since FLOOXS now supports Gmsh .geo
        contour exporting. This function converts 2D mesh surfaces
        into physical surfaces with the mesh triangles removed.

        Format <Katana> <mesh file operations> <Silver Lining Command>
        <target mesh> <geo output path>

        e.g. ./katana -meshops -s data/f_out.msh data/convert.geo

    -meshops -v
        Calculate all Gmsh .msh \"Physical Volume\" values in mesh file.

        Format <Katana> <mesh file operations> <Volume Command>

        e.g. ./katana -meshops -v data/example.msh"
```

# Dependencies
Katana makes use of [Boost](https://www.boost.org/doc/libs/1_66_0/more/getting_started/unix-variants.html) functions and therefore requires the library present before building.

Katana is built in C++ with [CMake](https://cmake.org/install/) and utilizes [the clipper library](http://www.angusj.com/delphi/clipper.phpBoost)  for gds path construction. For your convenience, the relevant source files are included in this repository.

Katana makes use of the [Gdscpp](https://github.com/judefdiv/gdscpp) library in order to read GDS files. Four your convenience, the relevant source files are included in this repository.

Katana was coded with elements of C++17 and therefore requires a suitable compiler.

# Installation on CentOS 7
 In order to build Katana easily on CentOS 7, it is recommended to use Software Collections Developer Toolset 8. The toolset allows one to avoid having to build a gcc version greater than 7 from source as this process takes around 4 hours on a modern computer. Once Katana is built you can simply turn the toolset off again.

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