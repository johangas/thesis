#
# File: setup.py
# setup python path and add some basic functions that makes it
# easier to perform the tests.
#
# Author: Joakim Eriksson
#
import sys, select, time, subprocess, re
import ConfigParser
from sys import platform as _platform

sys.path.append("../../../tools/yanzi")

def get_platform():
    if _platform == "linux" or _platform == "linux2":
        return "linux"
    elif _platform == "darwin":
        return "osx"
    elif _platform == "win32":
        return "win32"

def read_config():
    cfg = ConfigParser.ConfigParser()
    cfg.read("testconf.cnf")
    return cfg

def write_config(cfg):
    # Writing our configuration file to 'example.cfg'
    with open('testconf.cnf', 'wb') as configfile:
        cfg.write(configfile)
        configfile.close()

def set_config(context, var, val):
    cfg = read_config()
    cfg.set(context, var, val)
    write_config(cfg)

# execute a command, match its output during a specified time
def execute_and_match(data, match, timeout = 10):
    proc = subprocess.Popen(data,
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    ret = None
    files = [proc.stdout.fileno(), proc.stderr.fileno()]
    start_time = time.time()
    line = None
    while ret == None and line != '' and (time.time() - start_time) < timeout:
        read, write, error = select.select(files, [], files, 1)
        if read:
            line = proc.stdout.readline()
            #print "Time: ", (time.time() - start_time), ":", line
            m = re.search(match, line)
            #print "M:", m
            if m != None:
                proc.terminate()
                return m

    ret = proc.poll()
    if ret == None:
        proc.terminate()
    return None

# quick test of the execute and match
#m = execute_and_match(['ping','www.sics.se'], "icmp_seq=5 ttl=(.+?)", 7)
#m = execute_and_match(['ifconfig'], "(zen.+?):", 7)
#print "Value: ",m.group(1)
