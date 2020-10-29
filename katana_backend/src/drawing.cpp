/**
 * Author:      H.F. Herbst
 * Origin:      Stellenbosch University
 * For:         IARPA SuperTools Project
 * license:     MIT
 * Description: Function definitions for.geo file drawing
 */
#include "drawing.hpp"
using namespace std;
int draw_geofile(section_data section_info, ldf ldf_info, const string &outpath,
                 const double &db_unit)
{
  // ostringstream curve;
  // string curve_loop = curve.str();
  // geo_file << curve_loop;
  // geo_file <<  "//======= Border Nodes ======\n";
  map<int, layer_data> my_layer_info;
  map<int, map<int, building_blocks>> my_blocks;
  map<char, vector<int>> physical_volumes;
  vector<int> layer_order;
  ofstream geo_file;

  ldf_info.pull_layer_data(my_layer_info);
  section_info.pull_section_blocks(my_blocks);
  section_info.pull_used_layers(layer_order);
  get_layer_order(layer_order, my_layer_info);

  int cross_section_distance = section_info.get_sec_dist();
  auto layer_it = layer_order.begin();
  geo_file.open(outpath);
  int point_counter = 0;
  int line_counter = 0;
  int curve_loop_counter = 0;
  int current_height = 0;
  while (layer_it != layer_order.end())
  {
    if (my_layer_info[*layer_it].film_type ==
        'Z') // don't draw if unspecified film
    {
      layer_it++;
      if (layer_it == layer_order.end())
        break;
    }
    geo_file << "// =============== Layer number [" << *layer_it << "] ==============\n";
    // Layer has section data
    if (my_blocks.count(*layer_it) > 0)
    {
      geo_file << "// ========== Layer with identfied x-sec ==========\n";
      bool draw_layer = false;
      if (my_layer_info[*layer_it].mask == 1)
      {
        draw_layer = true;
      }
      else if (my_layer_info[*layer_it].mask == -1)
      {
        draw_layer = true;
      }
      else if (my_layer_info[*layer_it].mask == 0) //all layers translated to model currently
      {
        draw_layer = true;
      }
      if (draw_layer == true)
      {
        auto block_it = my_blocks[*layer_it].begin();
        while (block_it != my_blocks[*layer_it].end())
        {
          geo_file << "//------- Block ------\n";
          // geo_file << "// Lower-left point\n";
          geo_file << "Point(" << ++point_counter << ") = {"
                   << block_it->second.start_distance * db_unit << ", "
                   << current_height * 1e-9 << ", 0, 1.0};\n";
          // geo_file << "// Lower-right point\n";
          geo_file << "Point(" << ++point_counter << ") = {"
                   << block_it->second.end_distance * db_unit << ", "
                   << current_height * 1e-9 << ", 0, 1.0};\n";
          // geo_file << "// Bottom line\n";
          geo_file << "Line(" << ++line_counter << ") = {" << point_counter - 1
                   << ", " << point_counter << "};\n";
          current_height = current_height + my_layer_info[*layer_it].thickness;
          // geo_file << "// Upper right point\n";
          geo_file << "Point(" << ++point_counter << ") = {"
                   << block_it->second.end_distance * db_unit << ", "
                   << current_height * 1e-9 << ", 0, 1.0};\n";
          // geo_file << "// Right-side vertical line\n";
          geo_file << "Line(" << ++line_counter << ") = {" << point_counter - 1
                   << ", " << point_counter << "};\n";
          // geo_file << "// Upper-left point\n";
          geo_file << "Point(" << ++point_counter << ") = {"
                   << block_it->second.start_distance * db_unit << ", "
                   << current_height * 1e-9 << ", 0, 1.0};\n";
          // geo_file << "// Top line\n";
          geo_file << "Line(" << ++line_counter << ") = {" << point_counter - 1
                   << ", " << point_counter << "};\n";
          // geo_file << "// Left-side vertical line\n";
          geo_file << "Line(" << ++line_counter << ") = {" << point_counter
                   << ", " << point_counter - 3 << "};\n";
          // geo_file << "// Curve loop for the rectangle\n";
          geo_file << "Curve Loop(" << ++curve_loop_counter << ") = {"
                   << line_counter - 3 << ", " << line_counter - 2 << ", "
                   << line_counter - 1 << ", " << line_counter << "};\n";
          // geo_file << "// Plane surface for rectangle\n";
          geo_file << "Plane Surface(" << curve_loop_counter << ") = {"
                   << curve_loop_counter << "};\n";
          current_height = current_height - my_layer_info[*layer_it].thickness;
          // Add the surface to substance or non substance vector
          // check for substance or absence
          if (((block_it->second.primary == true) &&
               (my_layer_info[*layer_it].mask == 1)) ||
              ((block_it->second.primary == false) &&
               (my_layer_info[*layer_it].mask == -1)))
          { //Substance vector
            // search through physical volumes map for the character of the film type this is
            if (physical_volumes.count(my_layer_info[*layer_it].film_type) > 0) // Check map for key
            {
              physical_volumes[my_layer_info[*layer_it].film_type].push_back(curve_loop_counter);
            }
            else // no character of this type was found yet, add it to the map
            {
              vector<int> newvector;
              newvector.push_back(curve_loop_counter);
              physical_volumes.insert(pair<char, vector<int>>(my_layer_info[*layer_it].film_type, newvector));
            }
          }
          else //Material void. Determine what physical volume to assign block to
          {
            if (physical_volumes.count('V') > 0) // Check map for key
            {
              physical_volumes['V'].push_back(curve_loop_counter);
            }
            else // no character of this type was found yet, add it to the map
            {
              vector<int> newvector;
              newvector.push_back(curve_loop_counter);
              physical_volumes.insert(pair<char, vector<int>>('V', newvector));
            }
            /*bool cavity_filled = false;
            int layer_number = *layer_it;
            while (cavity_filled == false)
            {
              int layer_order = my_layer_info[layer_number].order;
              layer_order++;
              // Find the layer number associated with the order. ~~~~~~~~~~~~~~~~~~~
              auto my_layer_iterator = my_layer_info.begin();
              while (my_layer_iterator != my_layer_info.end())
              {
                if (layer_order == my_layer_iterator->second.order)
                {
                  layer_number = my_layer_iterator->first;
                  break;
                }
                my_layer_iterator++;
                // Reached the end of the line but never found a cavity filler \|/
                if (my_layer_iterator == my_layer_info.end())
                {
                  if (physical_volumes.count('V') > 0) // Check map for key
                  {
                    physical_volumes['V'].push_back(curve_loop_counter);
                  }
                  else // no character of this type was found yet, add it to the map
                  {
                    vector<int> newvector;
                    newvector.push_back(curve_loop_counter);
                    physical_volumes.insert(pair<char, vector<int>>('V', newvector));
                  }
                  cout << "No layer above etched layer. Creating void physical volume" << endl;
                }
              }
              // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
              // You have the new layer number, now search for blocks on that layer
              if (my_layer_info[layer_number].mask == 1) // Not inverted layer
              {

              }
              else // Inverted layer
              {

              }
            }
            */
          }
          block_it++;
        }
        current_height = current_height + my_layer_info[*layer_it].thickness;
        //  TODO: Implement later [At end of layer, add them to vector of that material name]
        // add all physical surfaces to respective groups
        // my_layer_info[layer_it->first].
      }
    }
    else // Layer has no section data. Perform autofill
    {
      // Regardless on mask type, empty layer is filled with it's material
      if (my_layer_info[*layer_it].mask == -1)
      {
        geo_file << "// ======== Layer with no identfied x-secs ========\n";
        // geo_file << "// Lower-left point\n";
        geo_file << "Point(" << ++point_counter << ") = {" << 0 << ", "
                 << current_height * 1e-9 << ", 0, 1.0};\n";
        // geo_file << "// Lower-right point\n";
        geo_file << "Point(" << ++point_counter << ") = {"
                 << cross_section_distance * db_unit << ", "
                 << current_height * 1e-9 << ", 0, 1.0};\n";
        // geo_file << "// Bottom line\n";
        geo_file << "Line(" << ++line_counter << ") = {" << point_counter - 1
                 << ", " << point_counter << "};\n";
        current_height = current_height + my_layer_info[*layer_it].thickness;
        // geo_file << "// Upper-right point\n";
        geo_file << "Point(" << ++point_counter << ") = {"
                 << cross_section_distance * db_unit << ", "
                 << current_height * 1e-9 << ", 0, 1.0};\n";
        // geo_file << "// Right-side vertical line\n";
        geo_file << "Line(" << ++line_counter << ") = {" << point_counter - 1
                 << ", " << point_counter << "};\n";
        // geo_file << "// Upper-left point\n";
        geo_file << "Point(" << ++point_counter << ") = {" << 0 << ", "
                 << current_height * 1e-9 << ", 0, 1.0};\n";
        // geo_file << "// Top-side line\n";
        geo_file << "Line(" << ++line_counter << ") = {" << point_counter - 1
                 << ", " << point_counter << "};\n";
        // geo_file << "// Left-side vertical line\n";
        geo_file << "Line(" << ++line_counter << ") = {" << point_counter
                 << ", " << point_counter - 3 << "};\n";
        // geo_file << "// Curve loop for the rectangle\n";
        geo_file << "Curve Loop(" << ++curve_loop_counter << ") = {"
                 << line_counter - 3 << ", " << line_counter - 2 << ", "
                 << line_counter - 1 << ", " << line_counter << "};\n";
        // geo_file << "// Plane surface for rectangle\n";
        geo_file << "Plane Surface(" << curve_loop_counter << ") = {"
                 << curve_loop_counter << "};\n";
        // add to the physical surface list
        if (physical_volumes.count(my_layer_info[*layer_it].film_type) >
            0) // Check map for key
        {
          physical_volumes[my_layer_info[*layer_it].film_type].push_back(
              curve_loop_counter);
        }
        else
        {
          vector<int> newvector;
          newvector.push_back(curve_loop_counter);
          physical_volumes.insert(pair<char, vector<int>>(
              my_layer_info[*layer_it].film_type, newvector));
        }
      }
    }
    layer_it++;
  }
  // Assign physical volumes
  // Physical Surface("Right") = {1, 2};
  auto physical_surface_iterator = physical_volumes.begin();
  geo_file << "//========== Surface data ===========\n";
  while (physical_surface_iterator != physical_volumes.end())
  {
    geo_file << "Physical Surface(\"" << physical_surface_iterator->first
             << "\") = {";
    ostringstream surfaces;
    auto vec_iter = physical_surface_iterator->second.begin();
    auto vec_iter_end = --(physical_surface_iterator->second.end());
    while (vec_iter != vec_iter_end)
    {
      surfaces << *vec_iter << ", ";
      vec_iter++;
    }
    surfaces << *vec_iter;
    string curve_loop = surfaces.str();
    geo_file << curve_loop << "};\n";
    // Colour in insulators and niobium
    switch (physical_surface_iterator->first)
    {
    case 'A': // Auxiliary
      geo_file << "Color Orange {Surface {" << curve_loop << "};}\n";
      break;
    case 'I': // Isolator A5B Si02
      geo_file << "Color Green {Surface {" << curve_loop << "};}\n";
      break;
    case 'J': // Isolator
      geo_file << "Color Purple {Surface {" << curve_loop << "};}\n";
      break;
    case 'L': // A5C Si02
      geo_file << "Color Cyan {Surface {" << curve_loop << "};}\n";
      break;
    case 'N': // Normal Conductor
      geo_file << "Color Pink {Surface {" << curve_loop << "};}\n";
      break;
    case 'R': // Resistor
      geo_file << "Color Red {Surface {" << curve_loop << "};}\n";
      break;
    case 'S': // Superconductor
              // Color {200,100,50} {Surface {1, 2};}
      geo_file << "Color Blue {Surface {" << curve_loop << "};}\n";
      break;
    case 'V': // Normal Conductor
      geo_file << "Color Grey {Surface {" << curve_loop << "};}\n";
      break;
    default:
      break;
    }
    physical_surface_iterator++;
  }
  geo_file.close();
  return EXIT_SUCCESS;
}

// Adds data into a vector of layer_numbers (int) which must be drawn.
// The vector is ordered from lowest layer to highest layer of the wafer.
// Param 1 = Destination vector (must already hold all layer keys)
// Param 2 = Source of LDF info
int get_layer_order(vector<int> &layers, map<int, layer_data> &layer_info)
{
  // Translate layer numbers to order
  auto layer_it = layers.begin();
  while (layer_it != layers.end())
  {
    // int order = layer_info[*layer_it].order;
    *layer_it = layer_info[*layer_it].order;
    layer_it++;
  }
  sort(layers.begin(), layers.end());
  // Translate back to layer
  layer_it = layers.begin();
  while (layer_it != layers.end())
  {
    // search through map for matching order
    auto ldf_iter = layer_info.begin();
    while (ldf_iter != layer_info.end())
    {
      if (ldf_iter->second.order == *layer_it)
      {
        *layer_it = ldf_iter->first;
        ldf_iter = layer_info.end();
      }
      else
      {
        ldf_iter++;
      }
    }
    layer_it++;
  }
  return EXIT_SUCCESS;
}
