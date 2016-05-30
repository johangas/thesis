import setup, tlvlib, time
import unittest, serialradio, binascii

# tests that check the basic serial radio features before any networking
# is tested.
# NOTE: this require that the NBR is started

class SerialRadioTest(unittest.TestCase):

    con = None

    def setUp(self):
        self.con = serialradio.SerialRadioConnection()
        self.con.connect()


    def test_get_mac(self):
        print "Getting mac address"
        self.con.send("?M")
        data = self.con.get_next_block(10)
        if data != None:
            print "MAC:", binascii.hexlify(data.serialData[2:])
        assert(data != None)
        assert(data.serialData != None)
        assert(len(data.serialData) == 10)

    def test_get_channel(self):
        print "Getting channel"
        self.con.send("?C")
        data = self.con.get_next_block(10)
        if data != None:
            print "Channel:", binascii.hexlify(data.serialData[2:])
        assert(data != None)
        assert(data.serialData != None)
        assert(len(data.serialData) == 3) # !C<byte> = 3 bytes


    def test_set_channel(self):
        self.con.send("!C\x10") # channel 16
        self.con.send("?C")
        data = self.con.get_next_block(2)
        if data != None:
            print "Channel:", ord(data.serialData[2])
        assert(ord(data.serialData[2]) == 16)

        self.con.send("!C\x11") # channel 17
        self.con.send("?C")
        data = self.con.get_next_block(2)
        assert(ord(data.serialData[2]) == 17)


suite = unittest.TestLoader().loadTestsFromTestCase(SerialRadioTest)
unittest.TextTestRunner(verbosity=3).run(suite)

