#!/usr/bin/python
#
# Copyright (c) 2015, Yanzi Networks AB.
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
# Grab nodes using Yanzi Application Layer and keep them in the network
#

import tlvlib, nstatslib, sys, struct, binascii, socket, time, threading
import subprocess, re

EVENT_DISCOVERY = "discovery"
EVENT_BUTTON = "button"
EVENT_NSTATS_RPL = "nstats-rpl"

class Device:
    product_type = None
    device_info = None
    label = 'unknown'
    boot_time = 0
    address = ""

    button_instance = None
    leds_instance = None
    temperature_instance = None
    nstats_instance = None

    nstats_rpl = None

    next_fetch = 0
    fetch_tries = 0

    next_update = 0
    update_tries = 0
    discovery_tries = 0

    def __init__(self, device_address):
        self.address = device_address
        self.last_seen = 0
        self.last_ping = 0

    def is_discovered(self):
        return self.product_type is not None

    def get_instance(self, instance_type):
        if self.device_info:
            i = 1
            for data in self.device_info[1]:
                if data[0] == instance_type:
                    return i
                i += 1
        return None

    def __str__(self):
        return "Device(" + self.address + ")"

# DeviceEvent used for different callbacks
class DeviceEvent:

    def __init__(self, device = None, event_type = None, event_data = None):
        self.device = device
        self.event_type = event_type
        self.event_data = event_data

    def __str__(self):
        addr = None
        if self.device:
            addr = self.device.address
        return "DeviceEvent(" + `addr` + "," + `self.event_type` + "," + `self.event_data` + ")"

class DeviceServer:
    _devices = {}
    _callbacks = []
    _sock = None

    device_server_host = None
    router_host = "localhost"
    router_address = None
    router_prefix = None
    router_instance = None
    brm_instance = None
    nstats_instance = None
    radio_instance = None
    radio_channel = 26
    radio_panid = 0xabcd

    udp_address = "aaaa::1"
    udp_port = 4444

    # The open range is 7000 - 7999
    # This is what is announced in the beacon
    location = 7000
    # This is what is set when grabbing the node
    grab_location = 0

    # watchdog time in seconds
    watchdog_time = 600
    guard_time = 300

    fetch_time = 120

    def send_event(self, device_event):
        print "Sending event:", device_event
        for callback in self._callbacks[:]:
