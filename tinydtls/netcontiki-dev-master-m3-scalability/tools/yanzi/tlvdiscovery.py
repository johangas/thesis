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
# TLV Discovery
# A small discovery tool for devices/applications using Yanzi Application Layer
#

import tlvlib, sys, struct
import binascii

verbose = False
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
print "Product label:", d[0][0], " type: %016x"%d[0][1], " instances:", len(d[1])
print "Booted at:",tlvlib.getStartIEEE64TimeAsString(d[0][2]),"-",tlvlib.convertIEEE64TimeToString(d[0][2])

if verbose:
    t1 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, 0,
                          tlvlib.VARIABLE_SW_REVISION, 2, None)
    t2 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, 0,
                          tlvlib.VARIABLE_BOOTLOADER_VERSION, 0, None)
    t3 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, 0,
                          tlvlib.VARIABLE_CHASSIS_CAPABILITIES, 0, None)
    enc, tlvs = tlvlib.sendTLV([t1,t2,t3], host)
    revision = ""
    if tlvs[0].Error == 0:
        revision += "SW revision: " + tlvlib.convertString(tlvs[0].Value)
    if tlvs[1].Error == 0:
        revision += "\tBootloader: " + str(tlvs[1].IntValue)
    if tlvs[2].Error == 0:
        revision += " %016x"%tlvs[2].IntValue
    if revision != "":
        print revision

if verbose:
    t0 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, 0,
                          tlvlib.VARIABLE_LOCATION_ID, 0, None)
    enc, tlvs = tlvlib.sendTLV([t0], host, showError=False)
    if tlvs[0].Error == 0:
        print "Location id:", tlvs[0].IntValue
    elif tlvs[0].Error == 9:
        print "Location id: not set"
    elif tlvs[0].Error != 2:
        print "Location id: #error",tlvlib.getTLVErrorAsString(tlvs[0].Error)

