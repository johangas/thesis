<?xml version="1.0" encoding="UTF-8"?>
<simconf>
  <project EXPORT="discard">[APPS_DIR]/mrm</project>
  <project EXPORT="discard">[APPS_DIR]/mspsim</project>
  <project EXPORT="discard">[APPS_DIR]/avrora</project>
  <project EXPORT="discard">[APPS_DIR]/serial_socket</project>
  <project EXPORT="discard">[APPS_DIR]/collect-view</project>
  <project EXPORT="discard">[APPS_DIR]/powertracker</project>
  <simulation>
    <title>6LoWPAN-ND</title>
    <randomseed>123456</randomseed>
    <motedelay_us>1000000</motedelay_us>
    <radiomedium>
      org.contikios.cooja.radiomediums.UDGM
      <transmitting_range>50.0</transmitting_range>
      <interference_range>100.0</interference_range>
      <success_ratio_tx>1.0</success_ratio_tx>
      <success_ratio_rx>1.0</success_ratio_rx>
    </radiomedium>
    <events>
      <logoutput>40000</logoutput>
    </events>
    <motetype>
      org.contikios.cooja.mspmote.SkyMoteType
      <identifier>sky1</identifier>
      <description>Host</description>
      <source EXPORT="discard">[CONTIKI_DIR]/examples/6lowpan-nd/6lh/ex-6lowpannd-6lh.c</source>
      <commands EXPORT="discard">make ex-6lowpannd-6lh.sky TARGET=sky</commands>
      <firmware EXPORT="copy">[CONTIKI_DIR]/examples/6lowpan-nd/6lh/ex-6lowpannd-6lh.sky</firmware>
      <moteinterface>org.contikios.cooja.interfaces.Position</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.RimeAddress</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.IPAddress</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.Mote2MoteRelations</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.MoteAttributes</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspClock</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspMoteID</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.SkyButton</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.SkyFlash</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.SkyCoffeeFilesystem</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.Msp802154Radio</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspSerial</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.SkyLED</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspDebugOutput</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.SkyTemperature</moteinterface>
    </motetype>
    <motetype>
      org.contikios.cooja.mspmote.SkyMoteType
      <identifier>sky2</identifier>
      <description>Router</description>
      <source EXPORT="discard">[CONTIKI_DIR]/examples/6lowpan-nd/6lr/ex-6lowpannd-6lr.c</source>
      <commands EXPORT="discard">make ex-6lowpannd-6lr.sky TARGET=sky</commands>
      <firmware EXPORT="copy">[CONTIKI_DIR]/examples/6lowpan-nd/6lr/ex-6lowpannd-6lr.sky</firmware>
      <moteinterface>org.contikios.cooja.interfaces.Position</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.RimeAddress</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.IPAddress</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.Mote2MoteRelations</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.MoteAttributes</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspClock</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspMoteID</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.SkyButton</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.SkyFlash</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.SkyCoffeeFilesystem</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.Msp802154Radio</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspSerial</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.SkyLED</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspDebugOutput</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.SkyTemperature</moteinterface>
    </motetype>
    <motetype>
      org.contikios.cooja.mspmote.SkyMoteType
      <identifier>sky3</identifier>
      <description>Border router</description>
      <source EXPORT="discard">[CONTIKI_DIR]/examples/6lowpan-nd/6lbr/ex-6lowpannd-6lbr.c</source>
      <commands EXPORT="discard">make ex-6lowpannd-6lbr.sky TARGET=sky</commands>
      <firmware EXPORT="copy">[CONTIKI_DIR]/examples/6lowpan-nd/6lbr/ex-6lowpannd-6lbr.sky</firmware>
      <moteinterface>org.contikios.cooja.interfaces.Position</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.RimeAddress</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.IPAddress</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.Mote2MoteRelations</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.MoteAttributes</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspClock</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspMoteID</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.SkyButton</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.SkyFlash</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.SkyCoffeeFilesystem</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.Msp802154Radio</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspSerial</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.SkyLED</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspDebugOutput</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.SkyTemperature</moteinterface>
    </motetype>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>22.539962642565648</x>
        <y>49.86995783751088</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>1</id>
      </interface_config>
      <motetype_identifier>sky3</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>24.57368106472331</x>
        <y>93.63732872459359</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>2</id>
      </interface_config>
      <motetype_identifier>sky2</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>28.16524036365975</x>
        <y>140.7051514183161</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>3</id>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspSerial
        <history>netd nc~;</history>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
  </simulation>
  <plugin>
    org.contikios.cooja.plugins.SimControl
    <width>280</width>
    <z>0</z>
    <height>160</height>
    <location_x>299</location_x>
    <location_y>10</location_y>
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.LogListener
    <plugin_config>
      <filter />
      <formatted_time />
      <coloring />
    </plugin_config>
    <width>919</width>
    <z>2</z>
    <height>911</height>
    <location_x>431</location_x>
    <location_y>35</location_y>
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.TimeLine
    <plugin_config>
      <mote>0</mote>
      <mote>1</mote>
      <mote>2</mote>
      <showRadioRXTX />
      <showRadioHW />
      <showLEDs />
      <zoomfactor>500.0</zoomfactor>
    </plugin_config>
    <width>1319</width>
    <z>7</z>
    <height>166</height>
    <location_x>0</location_x>
    <location_y>847</location_y>
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.Visualizer
    <plugin_config>
      <moterelations>true</moterelations>
      <skin>org.contikios.cooja.plugins.skins.IDVisualizerSkin</skin>
      <skin>org.contikios.cooja.plugins.skins.GridVisualizerSkin</skin>
      <skin>org.contikios.cooja.plugins.skins.TrafficVisualizerSkin</skin>
      <skin>org.contikios.cooja.plugins.skins.UDGMVisualizerSkin</skin>
      <viewport>3.47082132865459 0.0 0.0 3.47082132865459 -10.756516980873421 -157.36244059426517</viewport>
    </plugin_config>
    <width>399</width>
    <z>5</z>
    <height>400</height>
    <location_x>1</location_x>
    <location_y>1</location_y>
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.RadioLogger
    <plugin_config>
      <split>217</split>
      <formatted_time />
      <showdups>false</showdups>
      <hidenodests>false</hidenodests>
      <analyzers name="6lowpan" />
    </plugin_config>
    <width>865</width>
    <z>3</z>
    <height>438</height>
    <location_x>1</location_x>
    <location_y>402</location_y>
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.MoteInterfaceViewer
    <mote_arg>2</mote_arg>
    <plugin_config>
      <interface>Serial port</interface>
      <scrollpos>0,0</scrollpos>
    </plugin_config>
    <width>350</width>
    <z>6</z>
    <height>300</height>
    <location_x>975</location_x>
    <location_y>684</location_y>
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.ScriptRunner
    <plugin_config>
      <script>var prefix = "bbbb";&#xD;
