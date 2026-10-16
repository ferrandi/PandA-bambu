from functools import reduce
import glob, os, re, sys
import xml.etree.ElementTree as ET
from os import path

nx_suite_version=0
try:
	from nxpython import *
	nx_suite_version=4
except:
	try:
		from nxmap import *
		nx_suite_version=3
	except:
		try:
			from nanoxmap import *
			nx_suite_version=2
		except:
			print('Unable to find valid NXmap python module')
			pass
print('NXmap version: {}'.format(nx_suite_version))

connect_iob = False

table_corner = '+'
table_edge = '|'
table_row = '+--'

class PowerEstimation:
	def __init__(self):
		self.enable_rate = {'clock': 0.5,'FABRIC':0.5,'RAM':0.25,'DSP':0.25}

	def compute_power(self, temp, freq, luts, regs, brams, dsps):
		pass

class NG_ULTRA(PowerEstimation):
	def __init__(self):
		PowerEstimation.__init__(self)
		self.__ram_dyn_power_a = 0.02
		self.__ram_dyn_power_b = 0.09
		self.__dsp_dyn_power_a = 0.05
		self.__dsp_dyn_power_b = 0.47
		self.__voltage_vdd = 1.0

	def compute_power(self, temp, freq, luts, regs, brams, dsps):
		pwr_clk = 0.000000000008*freq*1000000*1000 + 0.0625*0.000000000001*freq*self.enable_rate['clock']*1000000*regs
		pwr_logic = ((0.000000000000446*(freq*regs*self.enable_rate['FABRIC'])*1000000 + 0.000000000000403*(freq*luts*self.enable_rate['FABRIC'])*1000000)*1000*self.__voltage_vdd)*1.05
		pwr_bram = brams*(self.__ram_dyn_power_a*freq*self.enable_rate['RAM']*+self.__ram_dyn_power_b)
		pwr_dsps = dsps*(self.__dsp_dyn_power_a*freq*self.enable_rate['DSP']*+self.__dsp_dyn_power_b)
		return pwr_clk + pwr_logic + pwr_bram + pwr_dsps

class NG_LARGE(PowerEstimation):
	def __init__(self):
		PowerEstimation.__init__(self)
		self.__voltage_vdd = 1.2
		self.__bram_write_rate = 0.5

	def compute_power(self, temp, freq, luts, regs, brams, dsps):
		if(temp != 25):
			raise Exception('Current version of the estimator only supports ambient temperature')
		pwr_clk = (0.00000008*(freq*self.enable_rate['clock']*regs) + 0.0107)*1000*(self.__voltage_vdd/1.2)**2
		pwr_logic = ((0.0000004*(self.enable_rate['FABRIC']*freq*regs) + 0.0173)*1000+ (0.0000004*(self.enable_rate['FABRIC']*freq*luts) + 0.0173)*1000)*(self.__voltage_vdd/1.2)**2
		pwr_ram = (((freq*brams*self.enable_rate['RAM'])*0.000018612*self.__bram_write_rate*1000)+((freq*brams*self.enable_rate['RAM'])*0.000017423*(1-self.__bram_write_rate)*1000))*(self.__voltage_vdd/1.2)**2
		pwr_dsps = (-0.000000001*(freq*dsps*self.enable_rate['DSP'])**2 + 0.0001*(freq*dsps*self.enable_rate['DSP']))*1000*(self.__voltage_vdd/1.2)**2
		return pwr_clk + pwr_logic + pwr_ram + pwr_dsps

class NG_MEDIUM(PowerEstimation):
	def __init__(self):
		PowerEstimation.__init__(self)
		self.__voltage_vdd = 1.2
		self.__bram_write_rate = 0.5
	def compute_power(self, temp, freq, luts, regs, brams, dsps):
		pwr_clk = (0.00000008*(freq*self.enable_rate['clock']*regs) + 0.0107)*1000*(self.__voltage_vdd/1.2)**2
		pwr_logic = ((0.0000004*(self.enable_rate['FABRIC']*freq*regs) + 0.0173)*1000+ (0.0000004*(self.enable_rate['FABRIC']*freq*luts) + 0.0173)*1000)*(self.__voltage_vdd/1.2)**2
		pwr_ram = (((freq*brams*self.enable_rate['RAM'])*0.000018612*self.__bram_write_rate*1000)+((freq*brams*self.enable_rate['RAM'])*0.000017423*(1-self.__bram_write_rate)*1000))*(self.__voltage_vdd/1.2)**2
		pwr_dsps = (-0.000000001*(freq*dsps*self.enable_rate['DSP'])**2 + 0.0001*(freq*dsps*self.enable_rate['DSP']))*1000*(self.__voltage_vdd/1.2)**2
		return pwr_clk + pwr_logic + pwr_ram + pwr_dsps

