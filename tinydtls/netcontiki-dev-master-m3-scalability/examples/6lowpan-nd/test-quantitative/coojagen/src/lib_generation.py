#Python COOJA simulation generator
import os
import sys
import shutil
import re
import generators
import imp
import time

class SimMoteType:
	def __init__(self, shortname, fw_folder, maketarget, makeargs, serial, description, platform="sky"):
		self.shortname = shortname
		self.fw_folder = os.path.normpath(fw_folder)
		self.maketarget = maketarget
		self.makeargs = makeargs
		self.serial = serial
		self.description = description
		self.platform = platform

	def text_from_template(self):
		text = """    <motetype>
      org.contikios.cooja.mspmote.MOTETYPE
      <identifier>SHORTNAME</identifier>
      <description>DESCRIPTION</description>
      <source EXPORT="discard">FIRMWAREPATH</source>
      <commands EXPORT="discard">make FIRMWARE.PLATFORM TARGET=PLATFORM MAKEARGS</commands>
      <firmware EXPORT="copy">FIRMWAREBIN</firmware>
      <moteinterface>org.contikios.cooja.interfaces.Position</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.RimeAddress</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.IPAddress</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.Mote2MoteRelations</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.MoteAttributes</moteinterface>"""
		if self.platform is 'wismote':
			text += """
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspClock</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspMoteID</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspButton</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.Msp802154Radio</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspDefaultSerial</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspLED</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspDebugOutput</moteinterface>"""
		else:
			text += """
      <moteinterface>se.sics.cooja.mspmote.interfaces.MspClock</moteinterface>
      <moteinterface>se.sics.cooja.mspmote.interfaces.MspMoteID</moteinterface>
      <moteinterface>se.sics.cooja.mspmote.interfaces.SkyButton</moteinterface>
      <moteinterface>se.sics.cooja.mspmote.interfaces.SkyFlash</moteinterface>
      <moteinterface>se.sics.cooja.mspmote.interfaces.SkyCoffeeFilesystem</moteinterface>
      <moteinterface>se.sics.cooja.mspmote.interfaces.SkyByteRadio</moteinterface>
      <moteinterface>se.sics.cooja.mspmote.interfaces.MspSerial</moteinterface>
      <moteinterface>se.sics.cooja.mspmote.interfaces.SkyLED</moteinterface>
      <moteinterface>se.sics.cooja.mspmote.interfaces.MspDebugOutput</moteinterface>
      <moteinterface>se.sics.cooja.mspmote.interfaces.SkyTemperature</moteinterface>"""
		text += """
    </motetype>\r\n"""
		text = text.replace('FIRMWAREPATH', self.fw_folder + os.path.sep + self.maketarget + '.c')
		text = text.replace('FIRMWAREBIN', self.fw_folder + os.path.sep + self.maketarget + '.'+self.platform)
		text = text.replace('PLATFORM', self.platform)
		text = text.replace('SHORTNAME', self.shortname)
		text = text.replace('DESCRIPTION', self.description)
		text = text.replace('FIRMWARE', self.maketarget)
		text = text.replace('MAKEARGS', self.makeargs)
		text = text.replace('MOTETYPE', self.platform[0].upper()+self.platform[1:len(self.platform)]+"MoteType")
		return text

class SimMote:
	def __init__(self, mote_type, nodeid):
		self.mote_type = mote_type
		self.nodeid = nodeid
		self.mobility_data = None
	def set_coords(self, xpos, ypos, zpos=0):
		self.xpos = xpos
		self.ypos = ypos
		self.zpos = zpos

	def set_mobility_data(self, data):
		self.mobility_data = data

	def text_from_template(self):


		text = """    <mote>
      <breakpoints />
      <interface_config>
	se.sics.cooja.mspmote.interfaces.MspMoteID
	<id>NODE_ID</id>
      </interface_config>
      <interface_config>
	se.sics.cooja.interfaces.Position
	<x>XPOS</x>
	<y>YPOS</y>
	<z>ZPOS</z>
      </interface_config>
      <motetype_identifier>MOTETYPE_ID</motetype_identifier>
    </mote>\r\n"""

		text = text.replace('XPOS','%03.13f' % self.xpos)
		text = text.replace('YPOS','%03.13f' % self.ypos)
		text = text.replace('ZPOS','%03.13f' % self.zpos)
		text = text.replace('NODE_ID', str(self.nodeid))
		text = text.replace('MOTETYPE_ID', self.mote_type.shortname)
		return text

	def serial_text(self):
		if self.mote_type.serial == 'pty':
			text = """  <plugin>
    de.fau.cooja.plugins.Serial2Pty
    <mote_arg>MOTEARG</mote_arg>
    <width>250</width>
    <z>0</z>
    <height>100</height>
    <location_x>161</location_x>
    <location_y>532</location_y>
  </plugin>\r\n"""
		elif self.mote_type.serial == 'socket':
			text = """  <plugin>
    SerialSocketServer
    <mote_arg>MOTEARG</mote_arg>
    <width>459</width>
    <z>4</z>
    <height>119</height>
    <location_x>5</location_x>
    <location_y>525</location_y>
  </plugin>\r\n"""
		else:
			return ''

		text = text.replace('MOTEARG','%d' % (self.nodeid-1))
		return text

