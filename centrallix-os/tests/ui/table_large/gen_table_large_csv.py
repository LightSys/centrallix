#!/usr/bin/env python3
# Generate (or reset) table_large.csv for the large result set table tests.
# Usage: python3 gen_table_large_csv.py [rowcount]

import os
import sys

GROUPS = ["Alpha", "Bravo", "Charlie", "Delta", "Echo"]
LONG_NOTE = "This row has a deliberately long note so that wrapped text makes the row taller than its neighbors in the variable height table"


def main():
    rowcount = int(sys.argv[1]) if len(sys.argv) > 1 else 1000
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "table_large.csv")
    with open(path, "w", newline="\n") as f:
        for i in range(1, rowcount + 1):
            note = LONG_NOTE if i % 7 == 0 else "Note %d" % i
            f.write("%d,Row %04d,%s,%d,%s\n" % (i, i, GROUPS[i % len(GROUPS)], (i * 37) % 1000, note))


if __name__ == "__main__":
    main()