def power_estimation(device: str, temp, freq, luts, regs, brams, dsps):
	class_ = globals()[re.sub('-EMBEDDED$', '', device).replace('-','_')]
	power = class_().compute_power(temp, freq, luts, regs, brams, dsps)
	return str(round(power / 1000.0, 3))

def first_table_after(table_header, lines):
	lines = lines[lines.index(next(iter(filter(lambda x: table_header in x, lines)), None)):]
	lines = lines[lines.index(next(iter(filter(lambda x: table_row in x, lines)), None)):]
	ltrim = lines[0].find(table_corner)
	try:
		lines = lines[:lines.index(next(iter(filter(lambda x: all(t not in x[ltrim:] for t in [table_row, table_edge]), lines)), None))]
	except Exception:
		None
	return list(map(lambda x: x[ltrim:], filter(lambda x: any(t in x[ltrim:] for t in [table_row, table_edge]), lines)))

def parse_ascii_table(table_lines):
	rows = []
	for line in table_lines:
		if table_row in line: continue
		splitted_line = list(map(str.strip, filter(lambda x: x!=table_edge, line.split(table_edge))))[1:-1]
		rows.append(['']*len(splitted_line))
		for i in range(len(splitted_line)):
			rows[-1][i] = splitted_line[i]
	return rows

def get_res(log_lines):
	global connect_iob

	num_re = re.compile(r"(\d+)")
	first_num = lambda str: num_re.search(str).group(1)
	if connect_iob:
		table_lines = first_table_after('Ports used in current design.', log_lines)
	else:
		table_lines = first_table_after('List of Pins:', log_lines)
	rows = parse_ascii_table(table_lines)
	pin_table = dict(zip(rows[1], rows[2]))
	if nx_suite_version < 4:
		lines = log_lines[log_lines.index(next(iter(filter(lambda x: 'Reporting routed project' in x, log_lines)), None)):]
		table_lines = first_table_after('Reporting instances', lines)
	else:
		table_lines = first_table_after('Reporting instances at state \'Routed\'', log_lines)
		lines = log_lines[log_lines.index(next(iter(filter(lambda x: 'Reporting instances at state \'Routed\'' in x, log_lines)), None)):]
	rows = parse_ascii_table(table_lines)
	res_table = dict(zip([' '.join([a,b,c]).strip() for a,b,c in zip(rows[0],rows[1],rows[2])], rows[3]))
	fe_num = '0'
	if nx_suite_version == 2:
		table_lines = first_table_after('The following table details the use of FEs.', lines)
		rows = parse_ascii_table(table_lines)
		fe_table = dict(zip(rows[2], rows[3]))
		fe_num = first_num(fe_table['Count'])
	else:
		fe_num = first_num(next(iter(filter(lambda x: 'FE occupancy is' in x, lines))))
	table_lines = first_table_after('The following table lists the number of registers for each instance type', lines)
	rows = parse_ascii_table(table_lines)
	reg_table = dict(zip(rows[2], rows[3]))
	return [fe_num, first_num(res_table['4-LUT']), reg_table['Count'], first_num(res_table['Memory block']), str(reduce(lambda a, b: a + b, map(int, pin_table.values()))), first_num(res_table['Digital signal processor'])]

def get_timing(log_lines):
	table_lines = first_table_after('Reporting timing constraints', log_lines)
	rows = parse_ascii_table(table_lines)
	timing_table = dict(zip(rows[1], rows[2]))
	unit = timing_table['Setup/Recovery'][-2:]
	value = float(timing_table['Setup/Recovery'][:-2])
	if unit == 'ns': return str(value)
	elif unit == 'ps': return str(value / 1000.0)
	return timing_table['Setup/Recovery'][:-2]

