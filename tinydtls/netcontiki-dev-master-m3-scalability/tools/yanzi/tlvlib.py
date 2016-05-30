#!/usr/bin/python
#
# Copyright (c) 2013-2015, SICS, Swedish ICT
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the Institute nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# Author: Joakim Eriksson, joakime@sics.se
#         Niclas Finne, nfi@sics.se
#
# TLV Library
# A small Python library for use in applications using Yanzi Application Layer
#

import binascii
from ctypes import *
import socket
import struct
import sys
import time
import string
from math import exp

DEBUG = False
# Some constants
TLV_GET_REQUEST = 0
TLV_GET_RESPONSE = 1
TLV_SET_REQUEST = 2
TLV_SET_RESPONSE = 3
TLV_EVENT_REQUEST = 6
TLV_EVENT_RESPONSE = 7

ENC_PAYLOAD_TLV = 1
ENC_PAYLOAD_SERIAL = 8

ENC_FINGERPRINT_NONE = 0  #/* 0 bytes */
ENC_FINGERPRINT_DEVID = 1 # /* 8 bytes */
ENC_FINGERPRINT_FIP4 = 2  # /* 4 bytes */
ENC_FINGERPRINT_LENOPT = 3 #, /* 4 bytes */
ENC_FINGERPRINT_DID_AND_FP = 4 # /* 16 bytes */

ENC_FINGERPRINT_LENOPT_OPTION_CRC = 1
ENC_FINGERPRINT_LENOPT_OPTION_SEQNO_CRC = 2

CRC_MAGIC_REMAINDER = 0x2144DF1C

# some defaults
UDP_IP = "127.0.0.1"
UDP_PORT = 49111

# Some product / instance ID's and instance variables
IMAGE = 0x0090da0303010010L
RADIO = 0x0090DA0303010014L
GPIO = 0x0090DA0302010013L
LEDS = 0x0090DA030201001EL
BUTTON = 0x0090DA030201001DL
ENERGY_METER = 0x0090da0303010013L
SHT20 = 0x0090DA0303010011L
CO2 = 0x0090DA0302010017L
TEMPHUM_GENERIC = 0x0090DA0302010018L
TEMP_GENERIC = 0x0090DA0302010019L
ROUTER = 0x0090da0302010015L
BORDER_ROUTER = 0x0090DA0302010014L
BORDER_ROUTER_MANAGEMENT = 0x0090DA0303010022L
NETWORK_STATISTICS = 0x0090DA0303010023L
PTCTEMP = 0x0090DA0303010012L
POWER_SINGLE = 0x0090DA0303010021L
RANGING_GENERIC = 0x0090DA030201001CL
LAMP = 0x0090DA030201001AL
CYCLOPS = 0x0090DA03010104B0L
MOTION_GENERIC = 0x0090DA0302010016L
LED_GENERIC = 0x0090DA030201001E
BUTTON_GENERIC = 0x0090DA030201001D

# variables available in all instances
VARIABLE_PRODUCT_TYPE = 0x000
VARIABLE_PRODUCT_ID = 0x001
VARIABLE_PRODUCT_LABEL = 0x002
VARIABLE_NUMBER_OF_INSTANCES = 0x003
VARIABLE_PRODUCT_SUB_TYPE = 0x004
VARIABLE_EVENT_ARRAY = 0x005

# variables used in image instances
VARIABLE_IMAGE_TYPE	= 0x104
VARIABLE_IMAGE_STATUS	= 0x105
VARIABLE_IMAGE_VERSION	= 0x106

IMAGE_STATUS_OK		   = 0x00
IMAGE_STATUS_BAD_CHECKSUM  = 0x01
IMAGE_STATUS_BAD_TYPE	   = 0x02
IMAGE_STATUS_BAD_SIZE	   = 0x04
IMAGE_STATUS_NEXT_BOOT	   = 0x08
IMAGE_STATUS_WRITABLE	   = 0x10
IMAGE_STATUS_ACTIVE	   = 0x20
IMAGE_STATUS_ERASED	   = 0x40

IMAGE_STATUS_NAME = {
    IMAGE_STATUS_OK: "OK",
    IMAGE_STATUS_BAD_CHECKSUM: "bad checksum",
    IMAGE_STATUS_BAD_TYPE: "bad type",
    IMAGE_STATUS_BAD_SIZE: "bad size",
    IMAGE_STATUS_NEXT_BOOT: "next boot",
    IMAGE_STATUS_WRITABLE: "upgradeable",
    IMAGE_STATUS_ACTIVE: "in use",
    IMAGE_STATUS_ERASED: "erased"
}

