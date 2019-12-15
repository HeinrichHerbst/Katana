# Documentation Under Construction
Last updated on 15 December, 2019.
Katana is currently pre-release. Open-source code under the MIT License will be accessible by July 2020. Commits will be published to the github page:
[Katana Github](https://github.com/HeinrichHerbst/Katana)

# Overview
Katana is an open-source process-modeling assistant & sectioning program. Katana allows a designer to peer into GDS files and generate two-dimensional Integrated Circuit (IC) cross-sections with the assistance of layer definition information. Additionally, Katana is capable of automatically generating a [FLOOXS](flooxs.ece.ufl.edu/) process-modeling input script.

[![Katana Demonstration](https://img.youtube.com/vi/CsfjJpd8BOQ/0.jpg)](https://www.youtube.com/watch?v=CsfjJpd8BOQ)

There are three main modules of Katana: Slice, the cross-section generator, Silver Linings, the mesh file to geometry file converter, and Quilt, the geometry stitching module.
<img src="images/Overview.png" alt="Katana Overview" class="inline"/>

# Slice
Slice takes six input parameters. The first parameter is the path to the GDS file, which holds the IC geometry information. The second parameter is the path to the layer definition file, which defines information such as layer thickness, material type and mask type. The last four parameters are the two co-ordinate pairs of where the two-dimensional cross-section should be taken. The co-ordinates are in database units as specified in the .gds file.
Example input:
```
<program call> <gds path> <ldf_path> <x1> <y1> <x2> <y2>
./Slice josephson_junction.gds mitll_sfq5ee.ldf 1000 0 1000 5000
```

# Silver Linings
Silver Linings is a module of Katana which takes a two-dimensional FLOOXS mesh file and identifies the boundaries of each material segment. Silver Linings uses this information to generate a [Gmsh](gmsh.info) geometry (.geo) file, which can be edited in Gmsh and used to modify and re-mesh the FLOOXS output.
Example input:
```
<program call> <mesh_path> <geo output path>
./SilverLining flooxs_out.msh flooxs_out.geo
```
# Quilt
Quilt is the module which combines multiple geometry files. Currently, Quilt only attaches geometry files from left to right. The characteristic length of all points can be overridden with a fifth argument.
Example input:
```
<program call> <first geo path> <second geo path> <combined path> <char. length>
./Quilt zone_1.geo zone_2.geo zone_1-2.geo 3e-8
```
# Dependencies
Katana is built in C++ with CMake and utilizes [the clipper library](http://www.angusj.com/delphi/clipper.phpBoost)  for gds path construction.
# Installation
The installation section will be updated once Katana is released publically.