import wsplugin, thread, subprocess, json, re, os.path

def coap_is_supported():
    return os.path.isfile("/usr/local/bin/coap-client")

def coap_get(ws, uri):
    ws.stop = False
    p=subprocess.Popen(["/usr/local/bin/coap-client", "-m", "get", uri],
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    data = ""
    try:
        while not ws.stop:
            line = p.stdout.readline()
            if line == '': break
            data = data + line
    except Exception as e:
        print e
        print "CoAP Unexpected error:", sys.exc_info()[0]
    p.terminate()
    return data

# This assumes that data is ascii!
def coap_put(ws, uri, data):
    ws.stop = False
    p=subprocess.Popen(["/usr/local/bin/coap-client", "-e" + data,
                        "-m", "put", uri],
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    line = ""
    try:
        while not ws.stop:
            line = p.stdout.readline()
            if line == '': break
            print line
    except Exception as e:
        print e
        print "CoAP Unexpected error:", sys.exc_info()[0]
    p.terminate()
    return line

def coap_check(ws):
    if not coap_is_supported():
        print "Error: can not find libcoap at /usr/local/bin/coap-client"
        ws.sendMessage(json.dumps({"error":"Can not find libcoap. Please install libcoap in the server and try again."}))
        return False
    return True

# The plugin class
class CoAPCommands(wsplugin.DemoPlugin):

    def get_commands(self):
        return ["coapled", "coaptemp"]

    def handle_command(self, wsdemo, cmds):
        if cmds[0] == "coapled":
            if coap_check(wsdemo):
                ip = cmds[1]
                led = cmds[2]
                on = cmds[3]
                thread.start_new_thread(coapled, (wsdemo, ip, led, on))
            return True
        elif cmds[0] == "coaptemp":
            if coap_check(wsdemo):
                ip = cmds[1]
                thread.start_new_thread(coaptemp, (wsdemo, ip))
            return True
        return False

# Toggle a LED on a Yanzi IoT-U10 node (or other node with LEDs)
def coapled(ws, ip, led, on):
    coap_put(ws, "coap://[" + ip + "]/led/" + led, on)

# Read the temperature from a Yanzi IoT-U10 node
def coaptemp(ws, ip):
    temperature = coap_get(ws, "coap://[" + ip + "]/temperature")
    print "\t",temperature
    # get the temperature from the coap response
    m = re.search("Temperature .+: (.+)", temperature)
    if m:
        ws.sendMessage(json.dumps({"temp":m.group(1),"address":ip}))
    else:
        ws.sendMessage(json.dumps({"error":"Failed to fetch temperature via CoAP"}))
