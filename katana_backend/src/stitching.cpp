/**
 * Author:      H.F. Herbst
 * Origin:      Stellenbosch University
 * For:         IARPA SuperTools Project
 * license:     MIT
 * Description: Function definitions for geofile stitching.
 */
#include "stitching.hpp"

using namespace std;

int print_stitched_file(map<int, node> &a_nodes_map,
                        map<int, stitch::line> &a_lines_map,
                        map<int, node> &b_nodes_map,
                        map<int, stitch::line> &b_lines_map,
                        const string &outfile_path,
                        const string &char_len_over) {
  a_nodes_map.insert(b_nodes_map.begin(), b_nodes_map.end());
  a_lines_map.insert(b_lines_map.begin(), b_lines_map.end());
  ofstream geo_file;
  geo_file.open(outfile_path);
  if (geo_file.is_open()) {
    geo_file << "//========================== Border Nodes ==========================\n";
    auto i = a_nodes_map.begin();
    if (char_len_over == "") {
      while (i != a_nodes_map.end()) {
        geo_file << "Point(" << i->first << ") = {" << i->second.x << ", "
                 << i->second.y << ", " << i->second.z << ", "
                 << i->second.characteristic_length << "};\n";
        i++;
      }
    } else {
      while (i != a_nodes_map.end()) {
        geo_file << "Point(" << i->first << ") = {" << i->second.x << ", "
                 << i->second.y << ", " << i->second.z << ", "
                 << char_len_over << "};\n";
        i++;
      }
    }
    geo_file << "//========================== Shape Faces ===========================\n";
    auto j = a_lines_map.begin();
    while (j != a_lines_map.end()) {
      geo_file << "Line(" << j->first << ") = {" << j->second.first << ", "
               << j->second.second << "};\n";
      j++;
    }
    geo_file << "//============================== END ================================\n";
    geo_file.close();
    return 0;
  } else {
    cout << "Unable to create new meshfile." << endl;
    return 1;
  }
}

// Moves file b over geographically
// int shift_file_b(b_nodes_map, b_lines_map, highest_node_ID, highest_line_ID,
// offset)
int shift_file_b(map<int, node> &b_nodes_map, map<int, stitch::line> &b_lines_map,
                 int &highest_node_ID, int &highest_line_ID, double &offset) {
  // Update the nodes of the second map by increasing the x co-ords by offset
  // and the node ID's by the highest node ID of the first map to avoid
  // collision.
  auto node_iterator = b_nodes_map.begin();
  while (node_iterator != b_nodes_map.end()) {
    if (node_iterator->second.raised == false) {
      auto temp_iterator = node_iterator;
      node_iterator++;
      int temp_key = temp_iterator->first;
      temp_iterator->second.x = temp_iterator->second.x + offset;
      temp_iterator->second.raised = true;
      auto node_handler = b_nodes_map.extract(temp_key);
      node_handler.key() = temp_key + highest_node_ID;
      b_nodes_map.insert(move(node_handler));
    } else {
      node_iterator++;
    }
  }
  // update the line ID's by the maximum values in order to avoid collision.
  // Additionally, update the line node references by the highest node ID
  // so that the lines still point to the same points and not map A's points.
  auto line_iterator = b_lines_map.begin();
  while (line_iterator != b_lines_map.end()) {
    if (line_iterator->second.raised == false) {
      auto temp_iterator = line_iterator;
      line_iterator++;
      int temp_key = temp_iterator->first;
      temp_iterator->second.first = temp_iterator->second.first + highest_node_ID;
      temp_iterator->second.second =temp_iterator->second.second + highest_node_ID;
      temp_iterator->second.raised = true;
      auto node_handler = b_lines_map.extract(temp_key);
      node_handler.key() = temp_key + highest_line_ID;
      b_lines_map.insert(move(node_handler));
    } else {
      line_iterator++;
    }
  }
  return 0;
}

// use the same iteration to search for highest node ID & offset
int calculate_shifting_parameters(map<int, node> &a_nodes_map,
                                  map<int, stitch::line> &a_lines_map,
                                  int &highest_node_ID, int &highest_line_ID,
                                  double &offset) {
  auto node_iterator = a_nodes_map.begin();
  auto last_ele = a_nodes_map.end();
  last_ele--;
  auto last_ele2 = a_lines_map.end();
  last_ele2--;
  highest_node_ID = last_ele->first;
  highest_line_ID = last_ele2->first;
  while (node_iterator != a_nodes_map.end()) {
    if (node_iterator->second.x > offset) {
      offset = node_iterator->second.x;
    }
    node_iterator++;
  }
  return 0;
}

