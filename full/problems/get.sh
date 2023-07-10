#!/bin/bash

for i in $(seq 90); do
   curl -s "https://api.icfpcontest.com/problem?problem_id=$i" | ruby extract_inner_json.rb > $i.json
   ls -l $i.json
done