IMAGE_REVISION_TYPE = {
    0x00: "branch",
    0x01: "pre",
    0x02: "RC",
    0x03: "LD"
}

IMAGE_DATE_TYPE = {
    0x00: "snapshot",
    0x01: "untagged",
    0x02: "reserved",
    0x03: "reserved"
}

TLV_ERROR_NAME = {
     0: "NO ERROR",
     1: "UNKNOWN VERSION",
     2: "UNKNOWN VARIABLE",
     3: "UNKNOWN INSTANCE",
     4: "UNKNOWN OP CODE",
     5: "UNKNOWN ELEMENT SIZE",
     6: "BAD NUMBER OF ELEMENTS",
     7: "TIMEOUT",
     8: "DEVICE BUSY",
     9: "HARDWARE ERROR",
    10: "BAD LENGTH",
    11: "WRITE ACCESS DENIED",
    12: "UNKNOWN BLOB COMMAND",
    13: "NO VECTOR ACCESS",
    14: "UNEXPECTED RESPONSE",
    15: "INVALID VECTOR OFFSET",
    16: "INVALID ARGUMENT",
    17: "READ ACCESS DENIED",
    18: "UNPROCESSED TLV"
}

# variables in instance 0
VARIABLE_HARDWARE_RESET = 0x0ca
VARIABLE_UNIT_BOOT_TIMER = 0x0c9
VARIABLE_SW_REVISION = 0x0cc
VARIABLE_CHASSIS_CAPABILITIES = 0x0e0
VARIABLE_BOOTLOADER_VERSION = 0x0e1
VARIABLE_TIME_SINCE_LAST_GOOD_UC_RX = 0x0e3
VARIABLE_OLD_SW_REVISION = 0x10c

# variables used for the netselect process
VARIABLE_UNIT_CONTROLLER_WATCHDOG = 0x0c0;
VARIABLE_UNIT_CONTROLLER_STATUS = 0x0c1;
VARIABLE_UNIT_CONTROLLER_ADDRESS = 0x0c2;
VARIABLE_LOCATION_ID = 0x0ce;

# variables used for the radio instance
VARIABLE_RADIO_CHANNEL = 0x100;
VARIABLE_RADIO_PAN_ID = 0x101;
VARIABLE_RADIO_BEACON_RESPONSE = 0x102;
VARIABLE_RADIO_MODE = 0x103;
VARIABLE_RADIO_LINK_LAYER_KEY = 0x200
VARIABLE_RADIO_LINK_LAYER_SECURITY_LEVEL = 0x201

# misc variables for specific devices
VARIABLE_CO2		= 0x100
VARIABLE_TEMPERATURE	= 0x100
VARIABLE_HUMIDITY	= 0x101

VARIABLE_RELAY_READ = 0x100;
VARIABLE_RELAY_WRITE = 0x101;
VARIABLE_GPOUT_OUTPUT = 0x101
VARIABLE_GPOUT_CONFIG = 0x106
VARIABLE_GPOUT_CAPABILITIES = 0x107
VARIABLE_GPIO_INPUT = 0x100
VARIABLE_GPIO_TRIGGER_COUNTER = 0x104

# For the LED control
VARIABLE_NUMBER_OF_LEDS = 0x100
VARIABLE_LED_CONTROL = 0x101
VARIABLE_LED_SET = 0x102
VARIABLE_LED_CLEAR = 0x103
VARIABLE_LED_TOGGLE = 0x104

# For the routing table
VARIABLE_TABLE_LENGTH = 0x100
VARIABLE_TABLE_REVISION = 0x101
VARIABLE_TABLE = 0x102
VARIABLE_NETWORK_ADDRESS = 0x103

# For the network statistics
VARIABLE_NSTATS_DATA = 0x106

# For the energy meter
VARIABLE_TOTAL_ENERGY_CONSUMED = 0x100
VARIABLE_POWER = 0x101
VARIABLE_VOLTAGE = 0x102
VARIABLE_CURRENT = 0x103
VARIABLE_TOTAL_ACTIVE_TIME = 0x104


NULL_TLV = binascii.unhexlify("0000")

