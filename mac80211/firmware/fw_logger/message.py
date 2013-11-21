import struct

class ParserError(Exception): pass

class InvalidMessageLength(ParserError): pass
class InvalidMessageFormat(ParserError): pass
class InvalidParamtersCount(ParserError): pass

def parse_message_format(format):
	if format >= 7 or not format:
		raise InvalidMessageFormat

	format -= 1
	if format & 1:
		msg_id_size = 13
	else:
		msg_id_size = 11
	param_size = 1 << (format / 2)
	return msg_id_size, param_size
	
def parse_message(message):
	# All messages are at least 4 bytes long
	if len(message) < 3:
		raise InvalidMessageLength
	elif len(message) == 3:
		# Handle cases of 0 parameters
		message = message + '\x00'

	header, msg_info = struct.unpack_from("<BH", message)
	msg_level = header & 0x7
	msg_format = header >> 3

	msg_id_size, param_size = parse_message_format(msg_format)

	message_id = msg_info & ((1 << msg_id_size) - 1)
	params_count = msg_info >> msg_id_size

	if (params_count > 4):
		raise InvalidParamtersCount

	if param_size == 1:
		parameters = struct.unpack_from('B' * params_count, message[3:])
	elif param_size == 2:
		parameters = struct.unpack_from('<' + ('H' * params_count), message[3:])
	else: # param_size = 4
		parameters = struct.unpack_from('<' + ('L' * params_count), message[3:])

	return message_id, parameters
