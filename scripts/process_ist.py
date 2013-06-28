#!/usr/bin/python -u

"""
Parse output of ffmpeg which includes frame timing and hex dump
of ist data structure. Dumps out to format for use with libsvm.
Treshold can be passed. If not, average frame time is used.

usage: process_ist.py tracefile svm_file [threshold]
"""

import re
import sys

"""
Break ist into a series of integers.
"""
def break_ist(ist):
  int_list = []
  chunk_len = 4
  for i in range(0, len(ist), chunk_len):
    int_list.append(int(ist[i:i+chunk_len], 16))
  return int_list

# Parse inputs
if len(sys.argv) <= 1 or len(sys.argv) >= 5:
  raise Exception(__doc__)
trace_filename = sys.argv[1]
svm_filename = sys.argv[2]
if len(sys.argv) == 4:
  threshold = int(sys.argv[3])
else:
  # Threshold will be set to average later
  threshold = None

tracefile = open(trace_filename, 'r')

# Find average frame time
total_frame_time = 0
max_frame_time = 0
min_frame_time = sys.maxint
total_frames = 0

# trace elements are [frame_time, data1, data2, ...]
trace = []
# Start parsing traces
for line in tracefile:
  # Look for feature data
  # Look for ist dump
  res = re.search("ist = ([0-9a-f]+)", line)
  if res:
    ist = res.group(1)
  # Frame height/width
  res = re.search("resample: ([0-9]+), ([0-9]+)", line)
  if res:
    height = int(res.group(1))
    width = int(res.group(2))
  # Packet size
  res = re.search("packet size = ([0-9]+)", line)
  if res:
    packet_size = int(res.group(1))

  # Look for frame line
  res = re.search("dlo: Frame ([0-9]+) = ([0-9]+)us", line)
  if res:
    frame_num = int(res.group(1))
    frame_time = int(res.group(2))

    # Save information to calculate max/min/average
    total_frame_time += frame_time
    total_frames += 1
    if frame_time > max_frame_time:
      max_frame_time = frame_time
    if frame_time < min_frame_time:
      min_frame_time = frame_time

    #trace.append([frame_time, height, width, packet_size])
    trace.append([frame_time, packet_size])

tracefile.close()

# Write out libsvm file
svm_file = open(svm_filename, 'w')

# Calculate average frame time
avg_frame_time = float(total_frame_time)/total_frames
print "Frame time range = (%d, %d)" % (min_frame_time, max_frame_time)
print "Average frame time = %f" % avg_frame_time
# Default treshold is the average
if threshold == None:
  threshold = avg_frame_time

num_slow_frames = 0
num_fast_frames = 0
# Write out to file for libsvm format
for trace_item in trace:
  frame_time = trace_item[0]
  # 2 class
  if frame_time < threshold:
    svm_file.write("-1 ")
    num_slow_frames += 1
  else:
    svm_file.write("+1 ")
    num_fast_frames += 1

  # Features
  for i in range(1, len(trace_item)):
    svm_file.write("%d:%d " % (i - 1, trace_item[i]))

  # Use length of ist
  #svm_file.write("0:%d " % len(ist))
  # Break ist string into individual integers
  # ist_list = break_ist(ist)
  # for (i, ist_chunk) in enumerate(ist_list):
  #   svm_file.write("%d:%d " % (i + 1, ist_chunk))

  svm_file.write("\n")

svm_file.close()

# Print out percentage above and below threshold
print "Slow frames = %f" % (float(num_slow_frames)/total_frames)
print "Fast frames = %f" % (float(num_fast_frames)/total_frames)
