#ifndef SOLUTION_H_
#define SOLUTION_H_

#include<array>
#include<vector>
#include"problem.h"

struct Solution
{
   // Solution output.
   std::vector<XY> placements;
   std::vector<double> volumes;

   // Final score.
   double score;

   // Debug info.
   enum CounterIndices
   {
      kInitialIterations,
      kInitialMovements,
      kDanceIterations,
      kDanceMovements,
      kDanceResets,

      kCounterCount
   };
   std::array<int, kCounterCount> counters;
};

// Check if path between attendee and musician is blocked.
bool BlockedLineOfSight(const Problem &problem,
                        const std::vector<XY> &placements,
                        const XY &source,
                        int target_index);

// Compute solution score.
double ComputeScore(const Problem &problem,
                    const std::vector<XY> &placements,
                    const std::vector<double> &volumes);

// Generate solution.
void Solve(const Problem &problem, Solution *solution);

// Upgrade a solution.  Returns false if upgrade failed.
bool UpgradeSolution(const Problem &problem, Solution *solution);

#endif  // SOLUTION_H_
