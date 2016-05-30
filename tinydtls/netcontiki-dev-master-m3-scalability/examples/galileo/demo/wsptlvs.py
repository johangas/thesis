import wsplugin, tlvlib, thread, deviceserver, json, struct, socket

# The plugin class
class TLVCommands(wsplugin.DemoPlugin):

    def get_commands(self):
        return ["tlvled", "tlvtemp"]

    def handle_command(self, wsdemo, cmds):
        if cmds[0] == "tlvled":
            ip = cmds[1]
            led = cmds[2]
            thread.start_new_thread(tlvled, (wsdemo, ip, led))
            return True
        elif cmds[0] == "tlvtemp":
            ip = cmds[1]
            thread.start_new_thread(tlvtemp, (wsdemo, ip))
            return True
        return False

# Toggle a LED on a Yanzi IoT-U10 node (or other node with LEDs)
def tlvled(ws, ip, led):
    de = ws.get_device_manager().get_device(ip)
    if de and de.leds_instance:
        t1 = tlvlib.createTLV(tlvlib.TLV_SET_REQUEST, de.leds_instance, tlvlib.VARIABLE_LED_TOGGLE, 0, struct.pack("!L", 1 << int(led)))
        try:
            enc, tlvs = tlvlib.sendTLV(t1, ip)
            if tlvs[0].Error == 0:
                print "LED ", led, " toggle."
            else:
                print "LED Set error", tlvs[0].Error
        except socket.timeout:
            print "LED No response from node", ip
            ws.sendMessage(json.dumps({"error":"No response from node " + ip}))

# Read the temperature from a Yanzi IoT-U10 node
def tlvtemp(ws, ip):
    de = ws.get_device_manager().get_device(ip)
    if de:
        instance = de.get_instance(tlvlib.TEMP_GENERIC)
        if instance:
            t1 = tlvlib.createTLV(tlvlib.TLV_GET_REQUEST, instance, tlvlib.VARIABLE_TEMPERATURE, 0, None)
            try:
                enc, tlvs = tlvlib.sendTLV(t1, ip)
                if tlvs[0].Error == 0:
                    temperature = tlvs[0].IntValue
                    if temperature > 100000:
                        temperature = round((temperature - 273150) / 1000.0, 2)
                    print "\tTemperature:",temperature,"(C)"
                    ws.sendMessage(json.dumps({"temp":temperature,"address":ip}))
                else:
                    print "\tTemperature error", tlvs[0].Error
            except socket.timeout:
                print "Temperature - no response from node", ip
                ws.sendMessage(json.dumps({"error":"No response from node " + ip}))
