#!/usr/bin/python

import serial, sys, struct, os, string, signal, subprocess, logging, logging.handlers, glob, time

BLOCK_SIZE = 16
outdb_writer_prog = "/home/root/hrng/bin/push-rand"
LOG_FILENAME = "/var/log/geiger-collector.log"
n = 0
randbytes = []

#def add_to_statistics(randlist):
#	#for b in randlist:
#		# print "%02x"%(b),
#	#print

#
#def get_last_filename(dirname):
#	ls = os.listdir(dirname)
#	if (len(ls)==0):
#		# print "Latest file in %s:  000000000000.bin"%(dirname)
#		return '000000000000.bin'
#	ls.sort(reverse=True)
#	# print "Latest file in %s:  '%s'"%(dirname,ls[0])
#	return ls[0]
#
#def append_to_outfile(randlist,outpath):
#	global n
#	m = int(string.replace(get_last_filename(outpath),'.bin',''))
#	if (m>n):
#		n = m
#	n += 1
#	filename = "%s/%012i.bin"%(outpath,n)
#	# print "new filename=%s"%(filename)
#	f = open(filename,"wb")
#	for b in randlist:
#		structure = struct.pack('B',b)
#		f.write(structure)
#	f.close()

def append_to_outdb(randlist):
	s = ''
	for b in randlist:
		#print ("%s "%b),
		#log.debug("%s "%b)
		s = s + ("%s\n"%b)
	#print
	#log.debug("sending to push_byte:  %s"%(s))
	pushprog = subprocess.Popen([outdb_writer_prog], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	(sout,serr) = pushprog.communicate(s)
	for sout_line in sout.split('\n'):
		log.debug("  %s"%sout_line)
	for serr_line in serr.split('\n'):
		log.debug("  %s"%serr_line)

def signal_handler_quit(signal, frame):
	global randbytes
	log.debug("Trapped signal 3 (SIGQUIT) ==> will flush %d bytes and exit"%(len(randbytes)))
	if (len(randbytes)>0):
		append_to_outdb(randbytes)
	log.debug("Shutdown at %s" % time.strftime('%X %x %Z'))
	sys.exit(0)

def signal_handler_term(signal, frame):
    	log.debug("***** Trapped signal 15 (%i; SIGTERM)"%signal)
	raise IOError("SIGTERM")

def main():
	global randbytes, retry_interval
	try:
		ser = serial.Serial('/dev/ttyUSB0',9600)
		
		# Successful connection, reset retry interval for future disconnections
		retry_interval = 1

		while (1):
			randbyte = 0
			for i in range(8):
				c = ser.read(1)
				# print c,
				if (c=="1"):
					randbyte |= 1
				if (i<7):
					randbyte <<= 1
			# print ""
			# print "randbyte = %x"%(randbyte)
			randbytes.append(randbyte)
			# print "%x"%randbyte
			if ((len(randbytes) % 32)==0):
				log.debug("randbytes:  %i of %i bytes needed to dump"%(len(randbytes),BLOCK_SIZE) )
			if (len(randbytes) >= BLOCK_SIZE):
				log.debug("randbytes:  %i bytes ready to push onto FIFO"%(BLOCK_SIZE))
				append_to_outdb(randbytes)
				randbytes = []
	except KeyboardInterrupt:
		log.debug("Caught Ctrl+C; flushing %i bytes to FIFO"%(len(randbytes)))
		if (len(randbytes)>0):  append_to_outdb(randbytes)
		return 0

if __name__ == "__main__":
	# Set up logging
	log = logging.getLogger('Geiger Collector Log')
	log.setLevel(logging.DEBUG)
	handler = logging.handlers.RotatingFileHandler(LOG_FILENAME, maxBytes=524288, backupCount=5)
	log.addHandler(handler)
	log.info('---------')
	log.info('Start up at %s' % time.strftime('%X %x %Z'))
	print "Log at %s"%LOG_FILENAME

	r = -1
	signal.signal(signal.SIGQUIT, signal_handler_quit)
	signal.signal(signal.SIGTERM, signal_handler_term)
	retry_interval = 1
	while (1):
		try:
			r = main()
		except serial.SerialException:
			try:
				log.debug("Serial connection error; retrying in %i seconds..."%retry_interval)
				time.sleep(retry_interval)
				retry_interval *= 2
			except KeyboardInterrupt:
				break
			continue

	log.info('Shutdown at %s' % time.strftime('%X %x %Z'))
	sys.exit(r)

