#ifndef PROBLEM_H_
#define PROBLEM_H_

#include<string>
#include<vector>

struct XY
{
   double x, y;
};

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

   explicit Problem(const std::string &json_text);

   bool valid() const { return !musicians_.empty(); }
   const XY &room_size() const { return room_size_; }
   const XY &stage_size() const { return stage_size_; }
   const XY &stage_bottom_left() const { return stage_bottom_left_; }
   const std::vector<int> &instruments() const { return instruments_; }
   const std::vector<int> &musicians() const { return musicians_; }
   const std::vector<Attendee> &attendees() const { return attendees_; }

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
};

#endif  // PROBLEM_H_