class Sim():
	def __init__(self, templatepath):
		self.templatepath = templatepath
		self.simfile_lines = read_simfile(self.templatepath)

	def insert_sky_motetype(self, mote_type):

		motetype_text = mote_type.text_from_template()

		motetype_indexes = all_indices("    <motetype>\r\n",self.simfile_lines)
		motetype_close_indexes = all_indices("    </motetype>\r\n",self.simfile_lines)

		if len(motetype_indexes) == 1:
			#in case of 1 motetype, check if it's the template version or a real mote
			if self.simfile_lines[motetype_indexes[0]+2] == "      <identifier>templatesky1</identifier>\r\n":
				#template version, we first remove the template motetype lines
				count = motetype_close_indexes[0] - motetype_indexes[0] + 1
				remove_n_at(motetype_indexes[0], count, self.simfile_lines)
				#insert the mote type
				self.simfile_lines = insert_list_at(motetype_text.splitlines(1), self.simfile_lines, motetype_indexes[0]) #1= we keep the endlines
			else:
				self.simfile_lines = insert_list_at(motetype_text.splitlines(1), self.simfile_lines, motetype_close_indexes[0]+1, )

		else:
			#if there are more than one, we know they are not template motetypes. We append a new one
			self.simfile_lines = insert_list_at(motetype_text.splitlines(1), self.simfile_lines, motetype_close_indexes[-1]+1)

	def add_mote(self, mote):

		mote_text = mote.text_from_template()
		
		mote_indexes = all_indices("    <mote>\r\n",self.simfile_lines)
		mote_close_indexes = all_indices("    </mote>\r\n",self.simfile_lines)
	
		if len(mote_indexes) == 1:
			#only 1 mote, check if it's the template
			if self.simfile_lines[mote_indexes[0]+4] == "        <x>XPOS</x>\r\n":
				#template version, we first remove the template motetype lines
				count = mote_close_indexes[0] - mote_indexes[0] + 1
				remove_n_at(mote_indexes[0], count, self.simfile_lines)
				#insert the mote type
				self.simfile_lines = insert_list_at(mote_text.splitlines(1), self.simfile_lines, mote_indexes[0])
			else:
				self.simfile_lines = insert_list_at(mote_text.splitlines(1), self.simfile_lines, mote_close_indexes[0]+1, )

		else:
			#if there are more than one, we know they are not template motetypes. We append a new one
			self.simfile_lines = insert_list_at(mote_text.splitlines(1), self.simfile_lines, mote_close_indexes[-1]+1)
		
		
		if mote.mote_type.serial != '':	
			plugin_indexes = all_indices("  </plugin>\r\n", self.simfile_lines)
			serial_text = mote.serial_text()
			self.simfile_lines = insert_list_at(serial_text.splitlines(1), self.simfile_lines, plugin_indexes[-1]+1)

	def add_motes(self, mote_list):
		for mote in mote_list:
			self.add_mote(mote)

	def udgm_set_range(self, mote_range):
		radiomedium_index = self.simfile_lines.index('    <radiomedium>\r\n')
		if self.simfile_lines[radiomedium_index+1] == '      se.sics.cooja.radiomediums.UDGM\r\n':
			self.simfile_lines.pop(radiomedium_index+2)
			self.simfile_lines.insert(radiomedium_index+2,"      <transmitting_range>%.1f</transmitting_range>\r\n" % mote_range)
		
		else:
			print("ERROR: radio model is not UDGM\r\n")

	def udgm_set_interference_range(self, interference_range):
		radiomedium_index = self.simfile_lines.index('    <radiomedium>\r\n')
		if self.simfile_lines[radiomedium_index+1] == '      se.sics.cooja.radiomediums.UDGM\r\n':
			self.simfile_lines.pop(radiomedium_index+3)
			self.simfile_lines.insert(radiomedium_index+3,"      <interference_range>%.1f</interference_range>\r\n" % interference_range)
		
		else:
			print("ERROR: radio model is not UDGM\r\n")

	def udgm_set_rx_tx_ratios(self, rx, tx):
		radiomedium_index = self.simfile_lines.index('    <radiomedium>\r\n')
		if self.simfile_lines[radiomedium_index+1] == '      se.sics.cooja.radiomediums.UDGM\r\n':
			self.simfile_lines.pop(radiomedium_index+4)
			self.simfile_lines.pop(radiomedium_index+4)
			self.simfile_lines.insert(radiomedium_index+4,"      <success_ratio_tx>%.1f</success_ratio_tx>\r\n" % tx)
			self.simfile_lines.insert(radiomedium_index+5,"      <success_ratio_rx>%.1f</success_ratio_rx>\r\n" % rx)
		
		else:
			print("ERROR: radio model is not UDGM\r\n")

	def set_timeout(self, timeout):
		script_index = all_indices('      <script>\r\n' ,self.simfile_lines)[0]
		self.simfile_lines.pop(script_index+1)
		self.simfile_lines.insert(script_index+1, '        TIMEOUT(%d);\r\n' % timeout)

	def set_script(self, filename):
		script_index = all_indices('      <scriptfile>\r\n' ,self.simfile_lines)[0]
		self.simfile_lines.pop(script_index+1)
		self.simfile_lines.insert(script_index+1, '        %s\r\n' % filename)

	def set_dgrm_model(self, dgrm_file_path):
		dgrm_file = open(dgrm_file_path, 'r')
		radiomedium_open_index = self.simfile_lines.index('    <radiomedium>\r\n')
		radiomedium_close_index = self.simfile_lines.index('    </radiomedium>\r\n')
		remove_n_at(radiomedium_open_index+1, radiomedium_close_index - radiomedium_open_index -1, self.simfile_lines)
		self.simfile_lines.insert(radiomedium_open_index+1, '      se.sics.cooja.radiomediums.DirectedGraphMedium\r\n')

		ptr = radiomedium_open_index+2

		#format:
		#71 20 0.00 0 0 0 -10.0 0 0

		total_errors = 0
		re_dgrm_line = re.compile('([0-9]+) ([0-9]+) ([0-9,.]+) ([0-9]+) ([0-9]+) ([0-9]+) ([0-9,.,-]+) ([0-9]+) ([0-9]+)\r\n')
		for line in dgrm_file:
			match_dgrm_line = re_dgrm_line.match(line)
			if not match_dgrm_line:
				total_errors += 1
			else:
				src = match_dgrm_line.group(1)
				dst = match_dgrm_line.group(2)
				prr = match_dgrm_line.group(3)
				prr_ci = match_dgrm_line.group(4)
				num_tx = match_dgrm_line.group(5)
				num_rx = match_dgrm_line.group(6)
				rssi = match_dgrm_line.group(7)
				rssi_min = match_dgrm_line.group(8)
				rssi_max = match_dgrm_line.group(9)

				delay = 0;

				radio_edge = dgrm_generate(src,dst,prr,rssi,delay)

				self.simfile_lines = insert_list_at(radio_edge.splitlines(1), self.simfile_lines, ptr)
				#we don't increment ptr, just let the next insertions shift the existing edges down

		print ("total_errors = %d\r\n",total_errors)

	def save_simfile(self, simfilepath):
		simfile = open(simfilepath,'w')
		for line in self.simfile_lines:
			simfile.write(line)
		simfile.close()

	def add_mobility(self, mobility_path):
		text="""  <plugin>
    Mobility
    <plugin_config>
      <positions EXPORT="copy">POSITIONSFILE</positions>
    </plugin_config>
    <width>500</width>
    <z>0</z>
    <height>200</height>
    <location_x>210</location_x>
    <location_y>210</location_y>
  </plugin>\r\n"""
		text = text.replace('POSITIONSFILE', mobility_path)
		plugin_indexes = all_indices("  </plugin>\r\n", self.simfile_lines)
		self.simfile_lines = insert_list_at(text.splitlines(1), self.simfile_lines, plugin_indexes[-1]+1)

		simulation_indexes = all_indices("  <simulation>\r\n", self.simfile_lines)
		text = '  <project EXPORT="discard">[APPS_DIR]/mobility</project>\r\n'
		self.simfile_lines = insert_list_at(text, self.simfile_lines, simulation_indexes[-1])

