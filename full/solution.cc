#include"solution.h"

#include<algorithm>
#include<array>
#include<chrono>
#include<cmath>
#include<random>

#include"grid.h"
#include"intersect.h"

#ifdef BENCHMARK
#include<iostream>
#endif

namespace {

// Sample size for fast score estimation.
static constexpr int kSampleSize = 150;

// Number of initial iterations for shuffling musicians.
static constexpr int kMaxInitIterationSteps = 100;

// Number of groups to divide at each iteration.
static constexpr int kRandomGroupCount = 3;

// Number of mutations per group.
static constexpr int kMutationCount = 10;

// Number of consecutive no-ops before randomizing all musicians.
static constexpr int kMaxConsecutiveNoOps = 5;

// Run for this much time before giving up.
static constexpr std::chrono::seconds kRunDuration{60};

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

// Check if any musician blocks 'a' with respect to audience 'p'.
static bool BlockedByAnotherMusician(const std::vector<XY> &placements,
                                     int a,
                                     const XY &p)
{
   for(int b = 0; b < static_cast<int>(placements.size()); b++)
   {
      if( b == a )
         continue;
      if( IsBlocked(p, placements[a], placements[b], kBlockingRadius) )
         return true;
   }
   return false;
}

// Compute closeness factor for a single musician 'm'.
static double ClosenessFactor(const Problem &problem,
                              int m,
                              const std::vector<XY> &placements)
{
   if( !problem.UseClosenessExtension() )
      return 1;

   double q = 1;
   for(int i = 0; i < static_cast<int>(problem.musicians().size()); i++)
   {
      if( i == m || problem.musicians()[i] != problem.musicians()[m] )
         continue;
      const double d = hypot(placements[i].x - placements[m].x,
                             placements[i].y - placements[m].y);
      if( d > 0 )
         q += 1 / d;
   }
   return q;
}

// Compute scores using just the top few audiences.
static double ComputeLimitedScore(const Problem &problem,
                                  const std::vector<XY> &placements,
                                  const std::vector<double> &volumes,
                                  int attendee_count)
{
   #ifdef BENCHMARK
      const auto start_time = std::chrono::steady_clock::now();
   #endif

   const int limit = std::min(attendee_count,
                              static_cast<int>(problem.attendees().size()));
   double score = 0;

   for(int i = 0; i < static_cast<int>(problem.musicians().size()); i++)
   {
      const XY &musician = placements[i];
      if( volumes[i] == 0 )
         continue;
      const double q =
         volumes[i] * ClosenessFactor(problem, i, placements);

      const int instrument = problem.musicians()[i];
      for(int j = 0; j < limit; j++)
      {
         const Problem::Attendee &a = problem.attendees()[j];
         if( BlockedLineOfSight(problem, placements, a.position, i) )
            continue;

         score +=
            std::ceil(std::ceil(1e6 * a.tastes[instrument] /
                                DistanceSquared(musician, a.position)) * q);
      }
   }

   #ifdef BENCHMARK
      const std::chrono::duration<double> elapsed =
         std::chrono::steady_clock::now() - start_time;
      std::cerr << "ComputeLimitedScore time: " << elapsed.count() << "\n";
   #endif
   return score;
}

// Move a single musician to a particular (column, row).
static void MoveMusician(
   Grid *grid, std::vector<XY> *placements, int m, int column, int row)
{
   const auto [old_x, old_y] = grid->FromXY((*placements)[m]);
   grid->Set(old_x, old_y, 0);
   grid->Set(column, row, 1);
   (*placements)[m] = grid->ToXY(column, row);
}

// Move musicians in selected group.
static void MoveMusicianGroup(const std::vector<int> &movable_group,
                              int group,
                              Grid *grid,
                              std::vector<XY> *placements)
{
   grid->ShufflePoints();
   int point_index = 0;
   for(int m = 0; m < static_cast<int>(movable_group.size()); m++)
   {
      if( movable_group[m] != group )
         continue;

      // Find next unoccupied point.
      //
      // All problems have spare room, so this should not be an infinite loop.
      while( true )
      {
         point_index = (point_index + 1) % grid->points().size();
         const auto [x, y] = grid->points()[point_index];
         if( grid->Get(x, y) != 0 )
            continue;

         // Apply movement.
         MoveMusician(grid, placements, m, x, y);
         break;
      }
   }
}

// Undo the movements in new_placement and restore them to original_placement.
static void UndoMovements(const std::vector<XY> &original_placement,
                          std::vector<XY> *new_placement,
                          Grid *grid)
{
   for(int m = static_cast<int>(original_placement.size()); m-- > 0;)
   {
      if( original_placement[m].x != (*new_placement)[m].x ||
          original_placement[m].y != (*new_placement)[m].y )
      {
         const auto [x, y] = grid->FromXY(original_placement[m]);
         MoveMusician(grid, new_placement, m, x, y);
      }
   }
}

// Apply movements in new_placement.
static void ApplyMovements(std::vector<XY> *placements,
                           const std::vector<XY> &new_placement,
                           Grid *grid)
{
   for(int m = 0; m < static_cast<int>(new_placement.size()); m++)
   {
      if( (*placements)[m].x != new_placement[m].x ||
          (*placements)[m].y != new_placement[m].y )
      {
         const auto [x, y] = grid->FromXY(new_placement[m]);
         MoveMusician(grid, placements, m, x, y);
      }
   }
}

// Optionally move a single musician by integrating taste forces.
//
// Returns true if movement was made.
static bool IntegrateTasteForces(const Problem &problem,
                                 int m,
                                 std::vector<XY> *placements,
                                 Grid *grid)
{
   double force_x = 0, force_y = 0;

   XY position = (*placements)[m];
   const int instrument = problem.musicians()[m];
   for(const Problem::Attendee &a : problem.attendees())
   {
      const double dx = a.position.x - position.x;
      const double dy = a.position.y - position.y;
      const double scale = a.tastes[instrument] / (dx * dx + dy * dy);
      force_x += scale * dx;
      force_y += scale * dy;
   }

   if( force_x < -1 )
   {
      position.x -= Grid::kCellSize;
   }
   else if( force_x > 1 )
   {
      position.x += Grid::kCellSize;
   }
   if( force_y < -1 )
   {
      position.y -= Grid::kCellSize;
   }
   else if( force_y > 1 )
   {
      position.y += Grid::kCellSize;
   }

   const auto [test_x, test_y] = grid->FromXY(position);
   if( test_x < 0 || test_x >= grid->columns() )
      position.x = (*placements)[m].x;
   if( test_y < 0 || test_y >= grid->rows() )
      position.y = (*placements)[m].y;
   if( position.x == (*placements)[m].x && position.y == (*placements)[m].y )
      return false;

   const auto [grid_x, grid_y] = grid->FromXY(position);
   if( grid->Get(grid_x, grid_y) != 0 )
      return false;

   MoveMusician(grid, placements, m, grid_x, grid_y);
   return true;
}

// Set initial musician positions by integrating forces.
static void SetInitialPositions(const Problem &problem,
                                Solution *solution,
                                Grid *grid,
                                std::default_random_engine &rng,
                                int max_steps)
{
   // Start with random positions and populate grid.
   grid->Reset();
   const int musician_count = static_cast<int>(problem.musicians().size());
   for(int i = 0; i < musician_count; i++)
   {
      const std::pair<int, int> &p = grid->points()[i];
      solution->placements[i] = grid->ToXY(p.first, p.second);
      grid->Set(p.first, p.second, 1);
   }

   // Try integrating forces from audiences.
   std::vector<int> musician_index;
   musician_index.reserve(musician_count);
   for(int i = 0; i < musician_count; i++)
      musician_index.push_back(i);

   for(int i = 0; i < max_steps; i++)
   {
      solution->counters[Solution::kInitialIterations]++;
      std::shuffle(musician_index.begin(), musician_index.end(), rng);

      bool attempted_movement = false;
      for(int m : musician_index)
      {
         if( IntegrateTasteForces(problem, m, &(solution->placements), grid) )
         {
            attempted_movement = true;
            solution->counters[Solution::kInitialMovements]++;
         }
      }

      // Stop when nobody is moving anymore.
      if( !attempted_movement )
         break;
   }
}

// Randomly dance some subset of musicians.
static void RandomDance(const Problem &problem,
                        Solution *solution,
                        Grid *grid,
                        std::default_random_engine &rng)
{
   double best_score = ComputeLimitedScore(problem,
                                           solution->placements,
                                           solution->volumes,
                                           kSampleSize);

   // Mutability state for each musician.
   const int musician_count = static_cast<int>(problem.musicians().size());
   std::vector<int> movable_group(musician_count, 1);

   std::uniform_int_distribution<> group_select(1, kRandomGroupCount);
   std::uniform_int_distribution<> init_steps(0, kMaxInitIterationSteps / 2);

   // Temporary states that are used within the loop, but declared
   // outside the loop to avoid repeated allocations.
   std::vector<XY> new_placement;
   std::array<int, kRandomGroupCount> movable_count;
   std::array<double, kRandomGroupCount> group_best_score;
   std::array<std::vector<XY>, kRandomGroupCount> group_best_placement;

   // Try random movements for a fixed amount of time.
   int consecutive_no_ops = 0;
   for(const auto start_time = std::chrono::steady_clock::now();
       std::chrono::steady_clock::now() - start_time < kRunDuration;
       solution->counters[Solution::kDanceIterations]++)
   {
      // Divide the currently movable musicians into a few groups.
      movable_count.fill(0);
      int empty_groups = kRandomGroupCount;
      for(int i = 0; i < musician_count; i++)
      {
         if( movable_group[i] != 0 )
         {
            const int group = group_select(rng);
            movable_group[i] = group;
            if( movable_count[group - 1] == 0 )
               empty_groups--;
            movable_count[group - 1]++;
         }
      }

      // If any group is empty, it means we have too few movable
      // musicians.  Reset mutability state and try again.
      if( empty_groups > 0 )
      {
         std::fill(movable_group.begin(), movable_group.end(), 1);
         continue;
      }

      // Apply movements to selected musicians in each group.
      new_placement = solution->placements;
      group_best_score.fill(best_score);
      for(int group = 0; group < kRandomGroupCount; group++)
      {
         for(int mutation = 0; mutation < kMutationCount; mutation++)
         {
            MoveMusicianGroup(movable_group, group + 1, grid, &new_placement);
            const double new_score = ComputeLimitedScore(problem,
                                                         new_placement,
                                                         solution->volumes,
                                                         kSampleSize);
            if( group_best_score[group] < new_score )
            {
               group_best_score[group] = new_score;
               group_best_placement[group] = new_placement;
            }

            UndoMovements(solution->placements, &new_placement, grid);
         }
      }

      // Find the best group to keep on mutating.
      int best_group = 0;
      for(int group = 1; group < kRandomGroupCount; group++)
      {
         if( group_best_score[best_group] < group_best_score[group] )
            best_group = group;
      }

      // Apply mutation from best group if they improved upon the score.
      if( best_score < group_best_score[best_group] )
      {
         // Apply mutation from best group.
         ApplyMovements(&(solution->placements),
                        group_best_placement[best_group],
                        grid);
         best_score = group_best_score[best_group];

         // Update stats for what we moved.
         for(int g : movable_group)
         {
            if( g == best_group + 1 )
               solution->counters[Solution::kDanceMovements]++;
         }
      }
      else
      {
         consecutive_no_ops++;
         if( consecutive_no_ops >= kMaxConsecutiveNoOps )
         {
            SetInitialPositions(problem, solution, grid, rng, init_steps(rng));
            solution->counters[Solution::kDanceResets]++;
         }
      }

      // Mark any member not in the best group as immutable.
      // The rationale being that not moving these yielded better
      // results than moving them, so we should keep no not moving
      // them.  We do this even if score from the better group is
      // still no better than existing score.
      for(int m = 0; m < musician_count; m++)
      {
         if( movable_group[m] != best_group + 1 )
            movable_group[m] = 0;
      }
   }
}

// Sanity check solution, returns true if there are no errors.
static bool SanityCheck(const Problem &problem, const Solution &solution)
{
   if( solution.placements.size() != problem.musicians().size() )
   {
      fprintf(stderr, "Bad placements, expected %zu, got %zu\n",
              problem.musicians().size(), solution.placements.size());
      return false;
   }
   if( solution.volumes.size() != problem.musicians().size() )
   {
      fprintf(stderr, "Bad volumes, expected %zu, got %zu\n",
              problem.musicians().size(), solution.volumes.size());
      return false;
   }

   bool status = true;
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
      const XY &musician = solution.placements[i];
      if( musician.x < stage_min.x || musician.x > stage_max.x ||
          musician.y < stage_min.y || musician.y > stage_max.y )
      {
         fprintf(stderr, "Musician %d is not on stage\n", i);
         status = false;
      }
      if( solution.volumes[i] < 0 || solution.volumes[i] > 10 )
      {
         fprintf(stderr, "Volume %d is out of range\n", i);
         status = false;
      }

      for(const Problem::Attendee &a : problem.attendees())
      {
         if( DistanceSquared(musician, a.position) < 100 )
         {
            fprintf(stderr, "Musician %d collides with audience\n", i);
            status = false;
         }
      }

      for(int j = i + 1; j < static_cast<int>(problem.musicians().size()); j++)
      {
         const XY &other = solution.placements[j];
         if( DistanceSquared(musician, other) < kMargin * kMargin )
         {
            fprintf(stderr, "Musician %d collides with %d\n", i, j);
            status = false;
         }
      }
   }
   return status;
}

