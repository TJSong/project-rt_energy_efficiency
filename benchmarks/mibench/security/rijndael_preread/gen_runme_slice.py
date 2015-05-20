#!/usr/bin/python

import random

random.seed(0)

def gen_key():
  """
  Generate a 256-bit key in hexadecimal format.
  """
  hex_digits = "0123456789abcdef"
  key = ""
  for i in range(64):
    key += random.choice(hex_digits)
  return key

output = "#!/bin/bash\n"
output += "./rijndael "
input_range = range(1, 100)
for i in input_range:
  output += "/home/odroid/project-rt_energy_efficiency/datasets/rijndael-50ms/input%d.txt output_large.enc e %s " % (i, gen_key())
  #output += "../rijndael/input_files/input_random%d.asc output_large.enc e %s " % (i, gen_key())
print output

