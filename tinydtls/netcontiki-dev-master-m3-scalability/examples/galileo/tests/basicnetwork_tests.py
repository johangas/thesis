import setup
import unittest
import urllib2
import os, time

class PingTest(unittest.TestCase):

    nbr_address = None

    def setUp(self):
        cfg = setup.read_config()
        self.nbr_address = cfg.get('nbr','address')
        print "Setup - using NBR Address: ", self.nbr_address

    def test_ping_first(self):
        count = 0
        while count < 10:
            print "Fetching web page from host:", self.nbr_address
            data = urllib2.urlopen("http://[" + self.nbr_address + "]/r").read()
            print "Data: ", data
            hostname = data.split('\n')[0]
            print "Host: ", hostname
            if hostname != "": break
            print "no host found... retrying in 10 seconds"
            time.sleep(10)
            count = count + 1
        response = os.system("ping6 -c 5 " + hostname)
        print "response:", response
        assert(response == 0)

