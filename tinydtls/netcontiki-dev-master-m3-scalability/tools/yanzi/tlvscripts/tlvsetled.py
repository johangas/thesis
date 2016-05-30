#!/usr/bin/python
# (c) 2015 Yanzi Networks AB
#
# Author: Ioannis Glaropoulos, <ioannisg@kth.se>
#
# Modifying (setting) the status of the LED from a Yanzi Node!

import os, inspect, sys, struct
import binascii

# Import parent folder for tlvlib
scriptfile = os.path.abspath(inspect.getfile(inspect.currentframe()))
parentdir = os.path.dirname(os.path.dirname(scriptfile))
sys.path.insert(0, parentdir)
import tlvlib

# Verbose is by default disabled
verbose = False

# LED is set to OFF by default
ledval = 0

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

if len(sys.argv) > arg and sys.argv[arg] == "-l":
    arg += 1
    if len(sys.argv) > arg:
      ledvalue = sys.argv[arg]
      if ledvalue == "ON":
        ledval = 1
      elif ledvalue == "OFF":
        ledval = 0
      else:
        print "Argument error"
      arg += 1

if len(sys.argv) > arg:
    instance = int(sys.argv[arg])
    arg += 1

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
  elif data[0] == tlvlib.LED_GENERIC:
    found = 1
    t1 = tlvlib.createTLV(tlvlib.TLV_SET_REQUEST, i, tlvlib.VARIABLE_LED_CONTROL, 0, struct.pack("!L", ledval))
    enc, tlvs = tlvlib.sendTLV([t1], host)
    if tlvs[0].Error == 0:
      if ledval & 1 == 0:
        print "\tLED Set to OFF"
      else:
        print "\tLED Set to ON"
    else:
       print "LED Set error"
  i = i + 1

if found == 0:
  print "LED Status instance not found"

