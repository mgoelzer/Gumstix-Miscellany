#!/usr/bin/python

import sys, struct, os, string, re, subprocess

STATPROG='/home/root/hrng/bin/status-rng'

def main():
	try:
		print "Content-type: text/plain; charset=iso-8859-1\n\n"

		upt = subprocess.Popen(["uptime"], stdout=subprocess.PIPE)
                (sout,serr) = upt.communicate()
                print "Uptime:  %s"%sout

		runningtime_min = 1
		m = re.search('up(?P<h>[\\s0-9]+):(?P<m>[0-9]+)',sout)
		if (m):
			runningtimestr_hours = m.group('h').strip()
			runningtimestr_min = m.group('m').strip()
			runningtime_sec = (int(runningtimestr_min)*60) + (int(runningtimestr_hours)*60*60)
			if (runningtime_sec > 60):
				runningtime_min = runningtime_sec / 60
			# print "'%i'"%runningtime_sec

		statprog = subprocess.Popen([STATPROG], stdout=subprocess.PIPE)
		(sout,serr) = statprog.communicate()

		m = re.search('Buffering (?P<bytes>[0-9]+)',sout)
		if (m):
			bcnt = int(m.group('bytes').strip())
			bperm = ((bcnt*8) / float(runningtime_min))
			print "Net gain %.2f bits/min"%bperm
			print 

		print sout

	except Exception,e:
		print "ERROR!"
		print e
		return 0


if __name__ == "__main__":
	sys.exit(main())
