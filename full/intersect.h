#ifndef INTERSECT_H_
#define INTERSECT_H_

struct XY
{
   double x, y;
};

// Check if a line segment from u to v would intersect blocker of a particular
// radius.  Returns true if so.
bool IsBlocked(const XY &u, const XY &v, const XY &blocker, double radius);

#endif  // INTERSECT_H_
