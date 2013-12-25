#!/usr/bin/python

"""
Print out the function call expressions in the passed AST file.

get_conditionals.py filename
"""

import re
import sys
import gcc_parse

filename = sys.argv[1]

f = open(filename, 'r')
# Contains gcc expression strings indexed by object id #
objects = {0:""}
# List of expression strings that are function call expressions
call_exprs = []
obj_no = 0
for line in f:
  # Pull out expression ID and add to dict
  res = re.search("^@([0-9]+)", line)
  if res:
    obj_no = int(res.group(1))
    objects[obj_no] = line.strip()
  # Continuation of last node
  else:
    objects[obj_no] += ' ' + line.strip()

  # Keep track of call expressions
  if "call_expr" in line:
    call_exprs.append(line.strip())
f.close()

gcc_parse.objects = objects
arguments = []
# For each call expressions
for line in call_exprs:
  # Create tree
  root = gcc_parse.parse(line)
  #print root.id, root

  root_args = root.get_variables()
  for a in root_args:
    if str(a) not in arguments:
      arguments.append(str(a))

for a in arguments:
  print "printf(\"%%d, \", (int)%s);" % (a)