var brID = 1;&#xD;
&#xD;
//Wait until all mote started&#xD;
for(var i=0; i&lt;3; i++) {&#xD;
    YIELD_THEN_WAIT_UNTIL(msg.indexOf("Contiki&gt;") != -1);&#xD;
}&#xD;
&#xD;
// Display all mote Table&#xD;
function displayAllTable(){&#xD;
    var allm = sim.getMotes();&#xD;
    for(var id in  allm) {&#xD;
	    write(allm[id], "netd nc");&#xD;
	    YIELD_THEN_WAIT_UNTIL(msg.indexOf("Contiki&gt;") != -1);&#xD;
	    write(allm[id], "netd rt");&#xD;
	    YIELD_THEN_WAIT_UNTIL(msg.indexOf("Contiki&gt;") != -1);&#xD;
	}&#xD;
}&#xD;
//displayAllTable();&#xD;
&#xD;
// Add to routing table&#xD;
function genip(num, pref){&#xD;
    var ip = "";&#xD;
    ip += pref;&#xD;
    for(var i=0; i&lt;3; i++){&#xD;
        ip +=":0000";&#xD;
    }&#xD;
    var n;&#xD;
    ip += ":0212";&#xD;
    n = 0x7400 + num;&#xD;
    ip += ":"+n.toString(16);&#xD;
    n = 0x0 + num;&#xD;
    ip += ":000"+n.toString(16);&#xD;
    n = (0x100 * num) + num;&#xD;
    ip += ":0"+n.toString(16);&#xD;
    return ip;&#xD;
}&#xD;
function gpip(num) { return genip(num, prefix); }&#xD;
function llip(num) { return genip(num, "fe80"); }&#xD;
function addroute(moteID, to, nexthop, len) {&#xD;
    mote = sim.getMoteWithID(moteID);&#xD;
    var cmd = "route -a "+gpip(to)+" "+llip(nexthop)+" "+len;&#xD;
    //log.log("MOTE " +moteID + "-&gt;" + cmd + "\n");&#xD;
    write(mote, cmd);&#xD;
}&#xD;
//RT mote 3&#xD;
//addroute(1, 0, 2, 32);&#xD;
//RT mote 2&#xD;
//addroute(2, 0, 1, 32);&#xD;
//addroute(2, 3, 3, 128);&#xD;
&#xD;
&#xD;
//Waiting configurate of all mote was done&#xD;
function waitingConfig(){&#xD;
	var lastid = 0;&#xD;
	for(var i=0;; i++) {&#xD;
	    YIELD_THEN_WAIT_UNTIL(msg.contains("Sending") || msg.contains("timeout"));&#xD;
	    if(msg.contains("Sending")){&#xD;
	        GENERATE_MSG(75000, "timeout"+lastid);&#xD;
	        lastid++;&#xD;
	    }else if(msg.equals("timeout"+(lastid-1))) {&#xD;
	        return;&#xD;
	    }&#xD;
	}&#xD;
}&#xD;
&#xD;
function sendudp(from, to) {&#xD;
    mote = sim.getMoteWithID(from);&#xD;
    var cmd = "sendudp " + gpip(to);&#xD;
    log.log("mote "+from+" -&gt; "+cmd+"\n");&#xD;
    write(mote, cmd);&#xD;
    YIELD_THEN_WAIT_UNTIL(msg.contains("DATA recv"));   &#xD;
}&#xD;
&#xD;
function sendudpBR(br) {&#xD;
    var allm = sim.getMotes();&#xD;
    for(var i in  allm) {&#xD;
        var id = allm[i].getID();&#xD;
        if(id != br) {&#xD;
            sendudp(id, br);&#xD;
            sendudp(br, id);&#xD;
        }&#xD;
    }&#xD;
}&#xD;
&#xD;
//Display NC when changement of msg was done&#xD;
while(true) {&#xD;
	waitingConfig();&#xD;
    displayAllTable();&#xD;
    //sendudpBR(brID);&#xD;
}</script>
      <active>true</active>
    </plugin_config>
    <width>511</width>
    <z>1</z>
    <height>995</height>
    <location_x>1390</location_x>
    <location_y>1</location_y>
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.MoteInterfaceViewer
    <mote_arg>1</mote_arg>
    <plugin_config>
      <interface>IP Address</interface>
      <scrollpos>0,0</scrollpos>
    </plugin_config>
    <width>350</width>
    <z>4</z>
    <height>300</height>
    <location_x>1011</location_x>
    <location_y>199</location_y>
  </plugin>
</simconf>