#            print "Callback to:", callback
            try:
                callback(device_event)
            except Exception as e:
                print "*** callback error:", e

    def add_event_listener(self, callback):
        self._callbacks.append(callback)

    def remove_event_listener(self, callback):
        self._callbacks.remove(callback)

    # 24-bit reserved, 8-bit type = 02
    # 16-bit reserved, 16-bit port
    # 16-byte IPv6 address
    # 4-byte location ID
    # 4-byte reserved
    def grab_device(self, address):
        IPv6Str = binascii.hexlify(socket.inet_pton(socket.AF_INET6, self.udp_address))
        payloadStr = "000000020000%04x"%self.udp_port + IPv6Str + "%08x"%self.grab_location + "00000000"
        payload = binascii.unhexlify(payloadStr)
        print "Grabbing device", address, " => ", payloadStr, len(payload)
        t1 = tlvlib.createTLV(tlvlib.TLV_SET_REQUEST, 0 ,
                              tlvlib.VARIABLE_UNIT_CONTROLLER_ADDRESS, 3,
                              payload)
        t2 = tlvlib.createTLV(tlvlib.TLV_SET_REQUEST, 0,
                              tlvlib.VARIABLE_UNIT_CONTROLLER_WATCHDOG, 0,
                              struct.pack("!L", self.watchdog_time))
        try:
            enc,tlvs = tlvlib.sendTLV([t1,t2], address)
            return tlvs
        except socket.timeout:
            print "Failed to grab device (time out)"
            return None

    def arm_device(self, address, instances):
        tlvs = [tlvlib.createVectorTLV(tlvlib.TLV_SET_REQUEST, 0,
                                       tlvlib.VARIABLE_EVENT_ARRAY, 0, 0, 1,
                                       struct.pack("!L", 1))]
        if type(instances) == list:
            for i in instances:
                t = tlvlib.createVectorTLV(tlvlib.TLV_SET_REQUEST, i,
                                           tlvlib.VARIABLE_EVENT_ARRAY, 0, 0, 2,
                                           struct.pack("!LL", 1, 2))
                tlvs.append(t)
        else:
            t = tlvlib.createVectorTLV(tlvlib.TLV_SET_REQUEST, instances,
                                       tlvlib.VARIABLE_EVENT_ARRAY, 0, 0, 2,
                                       struct.pack("!LL", 1, 2))
            tlvs.append(t)

        try:
            enc, tlvs = tlvlib.sendTLV(tlvs, address)
            return True
        except socket.timeout:
            print "Failed to arm device (time out)"
            return False

    def set_location(self, address, location):
        print "Setting location on", address, " => ", location
        t = tlvlib.createTLV(tlvlib.TLV_SET_REQUEST, 0 ,
                             tlvlib.VARIABLE_LOCATION_ID, 0,
                             struct.pack("!L", location))
        enc,tlvs = tlvlib.sendTLV(t, address)
        return tlvs

    def discovery(self, dev):
        dev.discovery_tries = dev.discovery_tries + 1
        try:
            print "Trying to TLV discover device:", dev.address
            dev.device_info = tlvlib.discovery(dev.address)
            dev.product_type = dev.device_info[0][1]
            dev.label = dev.device_info[0][0]
            print "\tFound: ", dev.device_info[0][0], "Type: 0x%016x"%dev.product_type
            seconds,nanoseconds = tlvlib.convert_ieee64_time(dev.device_info[0][2])
            dev.boot_time = time.time() - seconds

            i = 1
            for data in dev.device_info[1]:
                if data[0] == tlvlib.BUTTON:
                    dev.button_instance = i
                elif data[0] == tlvlib.LEDS:
                    dev.leds_instance = i
                elif data[0] == tlvlib.TEMP_GENERIC:
                    dev.temperature_instance = i
                elif data[0] == tlvlib.NETWORK_STATISTICS:
                    dev.nstats_instance = i
                i += 1

            if dev.next_update == 0:
                if self.grab_device(dev.address):
                    dev.next_update = time.time() + self.watchdog_time - self.guard_time
            if dev.button_instance:
                print "Arming device",dev.address
                self.arm_device(dev.address, dev.button_instance)

            de = DeviceEvent(dev, EVENT_DISCOVERY)
            self.send_event(de)
        except Exception as e:
            print e

    def ping(self, dev):
        print "Pinging: ", dev.address, " to check liveness..."
        p=subprocess.Popen(["ping6", "-c", "1", "-W", "2", dev.address],
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        try:
            while True:
                line = p.stdout.readline()
                if line == '': break
                print line
        except Exception as e:
            print e
            print "Ping Unexpected error:", sys.exc_info()[0]
        p.wait()
        dev.last_ping = time.time()
        if p.returncode == 0:
            dev.last_seen = time.time()
        if p.returncode == None: 
            p.terminate()

    # ----------------------------------------------------------------
    # manage devices
    # this will maintain the devices - keep the "grabbed" by updating the
    # watchdog timer - Note: might need to keep a add/delete list to aovid
    # messing with the same lists when devices need to be added/deleted.
    # ----------------------------------------------------------------
    def _manage_devices(self):
        while True:
            current_time = time.time()
            remove_list = []
            for dev in self.get_devices():
                # Do a live check if needed...
                if dev.last_seen + 60 < current_time and dev.last_ping + 60 < current_time:
                    self.ping(dev)

                # Check if there is need for discovery
                if not dev.is_discovered():
                    if dev.discovery_tries < 5:
                        self.discovery(dev)
                    else:
                        dev.discovery_tries = dev.discovery_tries + 1

                    if not dev.is_discovered():
                        # Remove non-discoverable devices after some time
                        # so they can be tried again
                        if dev.discovery_tries > 180:
                            remove_list.append(dev)
                        continue

                # Check if there is need for WDT update
                if current_time > dev.next_update:
                    print "Updating WDT in", dev.address
                    dev.update_tries += 1
                    t1 = tlvlib.createTLV(tlvlib.TLV_SET_REQUEST, 0,
                                          tlvlib.VARIABLE_UNIT_CONTROLLER_WATCHDOG,
                                          0,
                                          struct.pack("!L", self.watchdog_time))
                    try:
                        enc,tlvs = tlvlib.sendTLV(t1, dev.address)
                        if tlvs[0].Error == 0:
                            dev.next_update = current_time + self.watchdog_time - self.guard_time
                            dev.update_tries = 0
                        else:
                            print "Failed to update WDT, trying to grab"
                            if self.grab_device(dev.address):
                                dev.next_update = current_time + self.watchdog_time - self.guard_time
                                dev.update_tries = 0

                    except Exception as e:
                        print "Failed to update WDT in", dev.address
                        print e
                        if dev.update_tries > 10:
                            remove_list.append(dev)

                if current_time >= dev.next_fetch:
                    dev.fetch_tries += 1
                    if self._fetch_periodic(dev):
                        dev.fetch_tries = 0
                        dev.next_fetch = time.time() + self.fetch_time
                    else:
                        print "*** failed to fetch from", dev.address,"("+str(dev.fetch_tries)+")"
                        # Try again a little later
                        dev.next_fetch = time.time() + 10 * dev.fetch_tries

            for dev in remove_list:
                print "Removing device", dev.address
                self.remove_device(dev.address)

            time.sleep(1)


    def get_devices(self):
        return list(self._devices.values())

    def get_device(self, addr):
        if addr in self._devices:
            return self._devices[addr]
        return None

    # adds a device to the grabbed devices list
    def add_device(self, addr):
        d = self.get_device(addr)
        if d is not None:
            return d;
        print "Adding device",addr

        d = Device(addr)
        # to avoid pinging immediately 
        d.last_ping = time.time()
        self._devices[addr] = d
        return d

    def remove_device(self, addr):
        del self._devices[addr]

    def fetch_nstats(self):
        t = threading.Thread(target=self._fetch_nstats)
        t.daemon = True
        t.start()

    def _fetch_nstats(self):
        for dev in self.get_devices():
            if dev.nstats_instance:
                # The device has the network instance
                print "Fetching network statistics from", dev.address
                try:
                    nstats = nstatslib.fetch_nstats(dev.address, dev.nstats_instance)
                    if nstats:
                        self._handle_nstats(dev, nstats)
                    else:
                        print "*** Failed to fetch network statistics from", dev.address
                except Exception as e:
                    print "**** Failed to fetch network statistics:", e
                time.sleep(0.5)

    def _handle_nstats(self, device, nstats):
        rpl = nstats.get_data_by_type(nstatslib.NSTATS_TYPE_RPL)
        if rpl:
            device.nstats_rpl = rpl
            de = DeviceEvent(device, EVENT_NSTATS_RPL, rpl)
            self.send_event(de)

    def _lookup_device_host(self, prefix, default_host):
        try:
            output = subprocess.check_output('ifconfig | grep inet6 | grep -v " fe80" | grep -v " ::1"', shell=True)
            # Check prefix first
            p = re.compile(" (" + prefix + "[a-fA-F0-9:]+)/")
            m = p.search(output)
            if m:
                return m.group(1)

            p = re.compile(" ([a-fA-F0-9:]+)/")
            m = p.search(output)
            if m:
                return m.group(1)
            else:
                print "----------"
                print "ERROR: Failed to lookup device host address:"
                print output
                print "----------"
        except Exception as e:
            print "----------"
            print e
            print "ERROR: Failed to lookup device host address:", sys.exc_info()[0]
            print "----------"
            return default_host

    def _fetch_periodic(self, device):
        t = []
        t.append(tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, 0, tlvlib.VARIABLE_UNIT_BOOT_TIMER, 1, None))
        if device.nstats_instance is not None:
            t.append(nstatslib.create_nstats_tlv(device.nstats_instance))

        if device.button_instance is not None:
            t.append(tlvlib.createVectorTLV(tlvlib.TLV_SET_REQUEST, 0,
                                            tlvlib.VARIABLE_EVENT_ARRAY, 0, 0, 1,
                                            struct.pack("!L", 1)))
            t.append(tlvlib.createVectorTLV(tlvlib.TLV_SET_REQUEST, device.button_instance,
                                            tlvlib.VARIABLE_EVENT_ARRAY, 0, 0, 2,
                                            struct.pack("!LL", 1, 2)))

        try:
            print "Fetching data from", device.address
            enc,tlvs = tlvlib.sendTLV(t, device.address)
            device.last_seen = time.time();
            for tlv in tlvs:
                if tlv.Error != 0:
                    print "Received error:"
                    tlvlib.printTLV(tlv)
                elif tlv.Instance == 0:
                    if tlv.Variable == tlvlib.VARIABLE_UNIT_BOOT_TIMER:
                        # Update the boot time estimation
                        seconds,ns = tlvlib.convert_ieee64_time(tlv.IntValue)
                        device.boot_time = time.time() - seconds
                elif device.nstats_instance and tlv.Instance == device.nstats_instance:
                    if tlv.Variable == tlvlib.VARIABLE_NSTATS_DATA:
                        nstats = nstatslib.Nstats(tlv.Value)
                        self._handle_nstats(device, nstats)
            return True
        except socket.timeout:
            print "Failed to fetch from device (time out)"
            return False

    def setup(self):
        # discover and - if it is a NBR then find serial radio and configure the
        # beacons to something good!
        d = tlvlib.discovery(self.router_host)
        print "Product label:", d[0][0]

        if d[0][1] != tlvlib.BORDER_ROUTER:
            print "Error - could not find the radio - not starting the device server"
            return False

        i = 1
        for data in d[1]:
            print "Instance:",i , " type: %016x"%data[0], " ", data[1]
            # check for radio and if found - configure beacons
            if data[0] == tlvlib.RADIO:
                self.radio_instance = i
            elif data[0] == tlvlib.ROUTER:
                self.router_instance = i
                t = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, i,
                                     tlvlib.VARIABLE_NETWORK_ADDRESS, 2, None)
                enc, t = tlvlib.sendTLV(t, self.router_host)

                self.router_address = socket.inet_ntop(socket.AF_INET6, t[0].Value)
                print "\tRouter address:", self.router_address

                self.router_prefix = socket.inet_ntop(socket.AF_INET6, t[0].Value[0:8] + binascii.unhexlify("0000000000000000"))
                #while self.router_prefix.endswith("::"):
                #    self.router_prefix = self.router_prefix[0:len(self.router_prefix) - 1]
                print "\tNetwork prefix:",self.router_prefix
                if self.device_server_host:
                    self.udp_address = self.device_server_host
                else:
                    self.udp_address = self._lookup_device_host(self.router_prefix, socket.inet_ntop(socket.AF_INET6, t[0].Value[0:8] + binascii.unhexlify("0000000000000001")))
                print "\tNetwork address:",self.udp_address
            elif data[0] == tlvlib.BORDER_ROUTER_MANAGEMENT:
                self.brm_instance = i
            elif data[0] == tlvlib.NETWORK_STATISTICS:
                self.nstats_instance = i
            i = i + 1

        if not self.radio_instance:
            print "Error - could not find the radio instance - not starting the device server"
            return False

        # Setup socket to make sure it is possible to bind the address
        try:
            self._sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
            self._sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self._sock.bind((self.udp_address, self.udp_port))
            #self._sock.bind(('', self.udp_port))
        except Exception as e:
            print e
            print "Error - could not bind to the address", self.udp_address
            return False

        # set radio channel
        t1 = tlvlib.createTLV(tlvlib.TLV_SET_REQUEST, self.radio_instance,
                              tlvlib.VARIABLE_RADIO_CHANNEL, 0,
                              struct.pack("!L", self.radio_channel))
        # set radio PAN ID
        t2 = tlvlib.createTLV(tlvlib.TLV_SET_REQUEST, self.radio_instance,
                              tlvlib.VARIABLE_RADIO_PAN_ID, 0,
                              struct.pack("!L", self.radio_panid))
        tlvlib.sendTLV([t1,t2], self.router_host)

        # set-up beacon to ...
        IPv6Str = binascii.hexlify(socket.inet_pton(socket.AF_INET6, self.udp_address))
        BEACON = "fe02010a020090da01%08x"%self.location + "18020090da03" + IPv6Str + "%4x"%self.udp_port + "000000"
        beacon_payload = binascii.unhexlify(BEACON)
        print "Setting beacon with length",len(beacon_payload),"in instance", self.radio_instance
        print "\t",BEACON
        t = tlvlib.createVectorTLV(tlvlib.TLV_SET_REQUEST, self.radio_instance,
                                   tlvlib.VARIABLE_RADIO_BEACON_RESPONSE,
                                   0, 0, len(beacon_payload) / 4,
                                   beacon_payload)
        enc, t = tlvlib.sendTLV(t, self.router_host)