class EncapHeader:
    def __init__(self):
        self.Version = 1
        self.PayloadType = 0
        self.Error = 0
        self.FingerPrintMode = 0
        self.InitVectorMode = 0
        self.encapFormat = "!BBBB"
        self.encapSize = struct.calcsize(self.encapFormat)
        self.totSize = self.encapSize
        self.DevID = None
        self.Opt = None
        self.Len = None
        self.data = None
        self.serialData = None
        self.crcOK = None

    def pack(self):
        data = struct.pack(self.encapFormat, self.Version<<4, 
                           self.PayloadType, self.Error,
                           self.FingerPrintMode << 4 | self.InitVectorMode)
        if self.totSize > self.encapSize:
            data += self.data
        if self.Opt == ENC_FINGERPRINT_LENOPT_OPTION_CRC:
            crc32 = binascii.crc32(data) & 0xffffffff
            crc32data = struct.pack("<L", crc32)
            data = data + crc32data
        return data

    def set_serial(self, data):
        self.PayloadType = ENC_PAYLOAD_SERIAL
        self.FingerPrintMode = ENC_FINGERPRINT_LENOPT
        self.Opt = ENC_FINGERPRINT_LENOPT_OPTION_CRC
        optlen = struct.pack("!HH", ENC_FINGERPRINT_LENOPT_OPTION_CRC, len(data))
        self.data = optlen + data
        self.totSize = len(self.data)

    def unpack(self, bindata): 
        size = self.encapSize
        data = struct.unpack_from(self.encapFormat, bindata)
        self.Version = data[0] >> 4
        self.PayloadType = data[1]
        self.Error = data[2]
        self.FingerPrintMode = data[3] >> 4
        self.InitVectorMode = data[3] & 0xf
       
        # if this is with fingerprint mode DEVID then read in the DEVID
        if self.FingerPrintMode == ENC_FINGERPRINT_DEVID:
            self.DevID = bindata[4:12]
            self.data = bindata[4:12]
            size = size + 8

        # LENOPT => 4 bytes data // Shuold also check CRC option!!!
        if self.FingerPrintMode == ENC_FINGERPRINT_LENOPT:
            self.Opt, self.Len= struct.unpack_from("!HH", bindata, 4)
            size = size + 4
            
        if self.PayloadType == ENC_PAYLOAD_SERIAL:
            self.serialData = bindata[size : size + self.Len]
            self.data = bindata[size : size + self.Len]
            self.crc32, = struct.unpack_from("!L", bindata, size + self.Len)
            self.crcOK = binascii.crc32(bindata[0:size + self.Len + 4]) & 0xffffffff == CRC_MAGIC_REMAINDER
#            print "Data: ", binascii.hexlify(self.serialData)
#            print "CRC32", hex(binascii.crc32(bindata[0:size + self.Len]) & 0xffffffff), " <=> ", hex(self.crc32)
#            print "CRCOK:", self.crcOK

        self.totSize = size;

        return size

    def size(self):
        return self.totSize

    def printEncap(self):
        print "  ENC version", self.Version
        print "  ENC type", self.PayloadType
        print "  ENC error", self.Error
        print "  ENC FPMode", self.FingerPrintMode
        print "  ENC IVMode", self.InitVectorMode
        if self.DevID != None:
            print "  ENC DevID", binascii.hexlify(self.DevID)
        if self.Len != None:
            print "  ENC Opt", self.Opt, " Len: ", self.Len
        if self.serialData != None:
            print "  ENC Data:", binascii.hexlify(self.serialData)
        if self.crcOK != None:
            print "  CRC OK:", self.crcOK

