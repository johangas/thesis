#!/usr/bin/python
# (c) 2015 Yanzi Networks AB
#
# Author: Ioannis Glaropoulos, <ioannisg@kth.se>
#
# Getting the temperature value of a Yanzi Node!

import os, inspect, sys, struct
import binascii

# Import parent folder for tlvlib
scriptfile = os.path.abspath(inspect.getfile(inspect.currentframe()))
parentdir = os.path.dirname(os.path.dirname(scriptfile))
sys.path.insert(0, parentdir)
import tlvlib

# Verbose is by default disabled
verbose = False

# Number of arguments
arg = 1

host = "localhost"
instance = -1

if len(sys.argv) > 1 and sys.argv[1] == "-v":
    verbose = True
    arg = 2
if len(sys.argv) > arg:
    host = sys.argv[arg]
    print "HOST: ", host
    arg += 1

if len(sys.argv) > arg:
    instance = int(sys.argv[arg])

d = tlvlib.discovery(host)
if verbose:
  print "Product label:", d[0][0], " type: %016x"%d[0][1], " instances:", len(d[1])
  print "Booted at:",tlvlib.getStartIEEE64TimeAsString(d[0][2]),"-",tlvlib.convertIEEE64TimeToString(d[0][2])

i = 1
found = 0
for data in d[1]:
  if verbose:
    print "Instance " + str(data[2]) + ": type: %016x"%data[0], " ", data[1]
  if instance > 0 and instance != i:
    True
  elif data[0] == tlvlib.TEMP_GENERIC:
    found = 1
    t1 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i, tlvlib.VARIABLE_TEMPERATURE, 0, None) 
    enc, tlvs = tlvlib.sendTLV([t1], host)
    if tlvs[0].Error == 0:
      temperature = tlvs[0].IntValue
      if temperature > 100000:
        temperature = (temperature - 273150) / 1000.0
        print "\tTemperature:",round(temperature, 3),"(C)"
  i = i + 1

if found == 0:
  print "Temperature instance not found"

