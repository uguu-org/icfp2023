#!/usr/bin/ruby

require 'json'

if ARGV.size != 2
   print "#{$0} {problem_id} {solution.json} > output.json\n"
   exit 1
end

contents = nil
File.open(ARGV[1], "r") {|infile| contents = infile.read }

# Try parsing contents to verify that it's valid.
JSON.parse(contents)

print JSON.generate({"problem_id": ARGV[0].to_i, "contents": contents})