# 
# TLVHeader for both regular TLVs and Vector TLVs.
#
class TLVHeader:
    def __init__(self):
        self.Version = 0
        self.Length = 2
        self.Variable = 0
        self.Instance = 0
        self.Opcode = 0
        self.ElementSize = 0
        self.Error = 0
        self.ElementOffset = 0
        self.Elements = 0

        # Internal data
        self.headerFormat = "!BBHBBBB"
        self.vectorFormat = "!LL"
        self.headerSize = struct.calcsize(self.headerFormat)
        self.vectorSize = struct.calcsize(self.vectorFormat)
        self.data = None
        self.Value = None
        self.isNull = False

    def pack(self):
        data = struct.pack(self.headerFormat, 
                           ((self.Version << 4) | (self.Length >> 8)),
                           (self.Length & 0xff), self.Variable, self.Instance,
                           self.Opcode, self.ElementSize, self.Error)
        
        # if vector - add vector data too
        if self.Opcode >= 128:
            data = data + struct.pack(self.vectorFormat,
                                      self.ElementOffset,
                                      self.Elements)
        if self.data:
            data = data + self.data
        return data

    def unpack(self, bindata):
        if len(bindata) == 2 and bindata[0] == '\x00' and bindata[1] == '\x00':
            self.isNull = True
            self.Length
            return 2
        
        size = self.headerSize
        data = struct.unpack_from(self.headerFormat,bindata)
        self.Version = (data[0] >> 4) & 0xff
        self.Length = ((data[0] & 0xf) << 8) | data[1]
        self.Variable = data[2]
        self.Instance = data[3]
        self.Opcode = data[4]
        self.ElementSize = data[5]
        self.Error = data[6]
        
        # if vector - read vector data too
        if self.Opcode >= 128:
            data = struct.unpack_from(self.vectorFormat, bindata, self.headerSize)
            size = size + self.vectorSize
            self.ElementOffset = data[0]
            self.Elements = data[1]
            self.isVector = True

        # if get response set up data in TLV also!
        if self.Opcode == TLV_GET_RESPONSE:
            self.data = bindata[self.size(): self.size() + 4 * (2 ** self.ElementSize)]
            if self.ElementSize == 0:
                self.IntValue, = struct.unpack("!l", self.data)
            if self.ElementSize == 1:
                self.IntValue, = struct.unpack("!q", self.data)
        elif self.Opcode == (TLV_GET_RESPONSE | 128):
            self.data = bindata[self.size(): self.size() +
                             self.Elements * 4 * (2 ** self.ElementSize)]

        if self.Length * 4 > self.size():
            print "**** TLV Warning - data in unexpected OP"
            self.data = bindata[self.size() : self.Length * 4]

        # assing value for backwards compliance
        self.Value = self.data
        return self.size()

    def size(self):
        if self.isNull: return 2
        size = 0;
        # if with data then add data size
        if self.data: size = len(self.data)
        # if with vector then add the extra vector header size
        if self.Opcode >= 128: size = size + self.vectorSize
        return self.headerSize + size

    def printTLV(self):
        printTLV(self)

def parseImageVersion(version):
    vtype = (version >>  6) & 0x03
    incr  =  version & 0x1f
    if version & 0x80000000L:
        # Date based
        year =  (version >> 17) & 0x1fff
        month = ((version >> 13) & 0x0f)
        day =   (version >>  8) & 0x1f
        v = IMAGE_DATE_TYPE.get(vtype, str(vtype)) + "-" + str(year) + str(month).zfill(2) + str(day).zfill(2)
        if incr > 0:
             v += chr(96 + incr)
    else:
        # Revision based
        major = (version >> 24) & 0x7f
        minor = (version >> 16) & 0xff
        patch = (version >>  8) & 0xff
        v = str(major) + "." + str(minor) + "." + str(patch)
        if vtype != 3 or incr > 0:
            v += IMAGE_REVISION_TYPE.get(vtype, str(vtype))
        if incr > 0:
            v += str(incr)
    return v

def parseEncap(data):
    encap = EncapHeader()
    encap.unpack(data)
    return encap

def parseTLVs(data):
    tlvs = []
    if DEBUG: print "Received TLV: ", binascii.hexlify(data)
    while len(data) > 0:
        tlv = parseTLV(data)
        tlvs = tlvs + [tlv]
        data = data[tlv.size():]
        # Assume end of TLVs when less than 3 bytes there...
        if len(data) < 3:
#            print "Found zero TLV??? with len:", len(data)
            break
    return tlvs

def parseTLV(data):
    tlv = TLVHeader()
    tlv.unpack(data)
    return tlv


def createEncap(tlv):
    e = EncapHeader()
    e.Version = 1
    e.PayloadType = ENC_PAYLOAD_TLV
    encHeader = e.pack()
    if DEBUG: print "Encap: ", binascii.hexlify(encHeader)
    if type(tlv) == list:
        tlvHeader = ""
        for t in tlv:
            tlvHeader = tlvHeader + t.pack();
    else:
        tlvHeader = tlv.pack();
    if DEBUG: print "TLV: ", binascii.hexlify(tlvHeader)
    data = encHeader + tlvHeader + NULL_TLV
    return data

