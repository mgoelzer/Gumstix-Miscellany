#!/usr/bin/python

import serial, sys, struct, os, string, signal, subprocess, logging, logging.handlers, glob, time, datetime

BLOCK_SIZE = 32
outdb_writer_prog = "/home/root/gumstix-rng/bin/push-rand"
LOG_FILENAME = "/var/log/geiger-collector.log"
n = 0
randbytes = []

def append_to_outdb(randlist):
	s = ''
	dbg = ''
	for b in randlist:
		#print ("%s "%b),
		#log.debug("%s "%b)
		s = s + ("%s\n"%b)
		dbg = dbg + ("%s "%b)
	#print
	#log.debug("Sending to push_byte:  %s"%(s))
	pushprog = subprocess.Popen([outdb_writer_prog], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	(sout,serr) = pushprog.communicate(s)
	log.debug("push_byte:  %s %s"%(dbg,string.strip(sout.split('\n')[0])))
	#for sout_line in sout.split('\n'):
	#	if (len(string.strip(sout_line))>0):
	#		log.debug("  %s"%sout_line)
	for serr_line in serr.split('\n'):
		if (len(string.strip(serr_line))>0):
			log.debug("***  %s"%serr_line)

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

def read_bit(ser):
	ser.flushInput()
	tm0 = time.time()
	ser.read(1)
	tm1 = time.time()
	delta1 = tm1-tm0

	ser.flushInput()
	tm0 = time.time()
	ser.read(1)
	tm1 = time.time()
	delta2 = tm1-tm0


	if (delta2 > delta1):
		return '1'
	else:
		return '0'

def main():
	global randbytes, retry_interval
	try:
		ser = serial.Serial('/dev/ttyUSB0',9600)
		
		# Successful connection, reset retry interval for future disconnections
		retry_interval = 1

		while (1):
			randbyte = 0
			for i in range(8):
				c = read_bit(ser)
				# print c,
				if (c=="1"):
					randbyte |= 1
				if (i<7):
					randbyte <<= 1
			# print ""
			# print ">>> %f randbyte = %x"%(time.time(),randbyte)
			randbytes.append(randbyte)
			# print "%x"%randbyte
			#if ((len(randbytes) % 32)==0):
			#	log.debug("randbytes:  %i of %i bytes needed to dump"%(len(randbytes),BLOCK_SIZE) )
			if (len(randbytes) >= BLOCK_SIZE):
				#log.debug("randbytes:  %i bytes ready to push onto FIFO"%(BLOCK_SIZE))
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
			if (r==0):  break
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

