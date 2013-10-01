import re

MODE_NONE = 0
MODE_MSGID = 1
MODE_STRINGTABLE = 2

class IliParser(object):
	def __init__(self, filename):
		super(IliParser, self).__init__()

		self.ili_fname = filename
		self.msgids = {}
		self.strings_dict = {}
		self.parse_ili()

	def parse_ili(self):
		ili_lines = file(self.ili_fname, "rb").readlines()
		for line_num, line in enumerate(ili_lines):
			line = line.replace('\r', '').replace('\n', '').strip()
			if not line:
				continue
			if line.startswith('['):
				if line == "[MsgID]":
					mode = MODE_MSGID
				elif line == "[StringTable]":
					mode = MODE_STRINGTABLE
				else:
					mode = MODE_NONE
				continue
			if mode == MODE_MSGID:
				msgid, msg = re.findall("^(\d+)=(.*)", line)[0]
				self.msgids[int(msgid)] = msg
			elif mode == MODE_STRINGTABLE:
				if line.startswith('Str'):
					msgid, param_number, string_tuple = re.findall("^Str(\d+).(\d+)=(.*)", line)[0]
					li = self.strings_dict.setdefault((int(msgid), int(param_number)), [])
					li.append(string_tuple.split(','))

	def format_msg(self, msgid, params):
		try:
			format = list(self.msgids[msgid])
		except KeyError:
			return None
		paramidx = 0
		format_parameters = []
		i = 0
		while i < len(format):
			if format[i] == '%':
				if i+1 >= len(format):
					break
				if format[i+1] == '%': # Ignore escaped %
					pass
				elif '1' <= format[i+1] <= '9':
					try:
						possible_strings = self.strings_dict[(msgid, int(format[i+1])-1)]
					except KeyError:
						return None
					for ps in possible_strings:
						if int(ps[0]) <= params[paramidx] <= int(ps[1]):
							format_parameters.append(ps[2])
							break
					else:
						format_parameters.append("[0x%X]" % params[paramidx])
					format[i+1] = 's' # Hack - this will be replaced by the format string operator
					paramidx += 1
				else:
					format_parameters.append(params[paramidx])
					paramidx += 1
				i += 2
			else:
				i += 1
		return "".join(format) % tuple(format_parameters)

