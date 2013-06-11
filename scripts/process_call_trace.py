#!/usr/bin/python -u

"""
Parse output of ffmpeg which includes frame timing and function call
information.

usage: process_call_trace.py tracefile 
"""

import hashlib
import re
import sys

# Parse inputs
if len(sys.argv) != 2:
  raise Exception(__doc__)
trace_filename = sys.argv[1]

tracefile = open(trace_filename, 'r')

# Initialization
call_trace = []
call_depths = [50, 100, 150, 200]
max_call_depth = max(call_depths)
hash_dicts = [dict() for i in range(len(call_depths))]

# Start parsing traces
for line in tracefile:
  # Look for frame line
  res = re.search("dlo: Frame ([0-9]+) = ([0-9]+)us", line)
  if res:
    frame_num = int(res.group(1))
    frame_time = int(res.group(2))
    # Only for frames with a valid call trace
    if call_trace:
      # Calculate hashes for each call_depth
      for (i, call_depth) in enumerate(call_depths):
        # Add this to dict of call traces
        h = hash(str(call_trace[0:call_depth]))
        if h in hash_dicts[i]:
          hash_dicts[i][h].append(frame_time)
        else:
          hash_dicts[i][h] = [frame_time]

      print frame_num, frame_time
    # Reset call trace
    call_trace = []

  # Skip if we have enough to reach call depth
  if len(call_trace) > max_call_depth:
    continue

  # Create list of functions called
  res = re.search("call\[[0-9]+\]: enter 0x([0-9a-f]+)", line)
  if res:
    call_trace.append(res.group(1))

tracefile.close()

for i in range(len(call_depths)):
  print "Call depth: %d" % call_depths[i]
  print "Hash dict: %s" % str(hash_dicts[i])
