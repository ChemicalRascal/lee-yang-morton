#!/usr/bin/env python3
# vim: set et sts=4 sw=4 cc=80 tw=80: 
#
# gen_tree_input.py:    Generate a file containing list of co-ordinates within
#                       a given set of bounds, with a given probability for any
#                       particular point existing.

import sys
import random

def check_argv():
    if (len(sys.argv) != 6):
        print_usage()
    try:
        if not (int(sys.argv[1]) >= 0):
            print_usage()
        if not (int(sys.argv[2]) >= 0):
            print_usage()
        if not (int(sys.argv[3]) >= 0):
            print_usage()
        if not (int(sys.argv[4]) >= 0):
            print_usage()
        if not (int(sys.argv[1]) <= int(sys.argv[2])):
            print_usage()
        if not (int(sys.argv[3]) <= int(sys.argv[4])):
            print_usage()
    except ValueError:
        print_usage()
    try:
        if not (1 >= float(sys.argv[5]) >= 0):
            print_usage()
    except ValueError:
        print_usage()
    return

def print_usage():
    print("Usage: {0} [MIN_X] [MAX_X] [MIN_Y] [MAX_Y] [POINT PROBABILITY]"
            .format(sys.argv[0]), file=sys.stderr)
    sys.exit(1)

def gen_points(min_x, max_x, min_y, max_y, prob=0):
    points = []
    random.seed()
    for x in range(min_x, max_x+1):
        for y in range(min_y, max_y+1):
            r = random.random()
            if (r < prob):
                points.append((x, y))
    return points

def print_points(points):
    for (x, y) in points:
        print("{0}, {1},".format(str(x), str(y)))
    return

def main():
    check_argv()
    points = gen_points(int(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3]),
            int(sys.argv[4]), float(sys.argv[5]))
    print_points(points)
    sys.exit(0)

if __name__ == "__main__":
    main()
