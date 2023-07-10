#include"solution.h"

#include<chrono>
#include<cmath>

#include"grid.h"

#ifdef BENCHMARK
#include<iostream>
#endif

namespace {

// Sample size for fast score estimation.
static constexpr int kSampleSize = 200;

// Run for this much time before giving up.
static constexpr std::chrono::seconds kRunDuration{20};

// Maximum number of random positions to consider.
static constexpr int kRandomMoveLimit = 64;

// Minimum radius from musician to edge or another musician.
static constexpr double kMargin = 10;

// Minimum radius for blocking.
static constexpr double kBlockingRadius = 5;

// Return this score in event of error.
static constexpr double kErrorScore = -1e9;

static double DistanceSquared(const XY &a, const XY &b)
{
   const double dx = a.x - b.x;
   const double dy = a.y - b.y;
   return dx * dx + dy * dy;
}

// Check if potentially blocking musician 'b' is within bounding box
// enclosed by 'u' and 'v'.
//
// This check is applied before testing IsBlocked below.  This is because
// IsBlocked measures distance to an infinite line, but we just want
// distance to a specific segment.
//
// This is not completely accurate in that a point could be within
// that corner of the rectangle that is 2*sqrt(5**2 + 5**2) away and
// still fall on the same line, but contest spec says musicians can
// not be placed within radius 10 of each other, and 10 is greater
// than that corner distance, so we should be fine.
//
// That said, the scores we computed seem to differ from contest
// output by few parts per million, always in the negative direction
// (our score is less than what the contest environment computed).  I
// can't tell if this is just floating point precision or not.  Getting
// them to match exactly doesn't seem to be worth the effort.
static bool IsWithinBoundingBox(const XY &u, const XY &v, const XY &b)
{
   return b.x >= std::min(u.x, v.x) - kBlockingRadius &&
          b.x <= std::max(u.x, v.x) + kBlockingRadius &&
          b.y >= std::min(u.y, v.y) - kBlockingRadius &&
          b.y <= std::max(u.y, v.y) + kBlockingRadius;
}

// Check musician 'b' blocks 'a' with respect to audience 'p'.
static bool IsBlocked(const XY &p, const XY &a, const XY &b)
{
   #if 0
      const double dx = a.x - p.x;
      const double dy = a.y - p.y;
      const double n = fabs(dx * (p.y - b.y) - dy * (p.x - b.x));
      return n / hypot(dx, dy) <= kBlockingRadius;
   #else
      const double dx = a.x - p.x;
      const double dy = a.y - p.y;
      const double n = dx * (p.y - b.y) - dy * (p.x - b.x);
      return n * n <= kBlockingRadius * kBlockingRadius * (dx * dx + dy * dy);
   #endif
}

// Check if any musician blocks 'a' with respect to audience 'p'.
static bool IsAnyBlocking(const std::vector<XY> &placements,
                          int a,
                          const XY &p)
{
   for(int b = 0; b < static_cast<int>(placements.size()); b++)
   {
      if( b == a )
         continue;
      if( IsWithinBoundingBox(p, placements[a], placements[b]) &&
          IsBlocked(p, placements[a], placements[b]) )
      {
         return true;
      }
   }
   return false;
}

static double ComputeLimitedScore(const Problem &problem,
                                  const std::vector<XY> &placements,
                                  int attendee_count)
{
   #ifdef BENCHMARK
      const auto start_time = std::chrono::steady_clock::now();
   #endif

   const int limit = std::min(attendee_count,
                              static_cast<int>(problem.attendees().size()));
   double score = 0;

   const XY stage_min =
   {
      problem.stage_bottom_left().x + kMargin,
      problem.stage_bottom_left().y + kMargin
   };
   const XY stage_max =
   {
      problem.stage_bottom_left().x + problem.stage_size().x - kMargin,
      problem.stage_bottom_left().y + problem.stage_size().y - kMargin
   };
   for(int i = 0; i < static_cast<int>(problem.musicians().size()); i++)
   {
      const XY &musician = placements[i];
      if( musician.x < stage_min.x || musician.x > stage_max.x ||
          musician.y < stage_min.y || musician.y > stage_max.y )
      {
         fprintf(stderr, "Musician %d is not on stage\n", i);
         return kErrorScore;
      }

      const int instrument = problem.musicians()[i];
      for(int j = 0; j < limit; j++)
      {
         const Problem::Attendee &a = problem.attendees()[j];
         if( IsAnyBlocking(placements, i, a.position) )
            continue;

         const double d2 = DistanceSquared(musician, a.position);
         if( d2 < 100 )
         {
            fprintf(stderr, "Musician %d collides with audience\n", i);
            return kErrorScore;
         }

         score += std::ceil(1e6 * a.tastes[instrument] / d2);
      }
   }

   #ifdef BENCHMARK
      const std::chrono::duration<double> elapsed =
         std::chrono::steady_clock::now() - start_time;
      std::cerr << "ComputeLimitedScore time: " << elapsed.count() << "\n";
   #endif
   return score;
}

}  // namespace

