import socket, binascii, wsplugin, tlvlib, thread, deviceserver, json, struct, urllib2, time

RANK_DIVISOR = 256.0

def fetch_routes(ip):
    print "fetching routes from:", ip
    data = urllib2.urlopen("http://[" + ip + "]/r").read()
    ips = data.split('\n')
    return ips

# The plugin class
class NodeCommands(wsplugin.DemoPlugin):

    routing_table = []
    routing_table_time = 0

    def get_commands(self):
        return ["routes", "topology"]

    def handle_command(self, wsdemo, cmds):
        if cmds[0] == "routes":
            self.listRoutes(wsdemo)
            return True
        elif cmds[0] == "topology":
            self.topology(wsdemo)
            return True
        return False

    def listRoutes(self, ws):
        print "list routes", time.time()
        if self.routing_table_time + 15 < time.time():
            print "refreshing routes..."
            self.routing_table = fetch_routes(ws.get_nbr_address())
            ws.get_device_manager().fetch_nstats();
            dev_table = []
            for dev in self.routing_table:
                if dev is None or dev == "":
                    #print "Found empty routing entry?"
                    continue
                de = ws.get_device_manager().get_device(dev)
                new_dev = {'address':dev}
                new_dev['last_seen'] = 'never'
                if de is None:
                    print "adding device:", dev
                    de = ws.get_device_manager().add_device(dev)
                if de is not None:
                    print "Type:", de.label
                    new_dev['type'] = de.label
                    if de.last_seen > 0:
                        new_dev['last_seen'] = int(time.time() - de.last_seen)
                    if de.nstats_rpl is not None:
                        rplinfo = {"parent":de.nstats_rpl.parent_as_string(),
                                   "parent_rank":de.nstats_rpl.parent_rank() / RANK_DIVISOR,
                                   "rank":de.nstats_rpl.dag_rank() / RANK_DIVISOR}
                        new_dev['rplinfo'] = rplinfo;
                else:
                    new_dev['type'] = 'unknown'
                dev_table = dev_table + [new_dev]
        ws.sendMessage(json.dumps({'routes':dev_table}))

    def topology(self, ws):
        # only the managed devices - since routes will not have anything
        # stored...
        devs = ws.get_device_manager().get_devices()
        topology = {"nodes":[], "edges":[]}
        # Add the NBR
        nodes = {}
        addr = socket.inet_pton(socket.AF_INET6, ws.get_nbr_address())
        endfix = binascii.hexlify(addr[-4:])
        topology["nodes"] = [{"id":1, "label":"NBR",
                           "title":"Rank 1.0 (Root)<br>" + ws.get_nbr_address(),
                              "level":0, "address":endfix}]
        node = 2
        nodes[endfix] = 1
        for device in devs:
            rpl = device.nstats_rpl
            parent = None
            if rpl is None:
                rank = "unknown"
            else:
                rank = rpl.dag_rank() / RANK_DIVISOR
                parent = rpl.parent_as_string()
                addr = socket.inet_pton(socket.AF_INET6, device.address)
                endfix = binascii.hexlify(addr[-4:])
                # First we add the parent as level - second round we will
                # change this to "level" instead
                topology["nodes"] = topology["nodes"] + [{"id":node, "label":"N" + str(node),       "title":"Rank " + str(rank) + "<br>" + device.address,
                                                          "level":-1, "parent":parent, "address":endfix}]
                nodes[endfix] = node
                node = node + 1

        changed = True
        i = 0
        edges = []
        while changed and i < 10:
            changed = False
            i = i + 1
            for n in topology["nodes"]:
                if n["level"] is -1:
                    p = topology["nodes"][nodes[n["parent"]] - 1]
                    # add an edge
                    edges = edges + [{"from":p["id"],"to":n["id"]}]
                    if p["level"] is not -1:
                        n["level"] = p["level"] + 1
                        #print "level should be ", p["level"] + 1
                        changed = True
        topology["edges"] = edges
        print "Sending json: ",json.dumps({'topology':topology})
        ws.sendMessage(json.dumps({'topology':topology}))