#        print "Result:"
#        tlvlib.printTLV(t[0])
        return True

    def serve_forever(self):
        # Initialize if not already initialized
        if not self.router_instance:
            if not self.setup():
                # Initialization failed
                return False

        print "Device server started at [" + self.udp_address + "]:" + str(self.udp_port)

        # do device management each 10 seconds
        t = threading.Thread(target=self._manage_devices)
        t.daemon = True
        t.start()
        while True:
            try:
                data, addr = self._sock.recvfrom(1024)
                host, port, _, _ = addr
                print "Received from ", addr, binascii.hexlify(data)
                enc = tlvlib.parseEncap(data)
                tlvs = tlvlib.parseTLVs(data[enc.size():])
                device = self.get_device(host)
                if device is not None:
                    device.last_seen = time.time();
                dev_type = 0
                button_counter = None
                for tlv in tlvs:
                    if tlv.Error != 0:
                        print "Received error:"
                        tlvlib.printTLV(tlv)
                    elif tlv.Instance == 0:
                        if tlv.Variable == 0:
                            dev_type = tlv.IntValue
                        elif tlv.Variable == tlvlib.VARIABLE_UNIT_BOOT_TIMER:
                            if device:
                                # Update the boot time estimation
                                seconds,nanoseconds = tlvlib.convert_ieee64_time(tlv.IntValue)
                                device.boot_time = time.time() - seconds

                        elif tlv.Variable == tlvlib.VARIABLE_TIME_SINCE_LAST_GOOD_UC_RX:
                            pass
                        elif tlv.Variable == tlvlib.VARIABLE_UNIT_CONTROLLER_WATCHDOG:
                            print "Found device",host,"of type 0x%016x"%dev_type,"that can be taken over - WDT = 0"
                            if self.grab_device(host):
                                if not device:
                                    device = self.add_device(host)
                                device.next_update = time.time() + self.watchdog_time - self.guard_time
                    elif device and device.button_instance and tlv.Instance == device.button_instance:
                        if tlv.Variable == tlvlib.VARIABLE_GPIO_TRIGGER_COUNTER:
                            button_counter = tlv.IntValue
                        elif tlv.Variable == tlvlib.VARIABLE_EVENT_ARRAY:
                            print "Button pressed at node",host,"-",button_counter,"times"
                            # Rearm the button
                            self.arm_device(host, device.button_instance)

                            de = DeviceEvent(device, EVENT_BUTTON, button_counter)
                            self.send_event(de)
                    elif device and device.nstats_instance and tlv.Instance == device.nstats_instance:
                        if tlv.Variable == tlvlib.VARIABLE_NSTATS_DATA:
                            nstats = nstatslib.Nstats(tlv.Value)
                            self._handle_nstats(device, nstats)

            except socket.timeout:
