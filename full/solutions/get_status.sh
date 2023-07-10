#!/bin/bash

if [[ $# -lt 1 ]]; then
   echo "$0 {problem IDs}"
   exit 1
fi

for i in $@; do
   if [[ -s $i.json ]]; then
      echo -n "$i: "
      curl \
         -s \
         -H "Authorization: Bearer eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJ1aWQiOiI2NDllNDUyZThjNjg1MzEzZDFjNjBkNjciLCJpYXQiOjE2ODg1OTAxMzYsImV4cCI6MTY5ODU5MDEzNn0.MZyOLUuOoY4E4yExaPYE5EgnJ18dfIQyg3JUaTmNMdo" \
         "https://api.icfpcontest.com/submissions?offset=0&limit=1&problem_id=$i"
      echo ""
   else
      echo "Missing $i.json"
   fi
done