def createTLV(op, instance, var, elementSize, data):
    tlv = TLVHeader()
    tlv.Opcode = op
    tlv.Instance = instance
    tlv.Variable = var
    tlv.ElementSize = elementSize
    tlv.Version = 0
# this needs to be checked against data's size
# length might in that case increase!
    if data: tlv.Length = 2 + len(data) / 4
    else: tlv.Length = 2
    tlv.data = data
    return tlv

def createVectorTLV(op, instance, var, elementSize, offset, elementCount, data):
    tlv = TLVHeader()
    tlv.Opcode = op | 128
    tlv.Instance = instance
    tlv.Variable = var
    tlv.ElementSize = elementSize
    tlv.ElementOffset = offset
    tlv.Elements = elementCount
# this needs to be checked against data's size
# length might in that case increase!
    if data: tlv.Length = 4 + len(data) / 4
    else: tlv.Length = 4
    tlv.data = data
    return tlv


def sendTLV(intlv, host=UDP_IP, port=UDP_PORT, timeout=2.0, showError=True):
    data = createEncap(intlv)
    # This is a very quick hack to detect IPv6 address (given by xx:yy::...)
    if ":" in host:
        sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
    else:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    if DEBUG:
        print "sending: ", binascii.hexlify(data)
    sock.settimeout(timeout)
    try:
        sock.sendto(data, (host, port))
        data, addr = sock.recvfrom(1024)
    except IOError:
        sock.sendto(data, (host, port))
        data, addr = sock.recvfrom(1024)
    if DEBUG: print "Received: ", binascii.hexlify(data)
    enc = parseEncap(data)
    tlvs = parseTLVs(data[enc.size():])
    tlv = tlvs[0]
    if showError and tlv.Error != 0:
        print "  TLV Error",tlv.Error,"(" + TLV_ERROR_NAME.get(tlv.Error, str(tlv.Error)) + ") - TLV Data:", binascii.hexlify(data[enc.size():])
        if hasattr(intlv, 'Opcode') and hasattr(intlv, 'Instance'):
            print "  for op",intlv.Opcode,"instance",intlv.Instance,"variable",intlv.Variable
        else:
            print "  for",intlv
    if showError and (tlv.Error > 17 or DEBUG):
        print "  received raw: ", binascii.hexlify(data)
        print "  raw TLV: ", binascii.hexlify(data[enc.size():])
        print "  received: ", enc.size(), tlv.size(), data[enc.size() + tlv.size():]
        print "  -----------"
        enc.printEncap()
        print "  -----------"
        printTLV(tlv)
    return enc, tlvs

def printTLV(tlv):
    if tlv.isNull:
        print "NULL TLV"
        return
    print "  TLV vers:", tlv.Version, " Op: ", tlv.Opcode, " instance:", tlv.Instance, "  var:%x"%(tlv.Variable)
    print "      element size:", tlv.ElementSize, " length: ", tlv.Length
    print "      error:", tlv.Error
    if hasattr(tlv, 'HeaderLen'):
        print "      headerLen:", tlv.size()
    if hasattr(tlv, 'IntValue'):
        print "  value:", tlv.Value, tlv.IntValue, "0x%x"%tlv.IntValue
    elif hasattr(tlv, 'Value'):
        if tlv.Value:
            print "  value:", tlv.Value, binascii.hexlify(tlv.Value)
        else:
            print "  value:", tlv.Value
    if tlv.Opcode >= 128:
        print "  ElementOffset:", tlv.ElementOffset
        print "  Elements:", tlv.Elements

def convert_sht20_temperature(raw_value):
    # Ignore CRC for now
    temperature = raw_value >> 8
    return -46.85 + 175.72 * float(temperature) / 65536

def convert_sht20_humidity(rawHumidity):
    # Ignore CRC for now
    humidity = rawHumidity >> 8
    # Convert to relative humidity above liquid water
    return -6 + 125 * float(humidity) / 65536

def convert_sht20_humidity_over_ice(rawHumidity, temperature):
    # Ignore CRC for now
    humidity = rawHumidity >> 8
    # Convert to relative humidity above liquid water
    rhw = -6 + 125 * float(humidity) / 65536
    # Convert to relative humidity above ice
    rhi = rhw * exp((17.62 * temperature) / (234.12 + temperature)) / exp((22.46 * temperature) / (272.62 + temperature))
    return rhi

