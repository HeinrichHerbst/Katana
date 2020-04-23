cmake_minimum_required(VERSION 3.13.0)
project(katana VERSION 0.9.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Setting of default environmental variables for standard build ---
IF(NOT DEFINED ENV{GDSCPP_PATH})
    message("No environmental variable for GDSCPP path found")
    message("Setting env variable for gds path to {folder root}/gdscpp")
    set(ENV{GDSCPP_PATH} "gdscpp")
else()
    message("Detected a GDSCPP PATH: " $ENV{GDSCPP_PATH})
endif()
IF(NOT DEFINED ENV{CLIPPER_PATH})
    message("No environmental variable for CLIPPER_PATH found")
    message("Setting CLIPPER_PATH to {current folder}/gdscpp")
    set(ENV{CLIPPER_PATH} "clipper")
else()
    message("Detected CLIPPER_PATH: " $ENV{CLIPPER_PATH})
endif()
# -----------------------------------------------------------------

add_library(gdscpp
            $ENV{GDSCPP_PATH}/src/gdsCpp.cpp
            $ENV{GDSCPP_PATH}/src/gdsForge.cpp
            $ENV{GDSCPP_PATH}/src/gdsImport.cpp
            $ENV{GDSCPP_PATH}/src/gdsParser.cpp
            $ENV{GDSCPP_PATH}/include/gdscpp/gdsCpp.hpp
            $ENV{GDSCPP_PATH}/include/gdscpp/gdsForge.hpp
            $ENV{GDSCPP_PATH}/include/gdscpp/gdsParser.hpp)
target_include_directories(gdscpp PUBLIC $ENV{GDSCPP_PATH}/include/gdscpp)

add_library(clipper
            $ENV{CLIPPER_PATH}/clipper.cpp
            $ENV{CLIPPER_PATH}/clipper.hpp)
target_include_directories(clipper PUBLIC $ENV{CLIPPER_PATH})

add_library(katana_backend
            katana_backend/src/drawing.cpp
            katana_backend/src/flooxs_gen.cpp
            katana_backend/src/ldf_process.cpp
            katana_backend/src/meshfile_operations.cpp
            katana_backend/src/new_meshfile_operations.cpp
            katana_backend/src/pipolly.cpp
            katana_backend/src/section.cpp
            katana_backend/src/stitching.cpp
            katana_backend/src/geofile_operations.cpp
            )
target_include_directories(katana_backend PUBLIC katana_backend/incl)

add_executable(${PROJECT_NAME}  "src/main.cpp" "src/t_interface.cpp")
target_include_directories( ${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/incl)

target_link_libraries(${PROJECT_NAME} PRIVATE gdscpp)
target_link_libraries(katana_backend PRIVATE gdscpp)
target_link_libraries(katana_backend PRIVATE clipper)
target_link_libraries(${PROJECT_NAME} PRIVATE katana_backend)