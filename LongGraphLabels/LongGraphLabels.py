#!/usr/bin/env python3
# Travel Mapping Project, Jim Teresco, 2017
"""Read in a .tmg graph file and report all vertex labels longer than
some threshold.

(c) 2017, Jim Teresco
"""

import argparse
import io
import sys

parser = argparse.ArgumentParser(description="Find long vertex labels in a tmg format graph.")
parser.add_argument("graph", help=".tmg file name")
parser.add_argument("threshold", help="label length threshold", type=int)

args = parser.parse_args()

print("Loading graph " + args.graph + " to look for labels longer than " + str(args.threshold) + ".")

file = open(args.graph, "rt")
# check TMG header
line = file.readline()
if not line.startswith("TMG"):
    print("Not a TMG file.")
    sys.exit(1)

# read num vertices and num edges
line = file.readline()
nums = line.split()
num_verts = int(nums[0])

# read num_verts lines and print long labels
for i in range(num_verts):
    line = file.readline()
    parts = line.split()
    if len(parts[0]) >= args.threshold:
        print(parts[0])

file.close()
