#!/bin/env python3
# vim: set et sts=4 sw=4 cc=80 tw=80: 
#
# gen_tree_input.py:    Generate a file containing the depth of a tree
#                       and then a list of co-ordinates validly within
#                       that theoretical tree.

import sys
import random

def check_argv():
    if (len(sys.argv) != 3):
        print_usage()
    try:
        if not (int(sys.argv[1]) >= 0):
            print_usage()
    except ValueError:
        print_usage()
    try:
        if not (1 >= float(sys.argv[2]) >= 0):
            print_usage()
    except ValueError:
        print_usage()
    return

def print_usage():
    print("Usage: {0} [TREE DEPTH] [POINT PROBABILITY]".format(sys.argv[0]),
            file=sys.stderr)
    sys.exit(1)

def gen_points(depth=0, prob=0):
    points = []
    random.seed()
    for x in range(0, 2**depth):
        for y in range(0, 2**depth):
            r = random.random()
            if (r < prob):
                points.append((x, y))
    return points

def print_points(points, depth=None):
    if (depth is not None):
        print("{0},".format(str(depth)))
    for (x, y) in points:
        print("{0}, {1},".format(str(x), str(y)))
    return

def main():
    check_argv()
    points = gen_points(int(sys.argv[1]), float(sys.argv[2]))
    print_points(points, int(sys.argv[1]))
    sys.exit(0)

if __name__ == "__main__":
    main()
