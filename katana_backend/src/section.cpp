/**
 * Author:      H.F. Herbst
 * Origin:      Stellenbosch University
 * For:         IARPA SuperTools Project
 * license:     MIT
 * Description: Function definitions for cross-section generation.
 */
#include "section.hpp"
#include "clipper.hpp"
#include "gdsCpp.hpp"
#include "pipolly.hpp"
#include <cmath>
using namespace std;

void section_data::polygon_to_gds(const string &outpath)
{
  gdscpp my_polygons;
  gdsSTR main;
  main.name = "Polygons";
  vector<int> x_cor, y_cor;
  // std::map<unsigned int, std::vector<co_ord>> polygons; // key = layer number
  for (auto polymap_it = polygons.begin(); polymap_it != polygons.end();
       polymap_it++) {
    for (auto layer_polygon_it = polymap_it->second.begin();
         layer_polygon_it < polymap_it->second.end(); layer_polygon_it++) {
      main.BOUNDARY.push_back(drawBoundary(
          polymap_it->first, layer_polygon_it->x, layer_polygon_it->y));
    }
  }
  my_polygons.setSTR(main);
  my_polygons.write(outpath);
}

int section_data::populate(int &xx1, int &yy1, int &xx2, int &yy2,
                           gdscpp &gds_file)
{
  if (calibrate_axes(xx1, yy1, xx2, yy2) == EXIT_SUCCESS) {
    populate_polygons(gds_file);
    calculate_intercepts();
    unique_sort_intercepts();
    print_intercepts();
    generate_blocks();
    improve_blocks();
    return EXIT_SUCCESS;
  } else {
    return EXIT_FAILURE;
  }
}