def convert_ieee64_time(time):
    seconds = (time >> 32) & 0xffffffff
    nanoseconds = time & 0xffffffff
    return (seconds, nanoseconds)

def convertIEEE64TimeToString(elapsed):
    seconds = (elapsed >> 32) & 0xffffffff
    nanoseconds = elapsed & 0xffffffff
    years = seconds // (365 * 24 * 60 * 60)
    seconds = seconds % (365 * 24 * 60 * 60)
    days = seconds // (24 * 60 * 60)
    seconds = seconds % (24 * 60 * 60)
    hours = seconds // (60 * 60)
    seconds = seconds % (60 * 60)
    minutes = seconds // (60)
    seconds = seconds % (60)
    milliseconds = nanoseconds // 1000000
    nanoseconds = nanoseconds % 1000000
    time = ""
    if years > 0 or time != "":
        time = str(years) + " years "
    if days > 0 or time != "":
        time += str(days) + " days "
    if hours > 0 or time != "":
        time += str(hours) + " hours "
    if minutes > 0 or time != "":
        time += str(minutes) + " min "
    time += str(seconds) + " sec " + str(milliseconds) + " msec"
    if nanoseconds > 0:
        time += " " + str(nanoseconds) + " nanoseconds"
    return time

def getStartIEEE64TimeAsString(elapsed):
    seconds = (elapsed >> 32) & 0xffffffff
    startTime = time.time() - seconds
    return time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(startTime))

def getTLVErrorAsString(error):
    return TLV_ERROR_NAME.get(error, str(error))

def convertString(text):
    i = string.find(text, "\0")
    if i >= 0:
        return text[0:i]
    return text

def decodevalue(x):
    if x.startswith("0x"):
        return int(x[2:], 16)
    if x.startswith("#") or x.startswith("$"):
        return int(x[1:], 16)
    if x.startswith("0") and len(x) > 1:
        return int(x[1:], 8)
    return int(x)

def find_instance_with_type(host, product_type, verbose = False):
    t = createTLV(TLV_GET_REQUEST, 0, VARIABLE_NUMBER_OF_INSTANCES, 0, None)
    enc,tlvs = sendTLV(t, host)
    num_instances = tlvs[0].IntValue
    if verbose:
        print "Searching through",num_instances,"instances"
    t = []
    for i in range(num_instances):
        t.append(createTLV(TLV_GET_REQUEST, i + 1, VARIABLE_PRODUCT_TYPE, 1, None))
    enc,tlvs = sendTLV(t, host, showError = False)
    for i in range(num_instances):
        if verbose:
            print "\tInstance",(i + 1)," %016x"%tlvs[i].IntValue
        if tlvs[i].IntValue == product_type:
            return i + 1
    return None

def discovery(host):
    # get product type
    t1 = createTLV(TLV_GET_REQUEST, 0, VARIABLE_PRODUCT_TYPE, 1, None)
    # get product label
    t2 = createTLV(TLV_GET_REQUEST, 0, VARIABLE_PRODUCT_LABEL, 3, None)
    # get number of instances
    t3 = createTLV(TLV_GET_REQUEST, 0, VARIABLE_NUMBER_OF_INSTANCES, 0, None)
    # get the boot timer
    t4 = createTLV(TLV_GET_REQUEST, 0, VARIABLE_UNIT_BOOT_TIMER, 1, None)
    enc,tlvs = sendTLV([t1,t2,t3,t4], host)
    productType = tlvs[0].IntValue
    productLabel = convertString(tlvs[1].Value)
    numInstances = tlvs[2].IntValue
    bootTimer = tlvs[3].IntValue
    instances = []
    for i in range(numInstances):
        t1 = createTLV(TLV_GET_REQUEST, i + 1, VARIABLE_PRODUCT_TYPE, 1, None)
        t2 = createTLV(TLV_GET_REQUEST, i + 1, VARIABLE_PRODUCT_LABEL, 3, None)
        enc,tlvs = sendTLV([t1,t2], host, showError = False)
        instanceType = 0xffffffffffffffffL
        instanceLabel = "<failed to discover>"
        if tlvs[0].Error == 0:
            instanceType = tlvs[0].IntValue
        if tlvs[1].Error == 0:
            instanceLabel = convertString(tlvs[1].Value)
        instances = instances + [(instanceType, instanceLabel, i + 1)]
    return ((productLabel,productType,bootTimer), instances)