i = 1
for data in d[1]:
    print "Instance " + str(data[2]) + ": type: %016x"%data[0], " ", data[1]
    if instance > 0 and instance != i:
        True
    elif data[0] == tlvlib.IMAGE and verbose:
        t1 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                              tlvlib.VARIABLE_IMAGE_VERSION, 1, None)
        t2 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                             tlvlib.VARIABLE_IMAGE_STATUS, 0, None)
        t3 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                              tlvlib.VARIABLE_IMAGE_TYPE, 1, None)
        enc, tlvs = tlvlib.sendTLV([t1,t2,t3], host)
        tlv = tlvs[0]
        tlvstatus = tlvs[1]
        print "\tVersion:    %016x"%tlv.IntValue, "  ", tlvlib.parseImageVersion(tlv.IntValue), "inc:", str(tlv.IntValue & 0x1f)
        print "\tImage Type: %016x"%tlvs[2].IntValue,"   Status:", tlvlib.IMAGE_STATUS_NAME.get(tlvstatus.IntValue, tlvstatus.IntValue)
    elif data[0] == tlvlib.LEDS:
        t1 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                              tlvlib.VARIABLE_LED_CONTROL, 0, None)
        t2 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                              tlvlib.VARIABLE_NUMBER_OF_LEDS, 0, None)
        enc, tlvs = tlvlib.sendTLV([t1,t2], host)
        print "\tleds are set to 0x%02x"%tlvs[0].IntValue,"(" + str(tlvs[1].IntValue) + " user leds)"

    elif data[0] == tlvlib.BUTTON:
        t1 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                              tlvlib.VARIABLE_GPIO_INPUT, 0, None)
        t2 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                              tlvlib.VARIABLE_GPIO_TRIGGER_COUNTER, 0, None)
        enc, tlvs = tlvlib.sendTLV([t1,t2], host)
        if tlvs[0].IntValue == 0:
            print "\tbutton is not pressed, has been pressed",tlvs[1].IntValue,"times"
        else:
            print "\tbutton is pressed, has been pressed",tlvs[1].IntValue,"times"
        if verbose:
            # Arm instance 0 and rearm the button instance
            t0 = tlvlib.createVectorTLV(tlvlib.TLV_SET_REQUEST, 0,
                                        tlvlib.VARIABLE_EVENT_ARRAY, 0, 0, 1,
                                        struct.pack("!L", 1))
            t1 = tlvlib.createVectorTLV(tlvlib.TLV_SET_REQUEST, i,
                                        tlvlib.VARIABLE_EVENT_ARRAY, 0, 0, 2,
                                        struct.pack("!LL", 1, 2))
            enc, tlvs = tlvlib.sendTLV([t0,t1], host)

    elif data[0] == tlvlib.GPIO:
        t = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                             tlvlib.VARIABLE_GPOUT_OUTPUT, 0, None)
        enc, tlvs = tlvlib.sendTLV(t, host)
        tlv = tlvs[0]
        print "\tgpout set to", tlv.IntValue
        t1 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                              tlvlib.VARIABLE_GPOUT_CONFIG, 0, None)
        t2 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                              tlvlib.VARIABLE_GPOUT_CAPABILITIES, 1, None)
        enc, tlvs = tlvlib.sendTLV([t1,t2], host, showError=False)
        if tlvs[0].Error == 0:
            print "\tgpout start mode is 0x%02x"%tlvs[0].IntValue + "    Capabilities: 0x%016x"%tlvs[1].IntValue

    elif data[0] == tlvlib.SHT20:
        t1 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                              tlvlib.VARIABLE_TEMPERATURE, 0, None)
        t2 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                              tlvlib.VARIABLE_HUMIDITY, 0, None)
        enc, tlvs = tlvlib.sendTLV([t1,t2], host)
        temperature = tlvlib.convert_sht20_temperature(tlvs[0].IntValue)
        raw_humidity = tlvs[1].IntValue
        humidity = tlvlib.convert_sht20_humidity(raw_humidity)
        humidityI = tlvlib.convert_sht20_humidity_over_ice(raw_humidity, temperature)
        print "\tTemperature:",round(temperature, 3),"(C)"
        print "\tHumidity   : " + str(round(humidity, 3)) + "%  Humidity (I): " + str(round(humidityI,3)) + "%"
    elif data[0] == tlvlib.TEMPHUM_GENERIC:
        t1 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                              tlvlib.VARIABLE_TEMPERATURE, 0, None)
        t2 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                              tlvlib.VARIABLE_HUMIDITY, 0, None)
        enc, tlvs = tlvlib.sendTLV([t1,t2], host)
        temperature = tlvs[0].IntValue
        humidity = tlvs[1].IntValue
        if temperature > 100000:
            temperature = (temperature - 273150) / 1000.0
        print "\tTemperature:",round(temperature, 3),"(C)"
        humidity = humidity / 10000.0
        print "\tHumidity   :",str(round(humidity, 3)) + "%"
    elif data[0] == tlvlib.TEMP_GENERIC:
        t1 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                              tlvlib.VARIABLE_TEMPERATURE, 0, None)
        enc, tlvs = tlvlib.sendTLV([t1], host)
        if tlvs[0].Error == 0:
            temperature = tlvs[0].IntValue
            if temperature > 100000:
                temperature = (temperature - 273150) / 1000.0
            print "\tTemperature:",round(temperature, 3),"(C)"
    elif data[0] == tlvlib.ENERGY_METER:
        t1 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                              tlvlib.VARIABLE_POWER, 0, None)
        t2 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                              tlvlib.VARIABLE_VOLTAGE, 0, None)
        t3 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                              tlvlib.VARIABLE_CURRENT, 0, None)
        t4 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                              tlvlib.VARIABLE_TOTAL_ACTIVE_TIME, 0, None)
        t5 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                              tlvlib.VARIABLE_TOTAL_ENERGY_CONSUMED, 1, None)
        enc, tlvs = tlvlib.sendTLV([t1,t2,t3,t4,t5], host, showError=False)
        tlvpower = tlvs[0]
        tlvvoltage = tlvs[1]
        print "\tPower:             %8.2f (W)"%(tlvs[0].IntValue / 1000.0)
        print "\tVoltage:           %8.2f (V)    Current: %8.2f (A)"%(tlvs[1].IntValue / 1000.0,tlvs[2].IntValue / 1000.0)
        if tlvs[3].Error == 0:
            print "\tTotal active time: %8d (sec)"%(tlvs[3].IntValue)
        print "\tTotal consumed: %11.2f (Ws)"%(tlvs[4].IntValue / 1000.0)
    elif data[0] == tlvlib.CO2:
        t = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                             tlvlib.VARIABLE_CO2, 0, None)
        enc, tlvs = tlvlib.sendTLV(t, host)
        tlv = tlvs[0]
        print "\tCO2:",tlv.IntValue,"(ppm)"
    elif data[0] == tlvlib.RADIO:
        t1 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                             tlvlib.VARIABLE_RADIO_PAN_ID, 0, None)
        t2 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                             tlvlib.VARIABLE_RADIO_CHANNEL, 0, None)
        t3 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                             tlvlib.VARIABLE_RADIO_MODE, 0, None)

        enc, tlvs = tlvlib.sendTLV([t1,t2,t3], host)
        print "\tPan id:",tlvs[0].IntValue,"(0x%04x)"%tlvs[0].IntValue," Channel:",tlvs[1].IntValue,"  Mode:",tlvs[2].IntValue

    elif data[0] == tlvlib.ROUTER:
        t1 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                              tlvlib.VARIABLE_TABLE_LENGTH, 0, None)
        t2 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                              tlvlib.VARIABLE_TABLE_REVISION, 0, None)
        t3 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                              tlvlib.VARIABLE_NETWORK_ADDRESS, 2, None)
        enc, tlvs = tlvlib.sendTLV([t1,t2, t3], host)
        tlv = tlvs[0]
        print "\tNetwork address: 0x" + binascii.hexlify(tlvs[2].Value)
        print "\tRouting table size:",tlv.IntValue," revision:",tlvs[1].IntValue
        if tlv.IntValue > 0:
            t = tlvlib.createVectorTLV(tlvlib.TLV_GET_REQUEST, i,
                                       tlvlib.VARIABLE_TABLE, 4, 0,
                                       tlv.IntValue, None)
            enc, tlvs = tlvlib.sendTLV(t, host)
            tlv = tlvs[0]
            for r in range(0, tlv.Elements):
                o = r * 64
                import socket
                IPv6Str = socket.inet_ntop(socket.AF_INET6, tlv.Value[o:o+16])
                IPv6LLStr = socket.inet_ntop(socket.AF_INET6, tlv.Value[o+16:o+32])
                print "\t",(r + 1), IPv6Str, "->", IPv6LLStr
    elif data[0] == tlvlib.MOTION_GENERIC:
        t = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i, 0x100, 2, None)
        enc,tlvs = tlvlib.sendTLV(t, host)
        time, = struct.unpack("!q", tlvs[0].Value[0:8])
        count, = struct.unpack("!q", tlvs[0].Value[8:])
        print "\tMotion:",count,"Time:",tlvlib.convertIEEE64TimeToString(time)
        if verbose:
            # Arm instance 0 and rearm the motion instance
            t0 = tlvlib.createVectorTLV(tlvlib.TLV_SET_REQUEST, 0,
                                        tlvlib.VARIABLE_EVENT_ARRAY, 0, 0, 1,
                                        struct.pack("!L", 1))
            t1 = tlvlib.createVectorTLV(tlvlib.TLV_SET_REQUEST, i,
                                        tlvlib.VARIABLE_EVENT_ARRAY, 0, 0, 2,
                                        struct.pack("!LL", 1, 2))
            enc, tlvs = tlvlib.sendTLV([t0,t1], host)

    elif data[0] == tlvlib.RANGING_GENERIC:
        t1 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i, 0x101, 0, None)
        t2 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i, 0x102, 0, None)
        enc,tlvs = tlvlib.sendTLV([t1,t2], host)
        print "\tDistance:",tlvs[1].IntValue,"mm\tStatus:",tlvs[0].IntValue
    elif data[0] == tlvlib.PTCTEMP:
        t = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i, 0x100, 0, None)
        enc,tlvs = tlvlib.sendTLV(t, host)
        if tlvs[0].Error == 0:
            print "\tTemperature:",((tlvs[0].IntValue - 273150) / 1000.0),"(C)"
    elif data[0] == tlvlib.POWER_SINGLE:
        t1 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i, 0x100, 0, None)
        t2 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i, 0x102, 3, None)
        t3 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i, 0x105, 0, None)
        t4 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i, 0x107, 0, None)
        enc,tlvs = tlvlib.sendTLV([t1,t2,t3,t4], host)
        print "\tPower source: ",tlvlib.convertString(tlvs[1].Value),"(%d"%tlvs[0].IntValue,"power sources)"
        print "\tPower voltage:",(tlvs[2].IntValue / 1000.0),"V\tTemperature:",((tlvs[3].IntValue - 273150)/1000.0),"(C)"
    elif data[0] == tlvlib.LAMP:
        t1 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i, 0x100, 0, None)
        t2 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i, 0x101, 0, None)
        enc,tlvs = tlvlib.sendTLV([t1,t2], host)
        print "\tIntensity: 0x%x/%d"%(tlvs[0].IntValue & 0xffffffffL,tlvs[1].IntValue)
    elif data[0] == tlvlib.NETWORK_STATISTICS:
        t1 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i, 0x100, 0, None)
        t2 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i, 0x101, 1, None)
        t3 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i, 0x102, 0, None)
        t4 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i, 0x103, 0, None)
        t5 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i, 0x104, 0, None)
        enc,tlvs = tlvlib.sendTLV([t1,t2,t3,t4,t5], host)
        version = tlvs[0].IntValue
        capabilities = tlvs[1].IntValue
        pushPeriod = tlvs[2].IntValue
        pushTime = tlvs[3].IntValue
        pushPort = tlvs[4].IntValue
        print "\tVersion:",version,"\t\tCapabilities: 0x%x"%capabilities
        print "\tPush period:",pushPeriod,"sec\tRemaining:",pushTime,"sec\tTo port:",pushPort

        t1 = tlvlib.createVectorTLV(tlvlib.TLV_GET_REQUEST, i, 0x106, 0, 0, 64, None)
        enc,tlvs = tlvlib.sendTLV([t1], host)
