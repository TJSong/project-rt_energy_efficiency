#!/usr/bin/env python

from parse_lib import *
from dvfs_sim_lib import *

def run_dvfs(predict_times, times, deadline=None, dvfs_levels=None, margin=1.1):
  """
  Given predicted times and original execution times, sets DVFS level. The max
  original execution time is used as the deadline by default. [dvfs_levels] is
  a list of allowed normalized frequencies. If no list is specified, then a
  continuous range of dvfs_levels is allowed. Prediction times are multiplied
  by [margin] to allow a conservative margin to be added. Returns the resulting
  execution times, the frequencies selected, and the deadline used.
  """
  # Set deadline as worst-case observed time
  if not deadline:
    deadline = max(times)
  # Determine frequency to set DVFS to
  if dvfs_levels:
    frequencies = [scale_frequency(margin * t, deadline, dvfs_levels=dvfs_levels) for t in predict_times]
  else:
    frequencies = [scale_frequency_perfect(margin * t, deadline) for t in predict_times]
  # Time with DVFS
  result_times = [dvfs_time(t, f) for (t, f) in zip(times, frequencies)]

  return (result_times, frequencies, deadline)

input_dir = "data"
output_dir = "predict_times"

# Find all policies that were run
policies = []
for root, dirname, filenames in os.walk(output_dir):
  for filename in filenames:
    policy = filename.split("-")[0]
    if not policy in policies:
      policies.append(policy)

#for metric in [deadline_misses, avg_normalized_tardiness, energy]:
for metric in [energy, deadline_misses]:
  print metric.__name__
  print list_to_csv([""] + benchmarks + ["average"])
  for policy in policies:
    print policy, ",", 
    sum_metric = 0
    for benchmark in benchmarks:
      # Read in execution times and predicted times
      times = parse_execution_times("%s/%s/%s1.txt" % (input_dir, benchmark, benchmark))
      predict_times = read_predict_file("%s/%s-%s.txt" % (output_dir, policy, benchmark))

      # Perform DVFS
      deadline = None
      # Custom deadlines
      if "freeciv" in benchmark:
        deadline = 30000
      elif "shmupacabra" in benchmark:
        deadline = 1000000./60 # 60fps
      elif "ldecode" in benchmark:
        deadline = 1000000./30 # 30fps
      (result_times, frequencies, deadline) = run_dvfs(predict_times, times, dvfs_levels=None, deadline=deadline) # Continuous

      # Calculate metric of interest
      metric_result = metric(result_times, frequencies, deadline)
      sum_metric += metric_result
      print metric_result, ",",
    # Output average
    print sum_metric/len(benchmarks)
  print
