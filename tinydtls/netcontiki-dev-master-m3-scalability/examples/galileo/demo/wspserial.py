#
#
#
import wsplugin, serialradio, sys, thread, json

# The plugin class
class SerialCommands(wsplugin.DemoPlugin):
    
    def get_commands(self):
        return ["sniff", "rssi"]

    def handle_command(self, wsdemo, cmds):
        if cmds[0] == "rssi":
            thread.start_new_thread(rssi_scan, (wsdemo,))
            return True
        elif cmds[0] == "sniff":
            thread.start_new_thread(sniff, (wsdemo,))
            return True
        return False

def hexdump(src, length=16):
    FILTER = ''.join([(len(repr(chr(x))) == 3) and chr(x) or '.' for x in range(256)])
    lines = []
    for c in xrange(0, len(src), length):
        chars = src[c:c+length]
        hex = ' '.join(["%02x" % ord(x) for x in chars])
        printable = ''.join(["%s" % ((ord(x) <= 127 and FILTER[ord(x)]) or '.') for x in chars])
        lines.append("%04x  %-*s  %s\n" % (c, length*3, hex, printable))
    return ''.join(lines)


con = None

def serial_connect(ws):
    global con
    if con != None:
        print "Already Connected to TCP socket"
        return con
    print "Connecting to TCP socket of NBR"
    con = serialradio.SerialRadioConnection()
    con.connect()
#    con.set_debug(True)
    return con

def serial_disconnect(ws):
    global con
    if con != None:
        print "disconnection from serial radio"
    con.send("!m\001")
    con.close()
    con = None

def sniff(ws):
    ws.stop = False
    con = serial_connect(ws)
    print "Configuring radio for sniffing. ", con
    con.send("!m\002")
    while not ws.stop:
        try:
            data = con.get_next_block(1)
            sniff_data = []
            if data != None:
                # skip the first 3 chars and dump the rest to javascript...
                for d in data.serialData[2:]:
                    sniff_data = sniff_data + [ord(d)]
                ws.sendMessage(json.dumps({'sniff':sniff_data, 'hex':hexdump(data.serialData[2:])}))
        except Exception as e:
            print "Sniff - Unexpected error:", sys.exc_info()[0], "E:", e
            break
    serial_disconnect(ws)
    
def rssi_scan(ws):
    ws.stop = False
    con = serial_connect(ws)
    print "Configuring radio for RSSI scan.", con
    con.send("!c\001")
    con.send("!m\003")
    while not ws.stop:
        try:
            data = con.get_next_block(1)
            #print "Received ", i, " => ", data
            rssi = []
            if data != None:
                # skip the first 3 chars and dump the rest to javascript...
                for d in data.serialData[3:]:
                    v = ord(d)
                    if v >= 128: # negative...
                        v = v - 256
                    rssi = rssi + [v]
                ws.sendMessage(json.dumps({'rssi':rssi}))
        except Exception as e:
            print "RSSI - Unexpected error:", sys.exc_info()[0], e
            break
    serial_disconnect(ws)