// Stores cross-section co-ordinates.
// Determines the kind of section being taken.
// Return values:
// 0 = Invalid      cross-section
// 1 = Horizontal   cross-section
// 2 = Diagonal     cross-section
// 3 = Vertical     cross-section
int section_data::calibrate_axes(int xx1, int yy1, int xx2, int yy2)
{
  x1 = xx1;
  y1 = yy1;
  x2 = xx2;
  y2 = yy2;
  xsec_distance =
      (int)round(sqrt(((double)x2 - (double)x1) * ((double)x2 - (double)x1) +
                      ((double)y2 - (double)y1) * ((double)y2 - (double)y1)));
  int holder;

  if ((x1 == x2) && (y1 == y2)) // Single point
  {
    cout << "Slicing between [" << x1 << ", " << y1 << "] and [" << x2 << ", "
         << y2 << "]" << endl;
    cout << "Error: Cannot slice on a single point.";
    return EXIT_FAILURE;
  } else if (y1 == y2) // Horizontal cross-section
  {
    cout << "Horizontal cross-section" << endl;
    axis_type = 1;
    if (x1 > x2) // Ensure x1 is always LE to x2
    {
      holder = x2;
      x2 = x1;
      x1 = holder;
    }
    cout << "Slicing between [" << x1 << ", " << y1 << "] and [" << x2 << ", "
         << y2 << "]" << endl;
    return EXIT_SUCCESS;
  } else if (x1 == x2) // Vertical cross-section
  {
    cout << "Vertical cross-section" << endl;
    axis_type = 3;
    if (y1 > y2) // Ensure y1 is always LE to y2
    {
      holder = y2;
      y2 = y1;
      y1 = holder;
    }
    cout << "Slicing between [" << x1 << ", " << y1 << "] and [" << x2 << ", "
         << y2 << "]" << endl;
    return EXIT_SUCCESS;
  } else if ((x1 != x2) && (y1 != y2)) {
    cout << "Diagonal cross-section" << endl;
    axis_type = 2;
    if (x1 > x2) // Ensure x1 is always LE to x2
    {
      holder = x2;
      x2 = x1;
      x1 = holder;
      holder = y2;
      y2 = y1;
      y1 = holder;
    }
    cout << "Slicing between [" << x1 << ", " << y1 << "] and [" << x2 << ", "
         << y2 << "]" << endl;
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}

/**
 *  @brief Calls polygon vs cross-section check for all stuctures in a
 * heirarchical fashion.
 *  @param gds_file Gds file which holds the structure information
 *  @return 0 - Function completed successfully.
 */
int section_data::populate_polygons(gdscpp &gds_file)
{
  vector<gdsSTR> structure_vector;
  unordered_map<string, int> structure_lookup;
  gds_file.getSTR(structure_vector, structure_lookup);
  for (auto str_it = structure_vector.begin(); str_it != structure_vector.end();
       str_it++) {
    if (str_it->heirarchical_level == 0) {
      // check for intercept here
      if (check_for_intercept(str_it->bounding_box, axis_type,
                              x1, y1, x2, y2) == true)
      {
        std::map<unsigned int, std::vector<co_ord>> polygon_data;
        recursive_unpack(str_it->name, structure_vector, structure_lookup,
                         polygon_data);
        // append polygon data to polygons map
        for (auto poly_it = polygon_data.begin(); poly_it != polygon_data.end();
             poly_it++) {
          if (polygons.count(poly_it->first) > 0) {
            polygons[poly_it->first].insert(polygons[poly_it->first].end(),
                                            poly_it->second.begin(),
                                            poly_it->second.end());
          } else {
            polygons.insert(pair<unsigned int, vector<co_ord>>(
                poly_it->first, poly_it->second));
          }
        }
      }
    }
  }
  return EXIT_SUCCESS;
}

/**
 *  @brief Checks if intercept interacts with bounding box
 *  @param bounding_box[4] Holds the bounding box
 *  @return true = interacts, false = doesn't interact
 */
bool check_for_intercept(int *bounding_box, int &axis_type, int &x1, int &y1,
                         int &x2, int &y2)
{
  // If either point lies inside the box immediately return true
  if (((x1 >= bounding_box[0]) && (x1 <= bounding_box[2]) &&
       (y1 >= bounding_box[1]) && (y1 <= bounding_box[3])) ||
      ((x2 >= bounding_box[0]) && (x2 <= bounding_box[2]) &&
       (y2 >= bounding_box[1]) && (y2 <= bounding_box[3]))) {
    return true;
  }
  // At this stage, neither point lies inside the bounding box.
  switch (axis_type) {
  case 1: // Horizontal
    if ((y1 >= bounding_box[1]) && (y1 <= bounding_box[3]) &&
        (x1 <= bounding_box[0]) && (x2 >= bounding_box[0])) {
      return true;
    } else {
      return false;
    }
    break;
  case 3: // Vertical
    if ((y1 <= bounding_box[1]) && (y2 >= bounding_box[1]) &&
        (x1 >= bounding_box[0]) && (x1 <= bounding_box[2])) {
      return true;
    } else {
      return false;
    }
    break;
  default: // Angled
    double m = ((double)y2 - (double)y1) / ((double)x2 - (double)x1);
    double c = (double)y1 - m * (double)x1;
    // Bottom line of bounding box
    double x_intercept = round(((double)bounding_box[1] - c) / m);
    if ((x_intercept >= (double)bounding_box[0]) &&
        (x_intercept <= (double)bounding_box[2])) {
      return true;
    }
    // Right line of bounding box
    double y_intercept = round(m * (double)bounding_box[2] + c);
    if ((y_intercept >= (double)bounding_box[1]) &&
        (y_intercept <= (double)bounding_box[3])) {
      return true;
    }
    // Top line of bounding box
    x_intercept = round(((double)bounding_box[3] - c) / m);
    if ((x_intercept >= (double)bounding_box[0]) &&
        (x_intercept <= (double)bounding_box[2])) {
      return true;
    }
    // Left line of bounding box
    y_intercept = round(m * (double)bounding_box[0] + c);
    if ((y_intercept >= (double)bounding_box[1]) &&
        (y_intercept <= (double)bounding_box[3])) {
      return true;
    }
    // None of the above 4 lines had cross sections therefore this point was
    // reached.
    return false;
    break;
  }
}

void recursive_unpack(
    const std::string &structure_name, std::vector<gdsSTR> &struct_vect,
    std::unordered_map<std::string, int> &struct_lookup,
    std::map<unsigned int, std::vector<co_ord>> &retrieved_polygon_data)
{
  std::map<unsigned int, std::vector<co_ord>> polygon_data;
  int structure_index = struct_lookup[structure_name];
  process_boundaries(struct_vect[structure_index].BOUNDARY, polygon_data);
  process_boxes(struct_vect[structure_index].BOX, polygon_data);
  process_paths(struct_vect[structure_index].PATH, polygon_data);
  process_SREFs(struct_vect[structure_index].SREF, polygon_data, struct_vect,
                struct_lookup);
  process_AREFs(struct_vect[structure_index].AREF, polygon_data, struct_vect,
                struct_lookup);
  combine_maps(retrieved_polygon_data, polygon_data);
}

// Returns what type of line the polygon is.
// 1 = horizontal   line
// 2 = diagonal     line
// 3 = vertical     line
// 4 = error
int calibrate_line(int line_start[2], int line_end[2])
{
  int holder;
  if ((line_start[0] == line_end[0]) &&
      (line_start[1] == line_end[1])) // single point
  {
    return 4;
  } else if (line_start[1] == line_end[1]) {
    if (line_start[0] > line_end[0]) // Ensure x1 is always LE to x2
    {
      holder = line_end[0];
      line_end[0] = line_start[0];
      line_start[0] = holder;
    }
    return 1;
  } else if (line_start[0] == line_end[0]) {
    if (line_start[1] > line_end[1]) // Ensure y1 is always LE to y2 (Important
                                     // for slice_horiz_vertical())
    {
      holder = line_end[1];
      line_end[1] = line_start[1];
      line_start[1] = holder;
    }
    return 3;
  } else if ((line_start[0] != line_end[0]) &&
             (line_start[1] != line_end[1])) // diagonal
  {
    if (line_start[0] > line_end[0]) // Ensure x1 is always LE to x2
    {
      holder = line_end[0];
      line_end[0] = line_start[0];
      line_start[0] = holder;
      holder = line_end[1];
      line_end[1] = line_start[1];
      line_start[1] = holder;
    }
    return 2;
  }
  return EXIT_FAILURE;
}

// Runs through all boundaries of structure and
// adds them to the polygons map.
int process_boundaries(vector<gdsBOUNDARY> &current_boundaries,
                       map<unsigned int, vector<co_ord>> &poly_map_destination)
{
  auto bound_iter = current_boundaries.begin();
  int key = 0;
  co_ord temp_co;
  while (bound_iter != current_boundaries.end()) {
    key = bound_iter->layer;
    temp_co.x = bound_iter->xCor;
    temp_co.y = bound_iter->yCor;
    if (poly_map_destination.count(key) > 0) // append
    {
      poly_map_destination[key].push_back(temp_co);
    } else {
      vector<co_ord> new_vector;
      new_vector.push_back(temp_co);
      poly_map_destination.insert(
          pair<unsigned int, vector<co_ord>>(bound_iter->layer, new_vector));
    }
    bound_iter++;
  }
  return EXIT_SUCCESS;
}

int process_boxes(
    std::vector<gdsBOX> &current_boxes,
    std::map<unsigned int, std::vector<co_ord>> &poly_map_destination)
{
  auto box_iter = current_boxes.begin();
  int key = 0;
  co_ord temp_co;
  while (box_iter != current_boxes.end()) {
    key = box_iter->layer;
    temp_co.x = box_iter->xCor;
    temp_co.y = box_iter->yCor;
    if (poly_map_destination.count(key) > 0) // append
    {
      poly_map_destination[key].push_back(temp_co);
    } else {
      vector<co_ord> new_vector;
      new_vector.push_back(temp_co);
      poly_map_destination.insert(
          pair<unsigned int, vector<co_ord>>(box_iter->layer, new_vector));
    }
    box_iter++;
  }
  return EXIT_SUCCESS;
}

int process_paths(
    std::vector<gdsPATH> &current_paths,
    std::map<unsigned int, std::vector<co_ord>> &poly_map_destination)
{
  auto path_iter = current_paths.begin();
  int key = 0;
  co_ord temp_co;
  while (path_iter != current_paths.end()) {
    path_to_polygon(*path_iter, temp_co);
    if (poly_map_destination.count(key) > 0) // append
    {
      poly_map_destination[key].push_back(temp_co);
    } else {
      vector<co_ord> new_vector;
      new_vector.push_back(temp_co);
      poly_map_destination.insert(
          pair<unsigned int, vector<co_ord>>(path_iter->layer, new_vector));
    }
    path_iter++;
  }
  return EXIT_SUCCESS;
}

// for all SREF
// create a sref_map
// call recursive unpack on the SREF structure name to populate the sref_map
// apply transformation and rotation to all sref_map polygons
// add sref_map to polygon_data map
int process_SREFs(
    std::vector<gdsSREF> &current_SREFs,
    std::map<unsigned int, std::vector<co_ord>> &poly_map_destination,
    std::vector<gdsSTR> &struct_vect,
    std::unordered_map<std::string, int> &struct_lookup)
{
  for (auto sref_iter = current_SREFs.begin(); sref_iter < current_SREFs.end();
       sref_iter++) {
    std::map<unsigned int, std::vector<co_ord>> polygon_data;
    recursive_unpack(sref_iter->name, struct_vect, struct_lookup, polygon_data);
    transform_polygon_map(polygon_data, sref_iter->reflection, sref_iter->scale,
                          sref_iter->angle, sref_iter->xCor, sref_iter->yCor);
    combine_maps(poly_map_destination, polygon_data);
  }
  return EXIT_SUCCESS;
}

// for all AREF
// create an aref_map
// perform a lookup of the structure name
// call recursive unpack on the AREF structure name to populate the aref_map
// apply transformations to all aref_map polygons
// add aref_map to polygon_data map
int process_AREFs(
    std::vector<gdsAREF> &current_arefs,
    std::map<unsigned int, std::vector<co_ord>> &poly_map_destination,
    std::vector<gdsSTR> &struct_vect,
    std::unordered_map<std::string, int> &struct_lookup)
{
  for (auto aref_iter = current_arefs.begin(); aref_iter < current_arefs.end();
       aref_iter++) {
    std::map<unsigned int, std::vector<co_ord>> polygon_data;
    recursive_unpack(aref_iter->name, struct_vect, struct_lookup, polygon_data);
    transform_aref_map(polygon_data, *aref_iter);
    combine_maps(poly_map_destination, polygon_data);
  }
  return EXIT_SUCCESS;
}

void combine_maps(std::map<unsigned int, std::vector<co_ord>> &main,
                  std::map<unsigned int, std::vector<co_ord>> &auxiliary)
{
  // check if aux layer exists in main
  for (auto aux_iter = auxiliary.begin(); aux_iter != auxiliary.end();
       aux_iter++) {
    if (main.count(aux_iter->first) > 0) {
      main[aux_iter->first].insert(main[aux_iter->first].end(),
                                   aux_iter->second.begin(),
                                   aux_iter->second.end());
    } else {
      main.insert(pair<unsigned int, vector<co_ord>>(aux_iter->first,
                                                     aux_iter->second));
    }
  }
}

int transform_polygon_map(
    std::map<unsigned int, std::vector<co_ord>> &polygon_map, bool reflect_x,
    double magnification, int angle, int x_offset, int y_offset)
{
  // For each layer
  for (auto map_iter = polygon_map.begin(); map_iter != polygon_map.end();
       map_iter++) {
    // For each co-ordinate set on a layer:
    for (auto vec_iter = map_iter->second.begin();
         vec_iter != map_iter->second.end(); vec_iter++) {
      //  ============================= Reflect =============================
      if (reflect_x == true) {
        double myconstant{-1};
        std::transform(vec_iter->y.begin(), vec_iter->y.end(),
                       vec_iter->y.begin(),
                       [myconstant](auto &c) { return c * myconstant; });
      }
      //  ============================ Multiply =============================
      if (magnification != 0) {
        std::transform(vec_iter->x.begin(), vec_iter->x.end(),
                       vec_iter->x.begin(),
                       [magnification](auto &c) { return c * magnification; });
        std::transform(vec_iter->y.begin(), vec_iter->y.end(),
                       vec_iter->y.begin(),
                       [magnification](auto &c) { return c * magnification; });
      }
      //  ============================= Rotate ==============================
      if (angle != 0) {
        vector<POINT> point_vector;
        // Convert to compatible type for rotation function
        auto vec_it_x = vec_iter->x.begin();
        auto vec_it_y = vec_iter->y.begin();
        while (vec_it_x != vec_iter->x.end()) {
          POINT temp_point;
          temp_point.x = (double)*vec_it_x;
          temp_point.y = (double)*vec_it_y;
          point_vector.push_back(temp_point);
          vec_it_x++;
          vec_it_y++;
        }
        // Convert back to compatible type for
        rotate_vector(0, 0, angle, point_vector);
        vec_it_x = vec_iter->x.begin();
        vec_it_y = vec_iter->y.begin();
        auto point_it = point_vector.begin();
        while (vec_it_x != vec_iter->x.end()) {
          *vec_it_x = (int)round(point_it->x);
          *vec_it_y = (int)round(point_it->y);
          vec_it_x++;
          vec_it_y++;
          point_it++;
        }
      }
      //  ============================= Offset ==============================
      if ((x_offset != 0) || (y_offset != 0)) {
        std::transform(vec_iter->x.begin(), vec_iter->x.end(),
                       vec_iter->x.begin(),
                       [&x_offset](auto &c) { return c + x_offset; });
        std::transform(vec_iter->y.begin(), vec_iter->y.end(),
                       vec_iter->y.begin(),
                       [&y_offset](auto &c) { return c + y_offset; });
      }
    }
  }
  return EXIT_SUCCESS;
}

// take referred structure and transform it

int transform_aref_map(std::map<unsigned int, std::vector<co_ord>> &target,
                       gdsAREF &aref_info)
{
  // Take original cell
  std::map<unsigned int, std::vector<co_ord>> aref_map = target;
  // Reflect, scale, rotate, offset the cell.
  transform_polygon_map(aref_map, aref_info.reflection, aref_info.scale,
                        aref_info.angle, aref_info.xCor, aref_info.yCor);
  repeat_polygon_map(aref_map, aref_info);
  target.clear();
  target = aref_map;
  return EXIT_SUCCESS;
}

void repeat_polygon_map(std::map<unsigned int, std::vector<co_ord>> &target,
                        gdsAREF &aref_info)
{
  // pull aref into own map
  std::map<unsigned int, std::vector<co_ord>> repeated_map;
  // calculate offset;
  int delta_row[2] = {(int)round((double)(aref_info.xCorRow - aref_info.xCor) /
                                 aref_info.colCnt),
                      (int)round((double)(aref_info.yCorRow - aref_info.yCor) /
                                 aref_info.colCnt)};
  int delta_col[2] = {(int)round((double)(aref_info.xCorCol - aref_info.xCor) /
                                 aref_info.rowCnt),
                      (int)round((double)(aref_info.yCorCol - aref_info.yCor) /
                                 aref_info.rowCnt)};
  // perform repeated placement , each layer, each vector, offset and append
  for (auto layer_it = target.begin(); layer_it != target.end(); layer_it++) {
    for (auto polygon_it = layer_it->second.begin();
         polygon_it != layer_it->second.end(); polygon_it++) {
      for (int i = 1; i <= aref_info.colCnt; i++) {
        for (int j = 1; j <= aref_info.rowCnt; j++) {
          co_ord shifted_polygon = *polygon_it;
          std::transform(shifted_polygon.x.begin(), shifted_polygon.x.end(),
                         shifted_polygon.x.begin(),
                         [&i, &j, &delta_row, &delta_col](auto &c) {
                           return c + delta_row[0] * i + delta_col[0] * j;
                         });
          std::transform(shifted_polygon.y.begin(), shifted_polygon.y.end(),
                         shifted_polygon.y.begin(),
                         [&i, &j, &delta_row, &delta_col](auto &c) {
                           return c + delta_row[1] * i + delta_col[1] * j;
                         });
          insert_or_append_to_polymap(repeated_map, layer_it->first,
                                      shifted_polygon);
        }
      }
    }
  }
  target.clear();
  target = repeated_map;
}

void insert_or_append_to_polymap(
    std::map<unsigned int, std::vector<co_ord>> &target_map, int key,
    const co_ord &polygon)
{
  if (target_map.count(key) > 0) {
    target_map[key].push_back(polygon);
  } else {
    vector<co_ord> polygons = {polygon};
    target_map.insert(pair<unsigned int, vector<co_ord>>(key, polygons));
  }
}
/**
 *  @brief Rotates the specified vector of points
 *  @param cx x_offset
 *  @param cy y_offset
 *  @param angle angle to rotate vector by. Angle must be counter-clockwise from
 * x-axis and in degrees.
 */
void rotate_vector(double cx, double cy, double angle,
                   std::vector<POINT> &subject) // POINT &subject
{
  angle = (angle * PI) / ((double)180); // convert angle to radians
  double s = sin(angle);
  double c = cos(angle);
  auto revert_point_to_origin = [&cx, &cy](POINT &n) {
    n.x -= cx;
    n.y -= cy;
  };
  auto rotate_point = [&c, &s](POINT &n) {
    int n_x_orig = n.x;
    n.x = n.x * c - n.y * s;
    n.y = n_x_orig * s + n.y * c;
  };
  auto translate_back = [&cx, &cy](POINT &n) {
    n.x = n.x + cx;
    n.y = n.y + cy;
  };
  for_each(subject.begin(), subject.end(),
           revert_point_to_origin); // translate points back to origins
  for_each(subject.begin(), subject.end(), rotate_point); // rotate points
  for_each(subject.begin(), subject.end(),
           translate_back); // translate points back
}

int path_to_polygon(gdsPATH path_data, co_ord &dest_co_ord)
{
  if (path_data.xCor.size() < 2) {
    throw invalid_argument("Path cannot be one point.");
  }
  co_ord poly_co_ord; // Polygon storage (remember to add to dest)
  ClipperLib::Path subj;
  ClipperLib::ClipperOffset offset;
  ClipperLib::Paths solution;
  auto path_iter_x = path_data.xCor.begin();
  auto path_iter_y = path_data.yCor.begin();
  // Convert to clipper library format ------------
  while (path_iter_x != path_data.xCor.end()) {
    subj << ClipperLib::IntPoint(*path_iter_x, *path_iter_y);
    path_iter_x++;
    path_iter_y++;
  }
  switch (path_data.pathtype) {
  case 0: // flush endcap
    offset.AddPath(subj, ClipperLib::jtMiter, ClipperLib::etOpenButt);
    break;
  case 1: // Half-circle endcap
    offset.AddPath(subj, ClipperLib::jtRound, ClipperLib::etOpenSquare);
    break;
  default: // Half-width extension
    offset.AddPath(subj, ClipperLib::jtMiter, ClipperLib::etOpenRound);
    break;
  }
  offset.Execute(solution, ((double)path_data.width * 0.5));
  auto a = solution.begin();
  auto b = *a;
  auto clipper_point_iterator = b.begin();
  while (clipper_point_iterator != b.end()) {
    poly_co_ord.x.push_back(clipper_point_iterator->X);
    poly_co_ord.y.push_back(clipper_point_iterator->Y);
    clipper_point_iterator++;
  }
  dest_co_ord = poly_co_ord;
  return EXIT_SUCCESS;
}

// Runs through all polygons of shape and
// calculates where in the layer the intercepts lie.
int section_data::calculate_intercepts()
{
  auto current_polygon = polygons.begin();
  while (current_polygon != polygons.end()) // each layer
  {
    auto vect = current_polygon->second;
    auto vect_iter = vect.begin();
    while (vect_iter != vect.end()) // each vector of polygons
    {
      analyze_polygon(current_polygon->first, *vect_iter);
      vect_iter++;
    }
    // No intercepts were detected, check if point lies within any polygon.
    // If it does, create two intercepts, one at each point
    // This is necessary to show layers even when you are working inside.
    if(!layer_intercepts.count(current_polygon->first))
      {
          bool is_inside = false;
          Point p;
          p.x = x1;
          p.y = y1;
          auto v_end = vect.end();
          for(auto v_it = vect.begin(); v_it!= v_end; v_it++)
          {
            vector<Point> v_it_as_Point;
            co_ord_to_point_v(*v_it, v_it_as_Point);
            if(Is_Inside(p, v_it_as_Point)==true)
              is_inside=true;
          }
          if (is_inside==true)
          {
            cout << "Cross-section detected inside polygon. "<<endl;
            cout << "Inserting boundary on layer "
            << current_polygon->first <<"."  << endl;
            vector<intercept_data> false_vect;
            intercept_data false_intercept;
            false_intercept.x=x1;
            false_intercept.y=y1;
            false_intercept.dist=0;
            false_vect.push_back(false_intercept);
            layer_intercepts.insert({current_polygon->first,false_vect});
          }
      }
    current_polygon++;
  }
  return EXIT_SUCCESS;
}

// Analyze each polygon for line-segments to check against cross-section line.
int section_data::analyze_polygon(int layer, const co_ord &line_data)
{
  vector<int> xvec = line_data.x;
  vector<int> yvec = line_data.y;
  uint count = 0;
  uint veclen = xvec.size() - 1;
  int vec_beg[2], vec_end[2]; // xy
  while (count < veclen) {
    vec_beg[0] = xvec[count];
    vec_beg[1] = yvec[count];
    vec_end[0] = xvec[count + 1];
    vec_end[1] = yvec[count + 1];
    count++;
    analyze_line_seg(vec_beg, vec_end, layer);
  }
  return EXIT_SUCCESS;
}

// Determines how to calculate cross_section of line_segment
int section_data::analyze_line_seg(int line_start[2], int line_end[2],
                                   int layer) // axis_type
{
  // enforce sorting
  int line_type = calibrate_line(line_start, line_end);
  int section_type = set_section_type(line_type, axis_type);
  int ret;
  // map<int, vector<double>> layer_intercepts;          // key = layer number
  switch (section_type) {
  case 1: // Horizontal-horizontal
    ret = slice_horiz_horiz(line_start, line_end, layer);
    break;
  case 2: // Horizontal-angled
    ret = slice_horiz_angled(line_start, line_end, layer);
    break;
  case 3: // Horizontal-vertical
    ret = slice_horiz_vertical(line_start, line_end, layer);
    break;
  case 4: // Angled-horizontal
    ret = slice_angled_horiz(line_start, line_end, layer);
    break;
  case 5: // Angled-angled
    ret = slice_angled_angled(line_start, line_end, layer);
    break;
  case 6: // Angled-vertical
    ret = slice_angled_vertical(line_start, line_end, layer);
    break;
  case 7: // Vertical-horizontal
    ret = slice_vertical_horiz(line_start, line_end, layer);
    break;
  case 8: // Vertical-angled
    ret = slice_vertical_angled(line_start, line_end, layer);
    break;
  case 9: // Vertical-vertical
    ret = slice_vert_vert(line_start, line_end, layer);
    break;
  case 99:
    cout << "Error analyzing line_segment. Unrecognized cross-section "
            "interaction."
         << endl;
    ret = EXIT_FAILURE;
    break;
  default:
    cout << "Error analyzing line_segment. Unhandled state." << endl;
    ret = EXIT_FAILURE;
    break;
  }
  return ret;
}

// Returns the integer representing type of cross-section
// First:  Polygon   line
// Second: Intercept line
int set_section_type(int &line_type, int &section_type)
{
  if ((line_type == 1) && (section_type == 1)) {
    return 1; // horizontal horizontal
  } else if ((line_type == 1) && (section_type == 2)) {
    return 2; // horizontal angled
  } else if ((line_type == 1) && (section_type == 3)) {
    return 3; // horizontal-vertical
  } else if ((line_type == 2) && (section_type == 1)) {
    return 4; // angled-horizontal
  } else if ((line_type == 2) && (section_type == 2)) {
    return 5; // angled-angled
  } else if ((line_type == 2) && (section_type == 3)) {
    return 6; // angled-vertical
  } else if ((line_type == 3) && (section_type == 1)) {
    return 7; // vertical-horizontal
  } else if ((line_type == 3) && (section_type == 2)) {
    return 8; // vertical-angled
  } else if ((line_type == 3) && (section_type == 3)) {
    return 9; // vertical-vertical
  } else
    return 99; // error state
}

// adds intercept point to vectory of intercepts at that layer
int section_data::push_intercept(int &layer, intercept_data intercept)
{
  // map<int, vector<double>> layer_intercepts
  // key = layer number
  layer_intercepts[layer].push_back(intercept);
  return EXIT_SUCCESS;
}

// Place all intercepts in ascending order
// Add boundary values if they are not intercepts
int section_data::unique_sort_intercepts()
{
  for (auto const &[key, val] : layer_intercepts) {
    auto vec = val;
    // add beginning and end points [boundary zone]
    intercept_data boxing_boundary;
    boxing_boundary.x = x1;
    boxing_boundary.y = y1;
    boxing_boundary.dist = 0;
    vec.push_back(boxing_boundary);
    boxing_boundary.x = x2;
    boxing_boundary.y = y2;
    boxing_boundary.dist = xsec_distance;
    vec.push_back(boxing_boundary);
    sort(vec.begin(), vec.end()); // must sort first
    vec.erase(unique(vec.begin(), vec.end()), vec.end());
    auto [it, ins] = layer_intercepts.insert_or_assign(key, vec);
    assert(ins == false);
  }
  return EXIT_SUCCESS;
}

// Prints all intercepts of section_data object.
void section_data::print_intercepts()
{
  for (auto const &[key, val] : layer_intercepts) {
    auto intercept_iterator = val.begin();
    while (intercept_iterator != val.end()) {
      if ((intercept_iterator->dist == 0) ||
          (intercept_iterator->dist == xsec_distance)) {
        cout << "Layer [" << key << "], XY=[" << intercept_iterator->x << ","
             << intercept_iterator->y << "], "
             << "Distance [" << intercept_iterator->dist << "] (BOUNDARY)"
             << endl;
      } else {
        cout << "Layer [" << key << "], XY=[" << intercept_iterator->x << ","
             << intercept_iterator->y << "], "
             << "Distance [" << intercept_iterator->dist << "]" << endl;
      }
      intercept_iterator++;
    }
  }
}

// Intercept calculations for case 1
int section_data::slice_horiz_horiz(int line_start[2], int line_end[2],
                                    int &layer)
{
  int xa = line_start[0];
  int xb = line_end[0];
  intercept_data tempy;
  if (line_start[1] == y1) // Lines could interact only if on same height
  {
    if ((xa <= x2) || (x1 < xb)) // Lines have some interaction
    {
      if (xa == x1) {
        tempy.x = x1;
        tempy.y = y1;
        tempy.dist = 0;
        push_intercept(layer, tempy);
      }
      if (xa == x2) {
        tempy.x = x2;
        tempy.y = y2;
        tempy.dist = x2 - x1;
        push_intercept(layer, tempy);
      }
      if (xb == x1) {
        tempy.x = x1;
        tempy.y = y1;
        tempy.dist = 0;
        push_intercept(layer, tempy);
      }
      if (xb == x2) {
        tempy.x = x2;
        tempy.y = y2;
        tempy.dist = x2 - x1;
        push_intercept(layer, tempy);
      }
      if ((xa > x1) && (xa < x2)) {
        tempy.x = xa;
        tempy.y = y2;
        tempy.dist = xa - x1;
        push_intercept(layer, tempy);
      }
      if ((xb > x1) && (xb < x2)) {
        tempy.x = xb;
        tempy.y = y2;
        tempy.dist = xb - x1;
        push_intercept(layer, tempy);
      }
      if ((x1 > xa) && (x1 < xb)) {
        tempy.x = x1;
        tempy.y = y2;
        tempy.dist = 0;
        push_intercept(layer, tempy);
      }
      if ((x2 > xa) && (x2 < xb)) {
        tempy.x = x2;
        tempy.y = y2;
        tempy.dist = x2 - x1;
        push_intercept(layer, tempy);
      }
    }
  }
  return EXIT_SUCCESS;
}

// Intercept calculations for case 2
int section_data::slice_horiz_angled(int line_start[2], int line_end[2],
                                     int &layer)
{
  double xa = (double)line_start[0];
  double xb = (double)line_end[0];
  double ya = (double)line_start[1];
  intercept_data tempy;
  // Solve linear equation
  double c =
      (double)y1 + (((double)x1 * (double)y1) - ((double)x1 * (double)y2)) /
                       ((double)x2 - (double)x1);
  double m = (((double)y2 - (double)y1) / ((double)x2 - (double)x1));
  // y = mx + c    -    Solve for x using y value of horizontal line
  // x = (ya-c)/m  -    Does x lie within boundary of [xa,xb]
  double x_intercept = round(((double)ya - c) / m);
  // if it does, calculate the distance from x1 [x1 always origin, x1 always LE
  // x2]
  if ((x_intercept >= xa) && (x_intercept <= xb)) {
    double distance = round(sqrt((x_intercept - x1) * (x_intercept - x1) +
                                 (line_start[1] - y1) * (line_start[1] - y1)));
    tempy.x = (int)x_intercept;
    tempy.y = (int)ya;
    tempy.dist = (int)distance;
    push_intercept(layer, tempy); // round to nearest database unit (nm)
  }
  return EXIT_SUCCESS;
}

// Intercept calculations for case 3
int section_data::slice_horiz_vertical(int line_start[2], int line_end[2],
                                       int &layer)
{
  int xa = line_start[0];
  int xb = line_end[0];
  int ya = line_start[1];
  intercept_data tempy;
  if ((x1 >= xa) && (x1 <= xb) && (ya >= y1) && (ya <= y2)) {
    tempy.x = x1;
    tempy.y = ya;
    tempy.dist = ya - y1;
    push_intercept(layer, tempy);
  }
  return EXIT_SUCCESS;
}

// Intercept calculations for case 4
int section_data::slice_angled_horiz(int line_start[2], int line_end[2],
                                     int &layer)
{
  double xa = (double)line_start[0];
  double xb = (double)line_end[0];
  double ya = (double)line_start[1];
  double yb = (double)line_end[1];
  intercept_data tempy;
  // Solve linear equation
  double c = ya + ((xa * ya) - (xa * yb)) / (xb - xa);
  double m = ((yb - ya) / (xb - xa));
  // y = mx + c    -    Solve for x using y value of horizontal line
  // x = (y1-c)/m  -    Does x lie within boundary of [xa,xb],
  //                   as well as within cross-section constraint
  double x_intercept = round(((double)y1 - c) / m);
  // if it does, calculate the distance from x1 [x1 always origin, x1 always LE
  // x2]
  if ((x_intercept >= (double)x1) && (x_intercept <= (double)x2) &&
      (x_intercept <= xb) && (x_intercept >= xa)) {
    if (((ya < yb) && ((double)y1 >= ya) && ((double)y1 <= yb)) ||
        ((ya > yb) && ((double)y1 >= yb) && (double)y1 <= ya)) {
      int distance = (int)x_intercept - x1;
      tempy.x = (int)x_intercept;
      tempy.y = y1;
      tempy.dist = distance;
      push_intercept(layer, tempy); // round to nearest database unit (nm)
    }
  }
  return EXIT_SUCCESS;
}

// Intercept calculations for case 5
int section_data::slice_angled_angled(int line_start[2], int line_end[2],
                                      int &layer)
{
  double x_a = (double)line_start[0];
  double y_a = (double)line_start[1];
  double x_b = (double)line_end[0];
  double y_b = (double)line_end[1];
  intercept_data tempy;
  double m_poly = (y_b - y_a) / (x_b - x_a);
  double m_cx = ((double)y2 - (double)y1) / ((double)x2 - (double)x1);
  double x_intercept = round(
      (m_poly * x_a - m_cx * (double)x1 + (double)y1 - y_a) / (m_poly - m_cx));
  double y_intercept = round(m_poly * (x_intercept - x_a) + y_a);
  if ((x_intercept >= x_a) && (x_intercept >= x1) && (x_intercept <= x_b) &&
      (x_intercept <= x2)) {
    if (((y_a < y_b) && (y_intercept >= y_a) && (y_intercept <= y_b)) ||
        ((y_a > y_b) && (y_intercept <= y_a) && (y_intercept >= y_b))) {
      if (((y1 > y2) && (y_intercept >= y2) && (y_intercept <= y1)) ||
          ((y1 < y2) && (y_intercept >= y1) && (y_intercept <= y2))) {
        tempy.x = (int)x_intercept;
        tempy.y = (int)y_intercept;
        tempy.dist = (int)round(
            sqrt((x_intercept - (double)x1) * (x_intercept - (double)x1) +
                 (y_intercept - (double)y1) * (y_intercept - (double)y1)));
        push_intercept(layer, tempy); // round to nearest database unit (nm)
      }
    }
  }
  return EXIT_SUCCESS;
}

// Intercept calculations for case 6
int section_data::slice_angled_vertical(int line_start[2], int line_end[2],
                                        int &layer)
{
  double xa = (double)line_start[0];
  double xb = (double)line_end[0];
  if (((double)x1 >= xa) && ((double)x1 <= xb)) {
    double ya = (double)line_start[1];
    double yb = (double)line_end[1];
    double m_poly = (yb - ya) / (xb - xa);
    double y_int = round(m_poly * (double)x1 - m_poly * xa + ya);
    if (((ya > yb) && (y_int >= yb) && (y_int <= ya)) ||
        ((ya < yb) && (y_int >= ya) && (y_int <= yb))) {
      intercept_data tempy;
      tempy.x = x1;
      tempy.y = (int)y_int;
      tempy.dist = tempy.y - y1;
      push_intercept(layer, tempy); // round to nearest database unit (nm)
    }
  }
  return EXIT_SUCCESS;
}

// Intercept calculations for case 7
int section_data::slice_vertical_horiz(int line_start[2], int line_end[2],
                                       int &layer)
{
  int xa = line_start[0];
  int ya = line_start[1];
  int yb = line_end[1];
  if ((xa >= x1) && (xa <= x2) && (y1 >= ya) && (y1 <= yb)) {
    intercept_data tempy;
    tempy.x = xa;
    tempy.y = y1;
    tempy.dist = xa - x1;
    push_intercept(layer, tempy);
  }
  return EXIT_SUCCESS;
}

// Intercept calculations for case 8
int section_data::slice_vertical_angled(int line_start[2], int line_end[2],
                                        int &layer)
{
  double xa = (double)line_start[0];
  double ya = (double)line_start[1];
  double yb = (double)line_end[1];
  if ((xa >= (double)x1) && (xa <= (double)x2)) {
    double m = ((double)y2 - (double)y1) / ((double)x2 - (double)x1);
    double y_int = round(m * (xa - (double)x1) + (double)y1);
    if ((((double)y1 > (double)y2) && (y_int > (double)y2) &&
         (y_int < (double)y1)) ||
        (((double)y1 < (double)y2) && (y_int > (double)y1) &&
         y_int < (double)y2)) {
      if (((ya > yb) && (y_int >= yb) && (y_int <= ya)) ||
          ((ya < yb) && (y_int >= ya) && (y_int <= yb))) {
        intercept_data tempy;
        tempy.x = (int)xa;
        tempy.y = (int)y_int;
        tempy.dist =
            (int)round((sqrt((xa - (double)x1) * (xa - (double)x1) +
                             (y_int - (double)y1) * (y_int - (double)y1))));
        push_intercept(layer, tempy);
      }
    }
  }
  return EXIT_SUCCESS;
}

// Intercept calculations for case 9
int section_data::slice_vert_vert(int line_start[2], int line_end[2],
                                  int &layer)
{
  int ya = line_start[1];
  int yb = line_end[1];
  intercept_data tempy;
  if (line_start[0] == x1) // Lines could interact only if at same x
  {
    if ((ya <= y2) || (y1 < yb)) // Lines have some interaction
    {
      if (ya == y1) {
        tempy.x = x1;
        tempy.y = ya;
        tempy.dist = 0;
        push_intercept(layer, tempy);
      }
      if (ya == y2) {
        tempy.x = x1;
        tempy.y = ya;
        tempy.dist = ya - y1; // y1 always le y2 for vert-vert
        push_intercept(layer, tempy);
      }
      if (yb == y1) {
        tempy.x = x1;
        tempy.y = yb;
        tempy.dist = 0;
        push_intercept(layer, tempy);
      }
      if (yb == y2) {
        tempy.x = x1;
        tempy.y = yb;
        tempy.dist = yb - y1;
        push_intercept(layer, tempy);
      }
      if ((ya > y1) && (ya < y2)) {
        tempy.x = x1;
        tempy.y = ya;
        tempy.dist = ya - y1;
        push_intercept(layer, tempy);
      }
      if ((yb > y1) && (yb < y2)) {
        tempy.x = x1;
        tempy.y = yb;
        tempy.dist = yb - y1;
        push_intercept(layer, tempy);
      }
      if ((y1 > ya) && (y1 < yb)) {
        tempy.x = x1;
        tempy.y = y1;
        tempy.dist = 0; // y1 is the starting point
        push_intercept(layer, tempy);
      }
      if ((y2 > ya) && (y2 < yb)) {
        tempy.x = x1;
        tempy.y = y2;
        tempy.dist = y2 - y1;
        push_intercept(layer, tempy);
      }
    }
  }
  return EXIT_SUCCESS;
}

int section_data::generate_blocks()
{
  std::map<int, std::vector<intercept_data>>::iterator layer_iter =
      layer_intercepts.begin();
  auto end = layer_intercepts.end();
  int block_id = 0;
  while (layer_iter !=
         end) // =================== Layers Loop ======================]
  {
    unsigned int key = layer_iter->first;
    auto it = layer_iter->second.begin();
    auto it_end = --layer_iter->second.end();
    while (it !=
           it_end) // --------------- Sectioned Segments Loop ---------------<
    {
      int startx = it->x;
      int starty = it->y;
      int start_dist = it->dist;
      it++;
      int endx = it->x;
      int endy = it->y;
      int end_dist = it->dist;
      Point checkme;
      checkme.x = (int)round((startx + endx) * 0.5);
      checkme.y = (int)round((starty + endy) * 0.5);
      building_blocks my_segment;
      my_segment.start_distance = start_dist;
      my_segment.end_distance = end_dist;
      my_segment.primary = false;
      vector<co_ord>::iterator current_polygon = polygons[key].begin();
      while ( current_polygon != polygons[key].end()) // Check midpoint against every polygon on layer
      {
        // while not end of current_polygon->x push current polygon to converted
        // polygon
        vector<Point> converted_polygon;
        auto itx = current_polygon->x.begin();
        auto ity = current_polygon->y.begin();
        while (itx != current_polygon->x.end()) {
          Point current_point;
          current_point.x = *itx;
          current_point.y = *ity;
          converted_polygon.push_back(current_point);
          itx++;
          ity++;
        }
        if (Is_Inside(checkme, converted_polygon) == true) {
          my_segment.primary = true;
        }
        current_polygon++;
      }
      // By this stage if the point is in a polygon, primary is true.
      // section_blocks[key].push_back(my_segment);
      auto [it, ins] =
          section_blocks[key].insert_or_assign(block_id, my_segment);
      block_id++;
    } // ------------------ End Sectioned Segment Loop  ---------<
    layer_iter++;
  } // ==================== End Layers Loop ===================]
  return EXIT_SUCCESS;
}

// Takes section blocks of a layer and searches for similar adjacent blocks.
// Changes the first adjacent block to size of both. Then removes the second
// adjacent.
// TODO: Check that improve_blocks() is functioning correctly.
int section_data::improve_blocks()
{
  auto layer_it = section_blocks.begin();
  auto end_layer_it = section_blocks.end();
  while (layer_it != end_layer_it) {
    auto block_it = layer_it->second.begin();
    auto block_end = layer_it->second.end()--;
    while (block_it != block_end) {
      bool needed_update = false;
      int end1 = block_it->second.end_distance;
      bool type1 = block_it->second.primary;
      block_it++;
      bool type2 = block_it->second.primary;
      int start2 = block_it->second.start_distance;
      int end2 = block_it->second.end_distance;
      if ((end1 == start2) && (type1 == type2)) {
        block_it--;
        block_it->second.end_distance = end2;
        needed_update = true;
        block_it++;
      }
      if (needed_update == true)
        block_it = section_blocks[layer_it->first].erase(block_it);
    }
    layer_it++;
  }
  return EXIT_SUCCESS;
}

int section_data::pull_section_blocks(
    map<int, map<int, building_blocks>> &target_destination)
{
  target_destination = section_blocks;
  return EXIT_SUCCESS;
}

int section_data::get_sec_dist()
{
  return xsec_distance;
}

int section_data::pull_used_layers(vector<int> &destination_vector)
{
  for (auto it = polygons.begin(); it != polygons.end(); ++it) {
    destination_vector.push_back(it->first);
  }
  return EXIT_SUCCESS;
}

int co_ord_to_point_v(const co_ord &co,
                        std::vector<Point> &p_v)
{
  auto itx = co.x.begin();
  auto ity = co.y.begin();
  while (itx != co.x.end()) {
    Point current_point;
    current_point.x = *itx;
    current_point.y = *ity;
    p_v.push_back(current_point);
    itx++;
    ity++;
  }
  return EXIT_SUCCESS;
}