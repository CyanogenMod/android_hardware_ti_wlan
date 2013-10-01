import sys
import struct
from message import parse_message
from iliparser import IliParser

BLOCK_SIZE = 256

def main(argv):
	if len(argv) != 2:
		print "Usage", argv[0], "<firmware.ili>"
		return 1

	ili_parser = IliParser(argv[1])

	while True:
		rec_len = sys.stdin.read(1)
		if not rec_len:
			break
		record = sys.stdin.read(ord(rec_len))
		try:
			msg_id, params = parse_message(record)
			print ili_parser.format_msg(msg_id, params)
		except:
			print "Could not parse message", repr(record)

if __name__ == "__main__":
	sys.exit(main(sys.argv))