// Set volumes for each musician while holding positions fixed.
static void AdjustVolumes(const Problem &problem, Solution *solution)
{
   for(int i = 0; i < static_cast<int>(problem.musicians().size()); i++)
   {
      const XY &musician = solution->placements[i];

      // Include maximum volume 10 in adjustment factor, to avoid the second
      // ceil() from dropping precision.  We want to find the impact of a
      // musician at maximum volume before decide whether to mute them.
      const double q = 10 * ClosenessFactor(problem, i, solution->placements);

      const int instrument = problem.musicians()[i];
      double contribution = 0;
      for(const Problem::Attendee &a : problem.attendees())
      {
         if( BlockedLineOfSight(problem, solution->placements, a.position, i) )
            continue;
         contribution +=
            std::ceil(std::ceil(1e6 * a.tastes[instrument] /
                                DistanceSquared(musician, a.position)) * q);
      }

      solution->volumes[i] = contribution < 0 ? 0 : 10;
   }
}

}  // namespace

bool BlockedLineOfSight(const Problem &problem,
                        const std::vector<XY> &placements,
                        const XY &source,
                        int target_index)
{
   return BlockedByAnotherMusician(placements, target_index, source) ||
          problem.BlockedByPillar(source, placements[target_index]);
}

double ComputeScore(const Problem &problem,
                    const std::vector<XY> &placements,
                    const std::vector<double> &volumes)
{
   return ComputeLimitedScore(problem,
                              placements,
                              volumes,
                              static_cast<int>(problem.attendees().size()));
}

