#!/usr/bin/env bash
#
read_data=1
if [ $# -eq 1 ]; then
  if [ "$1" == "--noread" ]; then
     read_data=0
  fi
fi
if [ "$read_data" == "1" ]; then
  echo "siteupdate.sh: launching read_data.py"
  ##./read_data.py
else
  echo "siteupdate.sh: SKIPPING read_data.py"
fi
exit 0