int read_in_geofiles(const string &f1, const string &f2, const string &f3,
                     map<int, node> &a_nodes_map, map<int, stitch::line> &a_lines_map,
                     map<int, node> &b_nodes_map, map<int, stitch::line> &b_lines_map) {
  fstream geo_file;
  string file_buffer;
  geo_file.open(f1, ios::in);
  // Read in first geofile
  // ===================================================================
  if (geo_file.is_open()) {
    while (getline(geo_file, file_buffer)) {
      std::string check = file_buffer.substr(0, 5);
      int current_key;
      vector<string> split_string_vector;
      boost::split(split_string_vector, file_buffer, boost::is_any_of("(),{}"));
      if (check.compare("Point") == 0) {
        node current_node;
        split_string_vector[4].erase(std::remove(split_string_vector[4].begin(),
                                                 split_string_vector[4].end(),
                                                 ' '),
                                     split_string_vector[4].end());
        split_string_vector[5].erase(std::remove(split_string_vector[5].begin(),
                                                 split_string_vector[5].end(),
                                                 ' '),
                                     split_string_vector[5].end());
        split_string_vector[6].erase(std::remove(split_string_vector[6].begin(),
                                                 split_string_vector[6].end(),
                                                 ' '),
                                     split_string_vector[6].end());
        current_key = stoi(split_string_vector[1]);
        current_node.x = strtod(split_string_vector[3].c_str(), NULL);
        current_node.y = strtod(split_string_vector[4].c_str(), NULL);
        current_node.z = strtod(split_string_vector[5].c_str(), NULL);
        current_node.characteristic_length =
            strtod(split_string_vector[6].c_str(), NULL);
        current_node.raised = false;
        a_nodes_map.insert(std::pair<int, node>(current_key, current_node));
      } else if (check.compare("Line(") == 0) {
        stitch::line current_line; // 1 3 4
        current_key = stoi(split_string_vector[1]);
        current_line.first = stoi(split_string_vector[3]);
        current_line.second = stoi(split_string_vector[4]);
        a_lines_map.insert(std::pair<int, stitch::line>(current_key, current_line));
      }
    }
    geo_file.close();
    // Read in second geofile
    // ===================================================================
    geo_file.open(f2, ios::in);
    if (geo_file.is_open()) {
      while (getline(geo_file, file_buffer)) {
        std::string check = file_buffer.substr(0, 5);
        int current_key;
        vector<string> split_string_vector;
        boost::split(split_string_vector, file_buffer,
                     boost::is_any_of("(),{}"));
        if (check.compare("Point") == 0) {
          node current_node;
          split_string_vector[4].erase(
              std::remove(split_string_vector[4].begin(),
                          split_string_vector[4].end(), ' '),
              split_string_vector[4].end());
          split_string_vector[5].erase(
              std::remove(split_string_vector[5].begin(),
                          split_string_vector[5].end(), ' '),
              split_string_vector[5].end());
          split_string_vector[6].erase(
              std::remove(split_string_vector[6].begin(),
                          split_string_vector[6].end(), ' '),
              split_string_vector[6].end());
          current_key = stoi(split_string_vector[1]);
          current_node.x = strtod(split_string_vector[3].c_str(), NULL);
          current_node.y = strtod(split_string_vector[4].c_str(), NULL);
          current_node.z = strtod(split_string_vector[5].c_str(), NULL);
          current_node.characteristic_length =
              strtod(split_string_vector[6].c_str(), NULL);
          current_node.raised = false;
          b_nodes_map.insert(std::pair<int, node>(current_key, current_node));
        } else if (check.compare("Line(") == 0) {
          stitch::line current_line; // 1 3 4
          current_key = stoi(split_string_vector[1]);
          current_line.first = stoi(split_string_vector[3]);
          current_line.second = stoi(split_string_vector[4]);
          b_lines_map.insert(std::pair<int, stitch::line>(current_key, current_line));
        }
      }
      geo_file.close();
    } else {
      cout << "Unable to open second geo_file. Terminating program." << endl;
      return 1;
    }
  } else {
    cout << "Unable to open first geo_file. Terminating program." << endl;
    return 1;
  }
  return 0;
}

void simple_append(     std::string first_geo,
                        std::string second_geo,
                        std::string output_geo,
                        std::string char_len)
{
    std::map<int, node> a_nodes_map;
    std::map<int, stitch::line> a_lines_map;
    std::map<int, node> b_nodes_map;
    std::map<int, stitch::line> b_lines_map;
    int highest_node_ID = 0;
    int highest_line_ID = 0;
    double offset = 0;
    int status = read_in_geofiles(  first_geo, second_geo, output_geo,
                                    a_nodes_map, a_lines_map,
                                    b_nodes_map, b_lines_map);
    if (status==0)
    {
        status = calculate_shifting_parameters( a_nodes_map, a_lines_map, highest_node_ID,
                                                highest_line_ID, offset );
        if (status==0)
        {
            status = shift_file_b(b_nodes_map, b_lines_map, highest_node_ID, highest_line_ID, offset);
            if (status==0)
            {
                print_stitched_file(a_nodes_map, a_lines_map, b_nodes_map, b_lines_map, output_geo, char_len);
                std::cout << "Program finished execution." << endl;
            }
            else
            {
                std::cout << "Shifting of nodes failed."<< endl;
            }
        }
        else
        {
            std::cout << "Error: Failed to calculate shifting parameters." << endl;
        }
    }
    else
    {
        std::cout << "Error: unable to read in geofiles." << endl;
    }
}