def new_sim(templatepath = 'cooja-template.csc'):
	return Sim(templatepath)

def create_twistreplay_from_template(simfilepath):
	templatepath = 'twistreplay-template.csc'
	shutil.copyfile(templatepath, simfilepath)

def read_simfile(simfilepath):
	simfile = open(simfilepath,'r')
	simfile_lines = simfile.readlines()
	simfile.close()
	return simfile_lines

def all_indices(value, qlist):
    indices = []
    idx = -1
    while 1:
        try:
            idx = qlist.index(value, idx+1)
            indices.append(idx)
        except ValueError:
            break
    return indices

def remove_n_at(index,count,qlist):
	while count > 0:
		qlist.pop(index)
		count = count - 1
	return qlist

def insert_list_at(src,dst,index):
	cpt = 0
	for elem in src:
		dst.insert(index+cpt,elem)
		cpt = cpt+1
	return dst

def dgrm_generate(src,dst,prr,rssi,delay):
	template = """      <edge>
	<source>SRC</source>
	<dest>
	  se.sics.cooja.radiomediums.DGRMDestinationRadio
	  <radio>DST</radio>
	  <ratio>PRR</ratio>
	  <signal>RSSI</signal>
	  <delay>DELAY</delay>
	</dest>
      </edge>\r\n"""

	template = template.replace('SRC',src)
	template = template.replace('DST',dst)
	template = template.replace('PRR',prr)
	template = template.replace('RSSI',rssi)
	template = template.replace('DELAY',str(delay))
	return template

