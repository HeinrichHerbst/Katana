/**
 * Author:      H.F. Herbst
 * Origin:      Stellenbosch University
 * For:         IARPA SuperTools Project
 * license:     MIT
 * Description: Function definitions for layer definition file parsing
 */
#include "ldf_process.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
using namespace std;

enum StringValue {
  not_found,
  parameters,
  layer,
  endsign,
  units,
  number,
  name,
  thickness,
  order,
  mask,
  filmtype,
  colour
};

static std::map<std::string, StringValue> ldf_identifiers;

// Maps identifiers to enumeration for case statement
static void Initialize()
{
  ldf_identifiers["$Parameters\r"] = parameters;
  ldf_identifiers["$Layer\r"] = layer;
  ldf_identifiers["$End\r"] = endsign;
  ldf_identifiers["Units"] = units;
  ldf_identifiers["Number"] = number;
  ldf_identifiers["Name"] = name;
  ldf_identifiers["Thickness"] = thickness;
  ldf_identifiers["Order"] = order;
  ldf_identifiers["Mask"] = mask;
  ldf_identifiers["Filmtype"] = filmtype;
  ldf_identifiers["Colour"] = colour;
}

int ldf::read_ldf(const string &path)
{
  fstream ldf_file;
  string file_buffer;
  ldf_file.open(path, ios::in);
  if (ldf_file.is_open()) {
    Initialize();
    while (getline(ldf_file, file_buffer)) {
      if (!ldf_file.bad()) {
        switch (ldf_identifiers[file_buffer]) {
        case parameters:
          process_params(ldf_file, file_buffer);
          break;
        case layer:
          process_layer(ldf_file, file_buffer);
          break;
        default:
          break;
        }
      } else {
        cout << "Error: reached bad file state." << endl;
      }
    }
  } else {
    cout << "Error: Unable to open LDF file." << endl;
  }
  ldf_file.close();
  return EXIT_SUCCESS;
}

// Returns a vector of strings split by the specified delimiter
std::vector<std::string> split(const std::string &target_string, char delimiter)
{
  stringstream ss(target_string);
  string item;
  vector<string> split_strings;
  while (getline(ss, item, delimiter)) {
    split_strings.push_back(item);
  }
  return split_strings;
}

void ldf::process_params(fstream &my_ldf, string &my_filebuffer)
{
  // load up parameters until end command reached
  bool parameters_found = false;
  do {
    getline(my_ldf, my_filebuffer);
    vector<string> split_buffer = split(my_filebuffer, '=');
    string before_equals = split_buffer.front();
    before_equals.erase(
        std::remove(before_equals.begin(), before_equals.end(), ' '),
        before_equals.end());
    string after_equals = split_buffer.back();
    after_equals.erase(
        std::remove(after_equals.begin(), after_equals.end(), ' '),
        after_equals.end());
    after_equals.pop_back();

    switch (ldf_identifiers[before_equals]) {
    case units:
      database_unit = stof(after_equals);
      parameters_found = true;
      break;
    // If any other global parameters are ever required - put the cases here.
    default:
      break;
    }
  } while ((ldf_identifiers[my_filebuffer] != endsign) &&
           (parameters_found == false));
}

void ldf::process_layer(fstream &my_ldf, string &my_filebuffer)
{
  // load up parameters until end command reached
  int curr_layer_num = 0;
  layer_data curr_layer_data;
  do {
    getline(my_ldf, my_filebuffer);
    vector<string> split_buffer = split(my_filebuffer, '=');
    string before_equals = split_buffer.front();
    before_equals.erase(
        std::remove(before_equals.begin(), before_equals.end(), ' '),
        before_equals.end());
    string after_equals = split_buffer.back();
    after_equals.erase(
        std::remove(after_equals.begin(), after_equals.end(), ' '),
        after_equals.end());
    after_equals.pop_back();
    switch (ldf_identifiers[before_equals]) {
    case number:
      curr_layer_num = stoi(after_equals);
      break;
    case name:
      curr_layer_data.name = after_equals;
      break;
    case thickness:
      // Always stores the thickness in nanometers
      // t = value * database unit (usually um) * 1e9 (to nano)
      curr_layer_data.thickness =
          (int)round(stod(after_equals) * database_unit * 1e9);
      break;
    case order:
      curr_layer_data.order = stoi(after_equals);
      break;
    case mask:
      curr_layer_data.mask = stoi(after_equals);
      break;
    case filmtype:
      curr_layer_data.film_type = after_equals.back();
      break;
    case colour:
      curr_layer_data.colour = stoi(after_equals);
      break;
    default:
      break;
    }
  } while (ldf_identifiers[my_filebuffer] != endsign);
  auto [it, ins] =
      layer_definition.insert_or_assign(curr_layer_num, curr_layer_data);
}

void ldf::print()
{
  auto it = layer_definition.begin();
  cout << "Printing layer definition information." << endl;
  cout << "================= Start ================" << endl;
  while (it != layer_definition.end()) {
    cout << "Layer number = " << it->first << endl;
    cout << "Name         = " << it->second.name << endl;
    cout << "Thickness    = " << it->second.thickness << endl;
    cout << "Order        = " << it->second.order << endl;
    cout << "Mask         = " << it->second.mask << endl;
    cout << "Film type    = " << it->second.film_type << endl;
    cout << "Colour       = " << it->second.colour << endl;
    cout << "----------------------------------------" << endl;
    it++;
  }
  cout << "================== End =================" << endl;
}

// Retrieves the ldf layer data and places it in the target destination
int ldf::pull_layer_data(map<int, layer_data> &target_destination)
{
  target_destination = layer_definition;
  return EXIT_SUCCESS;
}