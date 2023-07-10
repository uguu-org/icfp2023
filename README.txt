# Team uguu.org

One person team, using primarily C++.

## TL;DR

Moved musicians randomly and hoped for the best.

## Overview

This year's task continued the welcomed tradition of problems that are succinct to specify but difficult to optimize:

1. Maintain minimum distance between musicians and edges of stage.
2. Optimize distance to maximize or minimize impact.
3. Fine tune placement to block or unblock musician.
4. Bonus extensions include optimize distance between musicians and volume control.

The first two constraints are straightforward, the bonus bits were alright.  The really difficult part is the blocking heuristics, and this is the rabbit hole that would make it worthy of ICFP.

Well, I never quite figured out how to do blocking placements that would satisfy all attendees.  In times when I don't know what to do, the two strategies are to do random things, or do it manually.  I did both.

## Random placements

The task sounds like it would be a good fit for genetic algorithms -- by randomly mutating some musician placements and moving forward with the mutations that made an improvement, we should converge on some reasonable placements.

I would like to say I implemented something in the spirit of genetic algorithms:

- Divide musicians into groups and apply random movements for each group.
- Move forward with the group that seemed most promising.

But honestly it was driven by intuitions as opposed to principles.  It seemed to work better than average, except on problems that require more deliberate placements to block out the most hated musicians.

## Collision constraints

To quickly avoid collisions while applying random movements, I forced all candidate positions to fall on a rectangular grid that is 10 units apart.  In other words, if a grid cell is not currently occupied, placing a musician there guarantees no collisions.  This check works very fast, and doesn't have any problems with floating point precision and edge cases.

There is an obvious down side that I have greatly reduced a large space of real numbers to a small space of integers, and any minute placements less than 10 units away simply aren't attempted.  To mitigate this, I tried shifting the grid by some random offset for each run.  This made an improvement for some problems, but generally it made things worse because by not aligning the grid to the stage, musicians couldn't be placed at the edges for maximum impact, so I reverted this change.

## Scoring optimizations

To increase scoring speed, I only look at the top 150 attendees that are likely to be sensitive to musician placements.  This worked out to be those that are closest to the stage or have a greater range in their tastes.  Since the impact of each musician diminishes greatly with distance, this worked out to be a reasonable proxy for actual score during iterations.

## Miscellaneous infrastructure

One consistent strategy that has always worked is to keep track of the current best solution locally, and only submit the ones that made an improvement.  This is implemented by caching all solutions, and store their scores in a separate metadata file.  Since I needed to generate SVG files for visualization, those also doubles as metadata storage.

After having cached solutions and make sure that the scores can only increase, I added a script that would run through the problems continuously while I am eating lunch and stuff.  Occasionally, it might even make a few improvements.  Obviously I could have greater luck if I had more computers to do this not-very-principled randomized search, but I was fairly happy with the results.

## Manual placements

One motivation for generating SVGs was facilitate manual edits, for the few hard problems that I thought I might have a better chance and manually placing the blockers.  Rather than writing an interactive UI for this purpose, I wrote a script to extract the placements from SVG, such that I can edit the placements with Inkscape.

Well, I went at problem 10 for a few minutes and quickly concluded that manual edits were unworkable.

## Extensions

I am glad the extensions were backward compatible and could have been completely ignored, which would have saved me a lot of grief if I were short on time.  I did not give any thought to increasing impact due to musicians of the same instrument being closer together, other than updating the scoring function to take that into account.  The randomized search sort of took care of that automatically.

I did make one good use for volume control -- any musician that generated negative impact are completely muted, while everyone else gets maximum volume.

## Closing thoughts

This year's task was reasonably fun, and had sufficient depth to make the ~42 hours I spent on it worthwhile.  There were some glitches with the contest environment that were a bit rough, but overall I had a good time.

## Contact info

- https://mastodon.social/@omoikane
- https://twitter.com/uguu_org
- omoikane@uguu.org
