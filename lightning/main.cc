#include<stdio.h>

#include<array>
#include<sstream>
#include<string>

#include"problem.h"
#include"solution.h"

namespace {

// Load text from file.
static std::string LoadText(const char *filename)
{
   std::string text;
   FILE *infile = fopen(filename, "rb");
   if( infile == nullptr )
   {
      fprintf(stderr, "Failed to open %s\n", filename);
      return text;
   }

   std::array<char, 1024> buffer;
   size_t read_size;
   do
   {
      read_size = fread(buffer.data(), 1, buffer.size(), infile);
      text.append(std::string(buffer.data(), read_size));
   } while( read_size == buffer.size() );
   fclose(infile);
   return text;
}

// Write solution to file.
static void OutputSolution(const Solution &solution, FILE *output)
{
   fputs("{\"placements\":[", output);

   bool first = true;
   for(const XY &xy : solution.placements)
   {
      if( first )
      {
         first = false;
      }
      else
      {
         fputc(',', output);
      }
      fprintf(output, "{\"x\":%f,\"y\":%f}", xy.x, xy.y);
   }

   fputs("]}\n", output);
}

// Write solution with visualization to file.
static void OutputVisualization(const Problem &problem,
                                const Solution &solution,
                                FILE *output)
{
   // Header.
   fprintf(output, R"(<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<svg width="%g" height="%g" xmlns="http://www.w3.org/2000/svg" xmlns:svg="http://www.w3.org/2000/svg">
<rect style="fill:none;stroke:#000000" x="0" y="0" width="%g" height="%g" />
<rect style="fill:none;stroke:#ff0000" x="%g" y="%g" width="%g" height="%g" />
)",
           problem.room_size().x, problem.room_size().y,
           problem.room_size().x, problem.room_size().y,
           problem.stage_bottom_left().x, problem.stage_bottom_left().y,
           problem.stage_size().x, problem.stage_size().y);

   // Attendees.
   for(const Problem::Attendee &a : problem.attendees())
   {
      fprintf(output, R"(<circle style="fill:none;stroke:#000000" cx="%g" cy="%g" r="5" />)",
              a.position.x, a.position.y);
   }

   // Musicians.
   for(const XY &m : solution.placements)
   {
      fprintf(output, R"(<circle style="fill:none;stroke:#0000ff" cx="%g" cy="%g" r="10" />)",
              m.x, m.y);
   }

   // Footer.
   static constexpr const char kFontStyle[] =
      "font-family:sans-serif;"
      "font-size:20;"
      "fill:#ff0000;"
      "fill-opacity:1;"
      "stroke:#ffffff;"
      "stroke-opacity:1;"
      "stroke-width:3;"
      "paint-order:stroke fill";
   fprintf(output, R"(<text text-anchor="start" x="%g" y="%g" style="%s">Score = %.0f</text>
<text text-anchor="end" x="%g" y="%g" style="%s">Score = %.0f</text>
</svg>
)",
           problem.stage_bottom_left().x,
           problem.stage_bottom_left().y,
           kFontStyle,
           solution.score,
           problem.stage_bottom_left().x + problem.stage_size().x,
           problem.stage_bottom_left().y + problem.stage_size().y + 20,
           kFontStyle,
           solution.score);
}

}  // namespace

int main(int argc, char **argv)
{
   if( argc != 4 )
   {
      return fprintf(stderr, "%s {input.json} {output.json} {output.svg}\n",
                     *argv);
   }

   Problem problem(LoadText(argv[1]));
   if( !problem.valid() )
   {
      fprintf(stderr, "%s is invalid\n", argv[1]);
      return 1;
   }

   Solution solution;
   Solve(problem, &solution);

   FILE *outfile = fopen(argv[2], "wb");
   if( outfile == nullptr )
   {
      perror("Error opening output JSON");
   }
   else
   {
      OutputSolution(solution, outfile);
      if( fclose(outfile) != 0 )
         perror("Error writing output JSON");
   }

   outfile = fopen(argv[3], "wb");
   if( outfile == nullptr )
   {
      perror("Error opening output SVG");
   }
   else
   {
      OutputVisualization(problem, solution, outfile);
      if( fclose(outfile) != 0 )
         perror("Error writing output SVG");
   }
   return 0;
}
