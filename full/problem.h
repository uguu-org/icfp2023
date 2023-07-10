#ifndef PROBLEM_H_
#define PROBLEM_H_

#include<string>
#include<vector>

#include"intersect.h"

class Problem
{
public:
   struct Attendee
   {
      XY position;
      std::vector<double> tastes;

      // Range of influences that may be exerted by this attendee.
      double max_influence;
      double min_influence;
   };

   struct Pillar
   {
      XY position;
      double radius;
   };

   explicit Problem(const std::string &json_text);

   // Check if any pillar blocks the line between 'u' and 'v'.
   bool BlockedByPillar(const XY &u, const XY &v) const;

   // Spec says extension 2 is only active for full round problems,
   // and all full round problems has at least one pillar.
   bool UseClosenessExtension() const { return !pillars_.empty(); }

   bool valid() const { return !musicians_.empty(); }
   const XY &room_size() const { return room_size_; }
   const XY &stage_size() const { return stage_size_; }
   const XY &stage_bottom_left() const { return stage_bottom_left_; }
   const std::vector<int> &instruments() const { return instruments_; }
   const std::vector<int> &musicians() const { return musicians_; }
   const std::vector<Attendee> &attendees() const { return attendees_; }
   const std::vector<Pillar> &pillars() const { return pillars_; }

private:
   // Compute maximum influence for each attendee.
   void ComputeInfluences();

   // Room and stage dimensions.
   XY room_size_;
   XY stage_size_;
   XY stage_bottom_left_;

   // List of instruments played by each musician.
   std::vector<int> musicians_;

   // Number of musicians for each instrument.
   std::vector<int> instruments_;

   // Attendee preferences, sorted by attendees who are most sensitive to
   // placement changes first.
   std::vector<Attendee> attendees_;

   // Pillar positions.
   std::vector<Pillar> pillars_;
};

#endif  // PROBLEM_H_