void Solve(const Problem &problem, Solution *solution)
{
   solution->counters.fill(0);
   solution->placements.resize(static_cast<int>(problem.musicians().size()));
   solution->volumes.resize(static_cast<int>(problem.musicians().size()));
   std::fill(solution->volumes.begin(), solution->volumes.end(), 10);

   Grid grid(problem);

   std::random_device rd;
   std::default_random_engine rng(rd());
   SetInitialPositions(problem, solution, &grid, rng, kMaxInitIterationSteps);
   RandomDance(problem, solution, &grid, rng);

   solution->score = SanityCheck(problem, *solution)
      ? ComputeScore(problem, solution->placements, solution->volumes)
      : kErrorScore;
}

bool UpgradeSolution(const Problem &problem, Solution *solution)
{
   solution->counters.fill(0);
   solution->score = kErrorScore;

   if( solution->placements.size() != problem.musicians().size() ||
       solution->volumes.size() != problem.musicians().size() )
   {
      fprintf(stderr,
              "Solution does not match problem: expected %zu,%zu, got %zu\n",
              problem.musicians().size(),
              solution->placements.size(),
              solution->volumes.size());
      return false;
   }
   if( !SanityCheck(problem, *solution) )
   {
      fputs("Solution does not pass sanity check\n", stderr);
      return false;
   }
   const double old_score =
      ComputeScore(problem, solution->placements, solution->volumes);
   AdjustVolumes(problem, solution);
   solution->score =
      ComputeScore(problem, solution->placements, solution->volumes);

   fprintf(stderr, "%.0f -> %.0f, change = %+.0f\n",
           old_score, solution->score, solution->score - old_score);
   if( old_score > solution->score )
   {
      fprintf(stderr, "Upgrade failed: old score = %.0f, new score = %.0f\n",
              old_score, solution->score);
      return false;
   }
   return true;
}