def main():
	global connect_iob

	outdir = os.path.dirname(os.path.realpath(__file__))
	pads_filename = os.path.join(os.getcwd(), 'pads.py')
	prjEXT='.nym' if nx_suite_version >= 3 else '.nxm'

	config = ET.parse(os.getenv('BAMBU_HLS_RESULTS'))
	connect_iob = config.find('./target').get('connect_iob') == 'True'
	target_family = config.find('./target').get('family')
	target_family = f'{target_family}-EMBEDDED' if not connect_iob else target_family

	sys.path.append(os.getcwd())
	
	num_threads = config.find('./backend').get('parallel')
	if num_threads:
		setCoreCount(int(num_threads))
	
	project = createProject()

	if nx_suite_version >= 3:
		setLogDirectory(outdir)

	project.setVariantName(target_family)
	project.setTopCellName(config.find('./top_module').get('name'))

	srcs = []
	if config.getroot().get('./vhdl_library'):
		for src in config.getroot().get('./vhdl_library').split(':'):
			srcs.append(os.path.join(os.getcwd(), src))
	for src in config.findall('./outputs/file'):
		srcs.append(os.path.join(os.getcwd(), src.text))
	project.addFiles(srcs)
	project.setOptions({
		'ManageAsynchronousReadPort': 'Yes',
		'ManageUnconnectedOutputs': 'Ground',
		'ManageUnconnectedSignals': 'Ground',
		'DefaultRAMMapping': 'RAM',
		'MaxRegisterCount': '12500',
	})
	project.setOptions({'MappingEffort': 'High', 'TimingDriven': 'Yes'})

	if nx_suite_version >= 3:
		project.setOptions({
			'MappingEffort': 'High',
			'TimingDriven': 'Yes',
			'DensityEffort': 'Medium',
			'BypassingEffort': 'High',
			'PartitioningEffort': 'High',
			'PolishingEffort': 'High',
			'RoutingEffort': 'High',
		})
	if nx_suite_version <= 3:
		project.setTimingUnit('ns')

	clock_period_ns = float(config.find('./target').get('period'))
	clock_name = config.find('./top_module').get('clock_name')
	project.createClock(target= f'getClockNet({clock_name})', name=clock_name, period=clock_period_ns)

	# OOC/EMBEDDED fix: bind the clock to a system-interface pin (TSI) so it is
	# not promoted to an unconfigured physical Global Clock Input (GCI), which
	# makes Routing step 3/3 fail the 'AnyInstance HasConfig' check.
	if not connect_iob:
		project.addPins({clock_name: 'TSI1'})

	if connect_iob and path.exists(pads_filename):
		from pads import pads
		project.addPads(pads)

	project.save(os.path.join(outdir, 'native'+prjEXT))

	if not project.synthesize():
		sys.exit(1)
	project.save(os.path.join(outdir, 'synthesized'+prjEXT))

	if not project.place():
		sys.exit(1)
	project.save(os.path.join(outdir, 'placed'+prjEXT))
	if nx_suite_version == 2 and not path.exists(pads_filename):
		project.savePorts(pads_filename)

	if not project.route():
		sys.exit(1)
	project.save(os.path.join(outdir, 'routed'+prjEXT))
	
	analyzer = project.createAnalyzer()
	analyzer.launch()
	analyzer.destroy()

	project.generateBitstream(os.path.join(outdir, 'bitfile.nxb'))
	#print 'Errors: ', getErrorCount()
	#print 'Warnings: ', getWarningCount()

	if nx_suite_version == 2:
		fr = open(os.path.join(outdir, 'logs/Timing_Constraints_Report.timing'), "r", encoding="utf-8")
	else:
		fr = open(glob.glob(outdir + '/Timing_Constraints_Report_Routed_*.timing')[0], "r", encoding="utf-8")
	lines = fr.readlines()
	fr.close()

	design_slack_ns = get_timing(lines)
	if nx_suite_version == 2:
		fr = open(os.path.join(outdir, 'logs/general.log'), "r", encoding="utf-8")
	elif nx_suite_version == 3:
		fr = open(os.path.join(outdir, 'progress.rpt'), "r", encoding="utf-8")
	else:
		fr = open(os.path.join(outdir, 'general.log'), "r", encoding="utf-8")
	lines = fr.readlines()
	fr.close()

	FunctionalElements, LUTs, TotalRegisters, TotalMemBlocks, TotalIOPins, TotalDSPs = get_res(lines)
	delay = clock_period_ns - float(design_slack_ns)
	freq_mhz = 1000.0 / delay
	power_W = power_estimation(target_family, 25, freq_mhz, int(LUTs), int(TotalRegisters), int(TotalMemBlocks), int(TotalDSPs))

	fw = open(os.path.join(outdir, config.find('./backend').get('bambu_results')), "w")
	fw.write('<?xml version=\"1.0\"?>\n')
	fw.write('<application>\n')
	fw.write('  <resources\n')
	fw.write(f'    AREA="{FunctionalElements}"\n')
	fw.write(f'    FE="{FunctionalElements}"\n')
	fw.write(f'    LUTS="{LUTs}"\n')
	fw.write(f'    REGISTERS="{TotalRegisters}"\n')
	fw.write(f'    BRAMS="{TotalMemBlocks}"\n')
	fw.write('    DRAMS="0"\n')
	fw.write(f'    IOPINS="{TotalIOPins}"\n')
	fw.write(f'    DSPS="{TotalDSPs}"\n')
	fw.write(f'    POWER="{power_W}"\n')
	fw.write(f'    CLOCK_SLACK="{design_slack_ns}"\n')
	fw.write(f'    FREQUENCY="{freq_mhz}"\n')
	fw.write(f'    SLACK="{design_slack_ns}"\n')
	fw.write(f'    PERIOD="{delay}"\n')
	fw.write(f'    DELAY="{delay}" />\n')
	fw.write('</application>\n')
	fw.close()

if __name__ == '__main__':
	main()
