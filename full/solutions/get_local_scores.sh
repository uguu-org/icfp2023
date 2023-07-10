#!/bin/bash

grep -F "Score =" *.svg \
   | sed -e 's/<[^<>]*>//g' \
   | sed -e 's/.svg:Score = / = /' \
   | sort -n -u
