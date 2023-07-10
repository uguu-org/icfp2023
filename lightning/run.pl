#!/usr/bin/perl -w

use strict;
use File::Temp qw{tempfile};

# Extract score from SVG file.
sub extract_score($)
{
   my ($svg) = @_;

   if( open my $handle, "<$svg" )
   {
      while( my $line = <$handle> )
      {
         if( $line =~ />Score = (\S+)</ )
         {
            close $handle;
            return $1;
         }
      }
      close $handle;
   }
   return -1e9;
}


if( $#ARGV < 0 )
{
   die "$0 {problem IDs}\n";
}

my ($handle, $output_json, $output_svg);
($handle, $output_json) = tempfile("tmp_XXXXXX", DIR=>".", SUFFIX=>".json");
($handle, $output_svg) = tempfile("tmp_XXXXXX", DIR=>".", SUFFIX=>".svg");

foreach my $i (@ARGV)
{
   my $input = "problems/$i.json";
   unless( -s $input )
   {
      unlink $output_json;
      unlink $output_svg;
      die "Missing $input\n";
   }

   # Run solver.
   if( system("./solve $input $output_json $output_svg") != 0 )
   {
      unlink $output_json;
      unlink $output_svg;
      print STDERR "Solver failed on $input\n";
      next;
   }

   # Check if new solution is any better than existing solution, replace
   # existing solution if so.
   my $existing_json = "solutions/$i.json";
   my $existing_svg = "solutions/$i.svg";
   my $existing_score = extract_score($existing_svg);
   my $new_score = extract_score($output_svg);
   if( $existing_score < $new_score )
   {
      print "Updated $existing_json: $existing_score < $new_score",
            " (+", $new_score - $existing_score, ")\n";
      unless( rename $output_json, $existing_json )
      {
         die "Failed to rename $output_json into $existing_json: $!";
      }
      unless( rename $output_svg, $existing_svg )
      {
         die "Failed to rename $output_svg into $existing_svg: $!";
      }
   }
   else
   {
      print "Keeping $existing_json: $existing_score >= $new_score\n";
      unlink $output_json;
      unlink $output_svg;
   }
}