double ComputeScore(const Problem &problem, const std::vector<XY> &placements)
{
   return ComputeLimitedScore(
      problem, placements, static_cast<int>(problem.attendees().size()));
}

void Solve(const Problem &problem, Solution *solution)
{
   // Set initial placement such that none of the musicians overlap.
   solution->placements.resize(problem.musicians().size());
   Grid grid(problem);

   for(int i = 0; i < static_cast<int>(problem.musicians().size()); i++)
   {
      const std::pair<int, int> &p = grid.points()[i];
      solution->placements[i] = grid.ToXY(p.first, p.second);
      grid.Set(p.first, p.second, 1);
   }

   std::random_device rd;
   std::default_random_engine rng(rd());
   std::uniform_int_distribution select_attendee(
      0,
      std::min(static_cast<int>(problem.attendees().size()), kSampleSize) - 1);
   std::uniform_int_distribution select_instrument(
      0, static_cast<int>(problem.instruments().size()) - 1);

   double best_score =
      ComputeLimitedScore(problem, solution->placements, kSampleSize);
   const int random_move_limit =
      std::min(static_cast<int>(grid.points().size()), kRandomMoveLimit);

   // Try random movements for a fixed amount of time.
   const auto start_time = std::chrono::steady_clock::now();
   while( std::chrono::steady_clock::now() - start_time < kRunDuration )
   {
      // Select a random attendee near the top.
      const Problem::Attendee &a = problem.attendees()[select_attendee(rng)];

      // Select an instrument to improve upon.
      int instrument = select_instrument(rng);
      double taste = a.tastes[instrument];
      for(int i = 0; i < 4; i++)
      {
         const int j = select_instrument(rng);
         const double t = a.tastes[j];
         if( fabs(taste) < fabs(t) )
         {
            taste = t;
            instrument = j;
         }
      }

      // Move all musicians playing that instrument.
      for(int m = 0;
          m < static_cast<int>(problem.musicians().size()) &&
             std::chrono::steady_clock::now() - start_time < kRunDuration;
          m++)
      {
         if( problem.musicians()[m] != instrument )
            continue;
         grid.ShufflePoints();
         double d2 = DistanceSquared(a.position, solution->placements[m]);
         int tested_points = 0;
         for(const auto &q : grid.points())
         {
            // Select a new empty spot.
            if( grid.Get(q.first, q.second) != 0 )
               continue;

            // If this is a preferred instrument, we want to make sure
            // that the new spot is not further away because it would
            // decrease points.  Similarly for a hated instrument, we
            // want to make sure that we are not moving it closer.
            const XY new_xy = grid.ToXY(q.first, q.second);
            const double new_d2 = DistanceSquared(a.position, new_xy);
            if( (taste > 0 && new_d2 > d2) || (taste < 0 && new_d2 < d2) )
               continue;

            // Apply this movement.
            const XY old_xy = solution->placements[m];
            solution->placements[m] = new_xy;
            const double new_score =
               ComputeLimitedScore(problem, solution->placements, kSampleSize);
            if( best_score < new_score )
            {
               // Good move, keep it.
               best_score = new_score;
               d2 = new_d2;
               const std::pair<int, int> p = grid.FromXY(old_xy);
               grid.Set(p.first, p.second, 0);
               grid.Set(q.first, q.second, 1);
            }
            else
            {
               // Bad move, undo.
               solution->placements[m] = old_xy;
            }

            if( ++tested_points > random_move_limit )
               break;
         }
      }
   }

   solution->score = ComputeScore(problem, solution->placements);
}
