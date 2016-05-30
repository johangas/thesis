#
# Basic unit tests of the TLV/Encap library for python
#

import tlvlib,binascii,struct,unittest

class TestTLV(unittest.TestCase):

    def doTLVAsserts(self, t1):
        self.assertEqual(t1.Version, 0)
        self.assertEqual(t1.Opcode, tlvlib.TLV_GET_REQUEST)
        self.assertEqual(t1.Variable, tlvlib.VARIABLE_SW_REVISION)
        self.assertEqual(t1.Instance, 0)
        self.assertEqual(t1.size(), 8)
    
    def test_tlv_create(self):
        t1 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, 0,
                              tlvlib.VARIABLE_SW_REVISION, 2, None)
        self.doTLVAsserts(t1)

    def test_tlv_parse(self):
        tbin = binascii.unhexlify("000200cc00000200")
        t1 = tlvlib.parseTLV(tbin)
        self.doTLVAsserts(t1)