def extract_node_id_list(nodeidpath):
	nodeids = []
	nodeidfile = open(nodeidpath, 'r')
	for line in nodeidfile:
		nodeids.append(line.rstrip())
	return nodeids

def mkdir(adir):
	try:
		os.makedirs(adir)
	except OSError:
		if os.path.exists(adir):
			# We are safe
			pass
		else:
			# There was an error on creation, so make sure we know about it
			raise

def cleardir(adir):
	for afile in os.listdir(adir):
		file_path = os.path.join(adir, afile)
		try:
			os.unlink(file_path)
		except Exception, e:
			print e

def export_mote_list(exportpath, motelist):
	exportfile = open(exportpath, 'w')
	for mote in motelist:
		exportfile.write("%d;%s" %(mote.nodeid, mote.mote_type.shortname))
		if mote.mobility_data != None:
			for xy in mote.mobility_data:
				exportfile.write(";%2.2f,%2.2f" % (xy[0], xy[1]))
		exportfile.write("\r\n")
	exportfile.close()


class ConfigParser():

	def __init__(self):
		self.mote_types = []
		self.motelist = []
		self.simfiles = []

	def assign_mote_types(self, assignment, mote_count):
		motenames = [assignment['all'] for i in range(mote_count)]
		for key, value in assignment.iteritems():
			if key != 'all':
				motenames[int(key)] = value
		return motenames

	def mote_type_from_shortname(self, shortname):
		for mote_type in self.mote_types:
			if mote_type.shortname == shortname:
				return mote_type
		return None

        def add_mobility_data(self, data):
		for index in data:
			self.motelist[int(index)].set_mobility_data(data[index])

	def parse_config_file(self, config_path):
		print("LOADING CONFIG %s" % config_path)
		config_simgen = imp.load_source('module.name', config_path)
                return self.parse_config(config_simgen)
	def parse_config(self, config_simgen):
		if hasattr(config_simgen, 'outputfolder'):
			outputfolder = config_simgen.outputfolder
		else:
			outputfolder = '..' + os.path.sep + 'output'

		if hasattr(config_simgen, 'template'):
			template_path = config_simgen.template
		else:
			template_path = '..' + os.path.sep + 'templates' . os.path.sep + 'cooja-template-udgm.csc'

		mkdir(outputfolder)
		cleardir(outputfolder)

		now = time.strftime("%Y%m%d%H%M%S")

		previous_count = 0
		if config_simgen.topology != 'preset':
			for mote_count in config_simgen.mote_count:
				sim = Sim(template_path)

				for mote_type in config_simgen.mote_types:
					platform = 'sky'
					if 'platform' in mote_type:
						platform = mote_type['platform']
					mote_type_obj = SimMoteType(    mote_type['shortname'],
									mote_type['fw_folder'],
									mote_type['maketarget'],
									mote_type['makeargs'],
									mote_type['serial'],
									mote_type['description'],
									platform)
					self.mote_types.append(mote_type_obj)
					sim.insert_sky_motetype(mote_type_obj)

				sim.udgm_set_range(config_simgen.tx_range)
				sim.udgm_set_interference_range(config_simgen.tx_interference)

				coords = generators.gen(config_simgen, mote_count)
				if(previous_count == len(coords)):
					continue

				previous_count = len(coords)

				motenames = self.assign_mote_types(config_simgen.assignment, len(coords))
				simfilepath = os.path.normpath(outputfolder) + os.path.sep + 'coojasim-' + config_simgen.topology + '-' + str(len(coords)) + '-' + now + '.csc'

				for index,coord in enumerate(coords):
					nodeid = index + 1
					mote = SimMote(self.mote_type_from_shortname(motenames[index]), nodeid)
					mote.set_coords(coord[0], coord[1], coord[2])
					self.motelist.append(mote)

				sim.add_motes(self.motelist)
				if config_simgen.script_file:
					sim.set_script(config_simgen.script_file)
				else:
					sim.set_timeout(999999999) #stop time in ms

				if hasattr(config_simgen, 'mobility'):
					sim.add_mobility(config_simgen.mobility)

				if hasattr(config_simgen, 'interactive_mobility'):
					self.add_mobility_data(config_simgen.interactive_mobility)

				sim.save_simfile(simfilepath)
				self.simfiles.append(simfilepath)
				print("****\n%s" % simfilepath)
				export_mote_list(simfilepath[:-4]+'.motes', self.motelist)

				self.motelist=[]
		else: #preset
			simlist = generators.load_preset(config_simgen.preset_data_path)
			for coords in simlist:
				sim = Sim(template_path)

				for mote_type in config_simgen.mote_types:
					platform = 'sky'
					if 'platform' in mote_type:
						platform = mote_type['platform']
					mote_type_obj = SimMoteType(    mote_type['shortname'],
									mote_type['fw_folder'],
									mote_type['maketarget'],
									mote_type['makeargs'],
									mote_type['serial'],
									mote_type['description'],
									platform)
					self.mote_types.append(mote_type_obj)
					sim.insert_sky_motetype(mote_type_obj)

				sim.udgm_set_range(config_simgen.tx_range)
				sim.udgm_set_interference_range(config_simgen.tx_interference)

				motenames = self.assign_mote_types(config_simgen.assignment, len(coords))
				"""
				if hasattr(config_simgen, 'label'):
					simfilepath = os.path.normpath(outputfolder) + os.path.sep + 'coojasim-' + config_simgen.topology + '_' + config_simgen.label + '-' + str(len(coords)) + '-' + now + '.csc'
				else:
					simfilepath = os.path.normpath(outputfolder) + os.path.sep + 'coojasim-' + config_simgen.topology + '-' + str(len(coords)) + '-' + now + '.csc'
				"""
				simfilepath = os.path.normpath(outputfolder) + os.path.sep + 'coojasim-' + os.path.splitext(os.path.basename(config_simgen.preset_data_path))[0] + '-' + str(len(coords)) + '-' + now + '.csc'

				for index,coord in enumerate(coords):
					nodeid = index + 1
					mote = SimMote(self.mote_type_from_shortname(motenames[index]), nodeid)
					mote.set_coords(coord['x'], coord['y'], coord['z'])
					self.motelist.append(mote)

				sim.add_motes(self.motelist)
				if config_simgen.script_file:
					sim.set_script(config_simgen.script_file)
				else:
					sim.set_timeout(999999999) #stop time in ms

				if hasattr(config_simgen, 'mobility'):
					sim.add_mobility(config_simgen.mobility)

				if hasattr(config_simgen, 'interactive_mobility'):
					self.add_mobility_data(config_simgen.interactive_mobility)

				sim.save_simfile(simfilepath)
				self.simfiles.append(simfilepath)
				print("****\n%s" % simfilepath)
				export_mote_list(simfilepath[:-4]+'.motes', self.motelist)

				self.motelist=[]

		print("Done. Generated %d simfiles" % len(self.simfiles))
		return True

	def get_simfiles(self):
		return self.simfiles
