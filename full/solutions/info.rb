#!/usr/bin/ruby

require "json"

data = JSON.parse(ARGF.read)

x = data["placements"].map{|p| p["x"]}
y = data["placements"].map{|p| p["y"]}
print "musicians = #{data["placements"].size} ",
      "bounding box = (#{x.min}, #{y.min}) .. (#{x.max}, #{y.max})\n"
