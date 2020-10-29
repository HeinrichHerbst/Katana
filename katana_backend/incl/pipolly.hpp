/**
 * Author:      H.F. Herbst
 * Origin:      Stellenbosch University
 * For:         IARPA SuperTools Project
 * license:     MIT
 * Description: Function declarations for path-in-polygon problem
 *              Function adapted from
 *              forums.codeguru.com/showthread.php?497679-To-check-if-a-point-is-inside-a-polygon
 */
#ifndef pipoly
#define pipoly
#include <vector>
typedef struct
{
  int x, y;
} Point;

bool Is_Inside(const Point &point, const std::vector<Point> &points_list);
int Is_Left(const Point &p0, const Point &p1, const Point &point);
#endif
