#include"intersect.h"

#include<algorithm>
#include<cmath>

namespace {

// Check if a potentially blocking obstacle 'b' with radius 'r'
// is within bounding box enclosed by 'u' and 'v'.
//
// This check is applied before testing IsWithinRadius .  This is because
// IsWithinRadius measures distance to an infinite line, but we just want
// distance to a specific segment.
//
// It's conceivable to create a case where the combination of these
// two functions yields a false positive, by placing the blocking
// obstacle behind 'v', with 'v' just inside the bounding box but just
// outside of radius of 'b'.  This doesn't happen for musicians due to
// the 10m radius limit, and doesn't happen for pillars because none
// of the pillars are placed near the stage.
static bool IsWithinBoundingBox(const XY &u, const XY &v, const XY &b, double r)
{
   return b.x >= std::min(u.x, v.x) - r &&
          b.x <= std::max(u.x, v.x) + r &&
          b.y >= std::min(u.y, v.y) - r &&
          b.y <= std::max(u.y, v.y) + r;
}

// Check if a potentially blocking obstacle 'b' with radius 'r'
// will block 'v' with respect to 'u'.
static bool IsWithinRadius(const XY &u, const XY &v, const XY &b, double r)
{
   #if 0
      const double dx = v.x - u.x;
      const double dy = v.y - u.y;
      const double n = fabs(dx * (u.y - b.y) - dy * (u.x - b.x));
      return n / hypot(dx, dy) < r;
   #else
      const double dx = v.x - u.x;
      const double dy = v.y - u.y;
      const double n = dx * (u.y - b.y) - dy * (u.x - b.x);
      return n * n < r * r * (dx * dx + dy * dy);
   #endif
}

}  // namespace

bool IsBlocked(const XY &u, const XY &v, const XY &blocker, double radius)
{
   return IsWithinBoundingBox(u, v, blocker, radius) &&
          IsWithinRadius(u, v, blocker, radius);
}