#                print "."
                pass

if __name__ == "__main__":
    # stuff only to run when not called via 'import' here
    manage_device = None
    arg = 1

    server = DeviceServer()

    while len(sys.argv) > arg + 1:
        if sys.argv[arg] == "-a":
            server.router_host = sys.argv[arg + 1]
        elif sys.argv[arg] == "-c":
            server.radio_channel = tlvlib.decodevalue(sys.argv[arg + 1])
        elif sys.argv[arg] == "-P":
            server.radio_panid = tlvlib.decodevalue(sys.argv[arg + 1])
        elif sys.argv[arg] == "-b":
            server.device_server_host = sys.argv[arg + 1]
        else:
            break
        arg += 2

    if len(sys.argv) > arg:
        if sys.argv[arg] == "-h":
            print "Usage:",sys.argv[0],"[-b bind-address] [-a host] [-c channel] [-P panid] [device]"
            exit(0)
        manage_device = sys.argv[arg]
        arg += 1

    if len(sys.argv) > arg:
        print "Too many arguments"
        exit(1)

    if manage_device:
        server.add_device(manage_device)

    try:
        if not server.setup():
            print "No border router found. Please make sure a border router is running!"
            sys.exit(1)
    except socket.timeout:
        print "No border router found. Please make sure a border router is running!"
        sys.exit(1)
    except Exception as e:
        print e
        print "Failed to connect to border router."
        sys.exit(1)

    server.serve_forever()
    print "*** Error - device server stopped"
