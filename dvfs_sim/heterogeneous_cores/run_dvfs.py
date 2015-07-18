#!/usr/bin/env python

import sys
sys.path.extend(['..'])
from parse_lib import *
from dvfs_sim_lib import *

"""
Configuration
"""
margin = 1.1 # Prediction margin

# Power [W] measured on ODROID 
little_active_power = 0.306
little_idle_power = 0.139
big_active_power = 0.921
big_idle_power = 0.363
# Relative active power
little_power_factor = little_active_power / big_active_power
big_power_factor = 1.0
# Relative idle power
little_idle_power_factor = little_idle_power / little_active_power
big_idle_power_factor = big_idle_power / big_active_power
# little exec time = big_speedup_factor * big exec time
big_speedup_factor = 2.06
# Time to switch between core in microseconds
switching_time = 0
# Frequency levels available on each core
dvfs_levels_little = [ 1.0 * x for x in range(2, 15) ] # 100 MHz
dvfs_levels_big = [ 1.0 * x for x in range(2, 21) ] # 100 MHz

def find_best_frequency(predicted_time, dvfs_levels, dvfs_function, deadline, switch_core):
  """
  Find lowest frequency that will meet deadline.
  """
  for f in dvfs_levels:
    time = dvfs_function(predicted_time, f, switch_core)
    if time <= deadline:
      return f
  # Default is to return max frequency
  return max(dvfs_levels)

def energy(frequency, power_factor, time):
  """
  E = P*t
    = 1/2CVAf^2 * t
  With V = bf,
  E = cf^3 * t
  where c is some scaling factor
  """
  return power_factor * frequency ** 3 * time

def total_energy(frequency, power_factor, idle_power_factor, time, deadline):
  """
  Total energy = active energy + idle energy
  """
  #return energy(frequency, power_factor, time) + energy(frequency, power_factor*idle_power_factor, deadline - time)
  return energy(frequency, power_factor, time)

def run_dvfs_hetero(metric, times, predict_times, deadline, biglittle, policy, normalize_big):
  """
  Run DVFS simulation.
  """
  # Initialization
  energies = []
  deadline_misses = 0
  little_count = 0
  big_count = 0
  switch_count = 0
  # Existence of big/little core
  big_core = 'big' in biglittle
  little_core = 'little' in biglittle
  assert big_core or little_core
  # Start on big core if it exists
  if big_core:
    last_core = 'big'
  else:
    last_core = 'little'

  # For each job,
  for pt, t in zip(predict_times, times):
    # Increase predicted time by margin
    pt = margin * pt

    # Little and big cores
    if little_core and big_core:
      # Find best frequency to use on each core
      little_f = find_best_frequency(pt, dvfs_levels_little, dvfs_function_little, deadline, last_core == 'big')
      big_f = find_best_frequency(pt, dvfs_levels_big, dvfs_function_big, deadline, last_core == 'little')
      # Calculate resulting actual execution times
      little_actual_time = dvfs_function_little(t, little_f, last_core == 'big')
      big_actual_time = dvfs_function_big(t, big_f, last_core == 'little')
      # Calculate energy usage for each core
      little_energy = total_energy(little_f, little_power_factor, little_idle_power_factor, little_actual_time, deadline)
      big_energy = total_energy(big_f, big_power_factor, big_idle_power_factor, big_actual_time, deadline)
      # Little core uses less energy (and meets deadline)
      if little_energy <= big_energy and little_actual_time <= deadline:
        energies.append(little_energy)
        deadline_misses += 1 if little_actual_time > deadline else 0
        switch_count += 1 if last_core == 'big' else 0
        last_core = 'little'
        little_count += 1
      # Big core uses less energy
      else:
        energies.append(big_energy)
        deadline_misses += 1 if big_actual_time > deadline else 0
        switch_count += 1 if last_core == 'little' else 0
        last_core = 'big'
        big_count += 1

    # Little core only
    elif little_core:
      # Find best frequency to use
      little_f = find_best_frequency(pt, dvfs_levels_little, dvfs_function_little, deadline, False)
      # Actual execution time
      actual_time = dvfs_function_little(t, little_f, False)
      # Calculate energy usage
      energies.append(total_energy(little_f, little_power_factor, little_idle_power_factor, actual_time, deadline))
      # Account for deadline misses
      if actual_time > deadline:
          deadline_misses += 1
      # Switch to running on little core
      last_core = 'little'
      little_count += 1

    # Big core only
    elif big_core:
      # Find best frequency to use
      big_f = find_best_frequency(pt, dvfs_levels_big, dvfs_function_big, deadline, False)
      # Actual execution time
      actual_time = dvfs_function_big(t, big_f, False)
      # Calculate energy usage
      energies.append(total_energy(big_f, big_power_factor, big_idle_power_factor, actual_time, deadline))
      # Account for deadline misses
      if actual_time > deadline:
          deadline_misses += 1
      # Switch to running on big core
      last_core = 'big'
      big_count += 1

  # Return the metric of interest
  if metric == 'energy':
    # Normalize energy against max frequency on the appropriate core
    if normalize_big:
      max_energy = sum([ total_energy(max(dvfs_levels_big), big_power_factor, big_idle_power_factor, t, deadline) for t in times ])
    else:
      max_energy = sum([ total_energy(max(dvfs_levels_little), little_power_factor, little_idle_power_factor, t, deadline) for t in times ])
    return sum(energies) / max_energy
  elif metric == 'deadline_misses':
    return float(deadline_misses) / len(times)
  elif metric == 'switch_count':
    return float(switch_count) / len(times)
  elif metric == "big_count":
    assert big_count + little_count == len(times)
    return float(big_count) / len(times)
  else:
    raise Exception('Unknown metrics: %s' % metric)