#        print "\tDefault data: Elements", tlvs[0].Elements, " ElementSize", tlvs[0].ElementSize
#        print "\tDefault data: 0x" + binascii.hexlify(tlvs[0].Value)

        dataVersion, = struct.unpack("!B", tlvs[0].Value[0:1])
        dataCount, = struct.unpack("!B", tlvs[0].Value[1:2])
        dataType, = struct.unpack("!B", tlvs[0].Value[2:3])
        if dataVersion != 0:
            print "\tUnsupported data version:",dataVersion
        elif dataCount == 0:
            print "\tNo data"
        elif dataType != 1:
            print "\tUnsupported data type ",dataType
        else:
            seqno, = struct.unpack("!B", tlvs[0].Value[3:4])
            freeRoutes, = struct.unpack("!B", tlvs[0].Value[4:5])
            freeNeighbors, = struct.unpack("!B", tlvs[0].Value[5:6])
            churn, = struct.unpack("!B", tlvs[0].Value[6:7])
            parent = tlvs[0].Value[7:11]
            rank, = struct.unpack("!H", tlvs[0].Value[11:13])
            rtmetric, = struct.unpack("!H", tlvs[0].Value[13:15])
            myrank, = struct.unpack("!H", tlvs[0].Value[15:17])
            print "\tRPL Rank:", myrank," Churn:",churn," Free neighbors:",freeNeighbors," Free routes:",freeRoutes
            print "\tDefault route:","0x" + binascii.hexlify(parent)," link-metric:",rtmetric," rank:",rank

    elif data[0] == tlvlib.BORDER_ROUTER_MANAGEMENT:
        t1 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i, 0x100, 3, None)
        t2 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i, 0x101, 3, None)
        t3 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i, 0x105, 2, None)
        t4 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i, 0x106, 0, None)
        enc,tlvs = tlvlib.sendTLV([t1,t2,t3,t4], host)
        interfaceNameLabel = tlvs[0].Value
        serialNameLabel = tlvs[1].Value
        print "\tSerial device name:", serialNameLabel,"  Interface device name:", interfaceNameLabel
        if hasattr(tlvs[2], 'Value'):
            if hasattr(tlvs[3], 'IntValue'):
                print "\tRadio SW revision:",tlvlib.convertString(tlvs[2].Value),"\tBootloader version:",tlvs[3].IntValue
            else:
                print "\tRadio SW revision:",tlvlib.convertString(tlvs[2].Value)
    i = i + 1
