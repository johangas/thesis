import setup
import unittest
import urllib2
import os
import socket

# tests that check the basic NBR features before any networking
# is tested (tun interface, etc).
class StartupTest(unittest.TestCase):

    nbr_address = None

    def setUp(self):
        cfg = setup.read_config()
        self.nbr_address = cfg.get('nbr','address')
        print "Setup - using NBR Address: ", self.nbr_address

    def test_tun(self):
        # run ifconfig as a subprocess and check for tun0
        m = setup.execute_and_match(['ifconfig'], "tun0", 10)
        assert(m != None)

    def test_ping_self(self):
        response = os.system("ping6 -c 5 " + self.nbr_address)
        print "response:", response
        assert(response == 0)