if __name__ == '__main__':
  # Parse command line arguments
  if len(sys.argv) < 2:
    print 'usage: run_dvfs.py big|little [--no_test]'
    exit(1)
  if sys.argv[1] == 'big':
    bigonly = True
    littleonly = False
  elif sys.argv[1] == 'little':
    bigonly = False
    littleonly = True
  else:
    print 'usage: run_dvfs.py big|little [--no_test]'
    exit(1)
  assert bigonly != littleonly
  no_test = False
  if '--no_test' in sys.argv:
    no_test = True

  # Configuration for bigonly vs. biglittle
  if bigonly:
    input_dir = '../data_big'
    output_dir = 'predict_times_big'
    dvfs_function_little = lambda t, f, switch: float(t) * max(dvfs_levels_big) / f * big_speedup_factor + (switching_time if switch else 0)
    dvfs_function_big = lambda t, f, switch: float(t) * max(dvfs_levels_big) / f + (switching_time if switch else 0)
  # Configuration for littleonly vs. biglittle
  elif littleonly:
    input_dir = '../data_little'
    output_dir = 'predict_times_little'
    dvfs_function_little = lambda t, f, switch: float(t) * max(dvfs_levels_little) / f + (switching_time if switch else 0)
    dvfs_function_big = lambda t, f, switch: float(t) * max(dvfs_levels_little) / f * (1 / big_speedup_factor) + (switching_time if switch else 0)

  # Find all policies that were tested
  policies = []
  for root, dirname, filenames in os.walk(output_dir):
    for filename in filenames:
      policy = filename.split('-')[0]
      if policy not in policies:
        policies.append(policy)

  # For each metric of interest
  for metric in ['energy', 'deadline_misses', 'switch_count', 'big_count']:
    # Print metric name
    if isinstance(metric, str):
      print metric
    else:
      print metric.__name__
    # Print list of benchmarks
    print list_to_csv([''] + benchmarks + ['average'])

    # Define different deadlines to use
    if bigonly:
      biglittle_configurations = ['bigonly-100', 'biglittle-100']
    elif littleonly:
      biglittle_configurations = ['littleonly-60', 'littleonly-80', 'littleonly-100',
          'biglittle-60', 'biglittle-80', 'biglittle-100']
    for biglittle in biglittle_configurations:

      # For each controller policy,
      for policy in policies:
        print '%s-%s' % (policy, biglittle), ',',
        sum_metric = 0

        # For each benchmark,
        for benchmark in benchmarks:
          # if no_test, use same test and train sets
          if no_test:
            times = parse_execution_times('%s/%s/%s0.txt' % (input_dir, benchmark, benchmark))
          else:
            times = parse_execution_times('%s/%s/%s1.txt' % (input_dir, benchmark, benchmark))
          # Read in predicted times
          predict_times = read_predict_file('%s/%s-%s.txt' % (output_dir, policy, benchmark))
          # Determine deadline based on worst-case execution time
          scaling = float(biglittle.split('-')[1]) / 100
          deadline = scaling * max(times)

          # Run DVFS simulation
          metric_result = run_dvfs_hetero(metric, times, predict_times, deadline, biglittle, policy, bigonly)

          # Sum metric to calculate average
          sum_metric += metric_result
          # Output metric result
          print metric_result, ',',
        # Output average across benchmarks
        print sum_metric / len(benchmarks)
    print
