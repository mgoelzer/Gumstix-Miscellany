#!/usr/bin/python

import sys, struct, os, string, re, subprocess, timeit

DBPOP = '/home/root/gumstix-rng/bin/pop-rand'

def sqlreadr(bcnt):
	ret = []
	popprog = subprocess.Popen([DBPOP, ("%i"%bcnt)], stdout=subprocess.PIPE)
	(sout,serr) = popprog.communicate()
	for l in string.split(sout,'\n'):
		if (len(l.strip())!=0):
			# print ">>> %s"%l
			ret.append(int(l))
	return ret

def mk_hexstr(byteslist):
	s = ''
	i = 0
	for b in byteslist:
		s = s + ("%02x"%b)
		if (i%16==15):
			s = s + '\n'
		i+=1
	return s

def mk_uint32(byteslist):
	i = 0
	for j in range(4):
		#print "i=%i,new byte=%i"%(i,byteslist[j])
		i |= byteslist[j]
		if (j<3):
			i <<= 8
	#print "i=%i"%i
	return i

def main():
	try:
		pat = re.compile('\\/(?P<name>[a-zA-Z0-9\\-]+)')
		print "Content-type: text/plain; charset=iso-8859-1\n\n"
		if (os.environ.has_key('REQUEST_URI')):
			REQUEST_URI=os.environ['REQUEST_URI']
			#print "REQUEST_URI=%s"%REQUEST_URI
			#for item in os.environ.keys():
			#	print "%s=%s"%(item,os.environ[item])
			m = pat.match(REQUEST_URI)
			req = m.group('name')
		else:
			req = '4096'
		# print "req=%s"%req
		if (req=='bit'):
			b = sqlreadr(1)[0]
			if (b&1==1):  
				print "HEADS"
			else:  
				print "TAILS"
		elif (req=='seed'):
			r4 = sqlreadr(4) 
			# print r4
			print "%u" % mk_uint32( r4 )
		elif (req=='4'):
			# print "32 bits"
			r32 = mk_hexstr( sqlreadr(4) )
			print "%s"%r32
		elif (req=='16'):
			# print "!28 bits"
			r16 = mk_hexstr( sqlreadr(16) )
			print "%s"%r16
		elif (req=='4096'):
			# print "4kb block"
			for i in range(4096/16):
				r16 = sqlreadr(16)
				print mk_hexstr( r16 ),
		else:
			print "UNRECOGNIZED URL"
	except:
		print "Error"
		return 0


if __name__ == "__main__":
	sys.exit(main())
