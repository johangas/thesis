#
#
#
import glob, re, setup, subprocess, unittest, testtlv, time
import basicnetwork_tests, startup_tests

nbr_path = "../../ipv6/native-border-router/border-router.native"
nbr = None

def start_nbr():
    print "Starting NBR"
    dev = ""
    if setup.get_platform() == "osx":
        files = glob.glob("/dev/tty.usb*")
        if files == None or len(files) == 0:
            return None
        dev = "-s" + files[0]

    print "Starting: ", nbr_path, "-B460800", dev, "aaaa::1/64"
    return subprocess.Popen([nbr_path, "-B460800", dev, "aaaa::1/64"],
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)


def kill_nbr(nbr):
    print "Killing NBR"
    if nbr != None:
        nbr.terminate()

suite = unittest.TestLoader().loadTestsFromTestCase(testtlv.TestTLV)
unittest.TextTestRunner(verbosity=3).run(suite)


# This test suite requires the NBR to be running
nbr = start_nbr()

if nbr == None:
    print "Failed to start NBR - exiting..."
    exit(1)

# we need to pick up the NBR address so that the web-page of the NBR
# can be read.
# this data will be written to disk for other tests to be able to acess
# it (the config file)
ret = None
addr = None
while ret == None:
    line = nbr.stdout.readline()
    print line
    ret = nbr.poll()
    m = re.search("=>(aaaa:.*)", line)
    if m != None:
        print "Found:", m.group(0), m.group(1)
        addr = m.group(1)
        break

if ret > 0:
    line = nbr.stderr.readline()
    print "Error: ", line
    exit(1)

# Set the NBR config
setup.set_config("nbr", "address", addr)

print "Waiting 10 seconds for NBR to start."
time.sleep(10)

suite = unittest.TestLoader().loadTestsFromTestCase(startup_tests.StartupTest)
unittest.TextTestRunner(verbosity=3).run(suite)


suite = unittest.TestLoader().loadTestsFromTestCase(basicnetwork_tests.PingTest)
unittest.TextTestRunner(verbosity=3).run(suite)

kill_nbr(nbr)
