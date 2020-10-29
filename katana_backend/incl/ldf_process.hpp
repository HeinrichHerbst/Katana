/**
 * Author:      H.F. Herbst
 * Origin:      Stellenbosch University
 * For:         IARPA SuperTools Project
 * license:     MIT
 * Description: Function declarations for layer definition file parsing
 */
#ifndef ldfprocess
#define ldfprocess

#include "section.hpp"
#include <string>
#include <vector>

class ldf
{
private:
  double database_unit;
  std::map<int, layer_data> layer_definition; // key = layer number
  void process_params(std::fstream &my_ldf, std::string &my_filebuffer);
  void process_layer(std::fstream &my_ldf, std::string &my_filebuffer);

public:
  int read_ldf(const std::string &path);
  int pull_layer_data(std::map<int, layer_data> &target_destination);
  void print();
  ldf()
  {
    database_unit = 0;
  }
  ~ldf() {}
};

std::vector<std::string> split(const std::string &target_string,
                               char delimiter);
static void Initialize();
#endif