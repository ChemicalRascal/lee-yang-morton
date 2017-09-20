#!/usr/bin/env python3
# vim: set et sts=4 sw=4 cc=80 tw=80: 
#
# run_queries_on_tree.py:   Reads a "tree input" file, and a series of
#                           queries from stdin. Outputs, per-line, the
#                           number of points within that query window
#                           in the tree.
#
#                           This process is done so in a particularly
#                           inefficient manner to ensure validity.

import sys
import os
import re

def check_argv():
    if (len(sys.argv) != 2):
        print_usage()
    try:
        open(sys.argv[1], "r").close()
    except ValueError:
        print_usage()
    except OSError:
        print("{0} cannot be opened.".format(sys.argv[1]))
        print_usage()
    return

def print_usage():
    print("Usage: {0} [TREE FILE]".format(sys.argv[0]), file=sys.stderr)
    sys.exit(1)

def parse_tree_line(line):
    regex = "[^0-9]*([0-9]+)[^0-9]+([0-9]+)[^0-9]*"
    match = re.match(regex, line)
    if (match == None):
        return None
    return (int(match.group(1)), int(match.group(2)))

def parse_query_line(line):
    regex = ("[^0-9]*([0-9]+)[^0-9]+([0-9]+)" + 
            "[^0-9]+([0-9]+)[^0-9]+([0-9]+)[^0-9]*")
    match = re.match(regex, line)
    if (match == None):
        return None
    return (min(int(match.group(1)), int(match.group(3))),
            min(int(match.group(2)), int(match.group(4))),
            max(int(match.group(1)), int(match.group(3))),
            max(int(match.group(2)), int(match.group(4))))

def run_query_against_tree(tree_path, lox, loy, hix, hiy):
    tree_fp = open(tree_path, "r")
    count = 0
    line = tree_fp.readline()
    while line:
        match_tup = parse_tree_line(line)
        if (match_tup != None):
            if((lox <= match_tup[0] <= hix) and (loy <= match_tup[1] <= hiy)):
                count += 1
        line = tree_fp.readline()
    tree_fp.close()
    return count

def main():
    check_argv()

    line = sys.stdin.readline()
    while line:
        match_tup = parse_query_line(line)
        if (match_tup != None):
            print("{0},{1} {2},{3}: {4}".format(
                match_tup[0], match_tup[1], match_tup[2], match_tup[3],
                run_query_against_tree(sys.argv[1],
                match_tup[0], match_tup[1], match_tup[2], match_tup[3])))
        line = sys.stdin.readline()
    sys.exit(0)

if __name__ == "__main__":
    main()
