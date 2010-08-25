#!/usr/bin/python

import serial, sys, struct, os, string, signal, subprocess, logging, logging.handlers, glob, time, datetime, re

restats = re.compile('\s*(?P<cnt0s>[0-9]+)\s+(?P<cnt1s>[0-9]+)\s+(?P<cnt00s>[0-9]+)\s+(?P<cnt01s>[0-9]+)\s+(?P<cnt10s>[0-9]+)\s+(?P<cnt11s>[0-9]+)',re.IGNORECASE)
re_end = re.compile('\s+[-]+\s*')

def main(stats_file,bAppendToStatsFile):
	cnt0s = cnt1s = cnt00s = cnt01s = cnt10s = cnt11s = 0;
	try:
		f = open(stats_file,'r+')
		for line in f.readlines():
			if (re_end.match(line)):
				break;
			line = line[17:]
			m = restats.match(line)
			if (m):
				cnt0s = cnt0s + int(m.group('cnt0s'))
				cnt1s = cnt1s + int(m.group('cnt1s'))
				cnt00s = cnt11s + int(m.group('cnt00s'))
				cnt01s = cnt01s + int(m.group('cnt01s'))
				cnt10s = cnt10s + int(m.group('cnt10s'))
				cnt11s = cnt11s + int(m.group('cnt11s'))

		sum = float(cnt0s + cnt1s)
		per0s = (cnt0s / sum)
		per1s = (cnt1s / sum)
		per00s = cnt00s / (sum/2)
		per01s = cnt01s / (sum/2)
		per10s = cnt10s / (sum/2)
		per11s = cnt11s / (sum/2)
		s = "\n0/1 00/01/10/11:  ---------------------------------------------------------\n"
		s = s + ("                  %i %i  %i %i %i %i\n\n"%(cnt0s,cnt1s,cnt00s,cnt01s,cnt10s,cnt11s))
		s = s + ("0/1:              %f%% %f%%\n" % (per0s,per1s))
		s = s + ("00/01/10/11:      %f%% %f%% %f%% %f%%\n"%(per00s,per01s,per10s,per11s))
		s = s + "\n"
		f.seek(0,os.SEEK_END)
		print s
		if (bAppendToStatsFile):
			f.write(s)
		f.close()
	except KeyboardInterrupt:
		return 0

if __name__ == "__main__":
	if (len(sys.argv)<3):
		print "Usage:  parse_stats.py STATSFILE [FINALNONFINAL]"
		print "\"FINAL\" keywords means that the results should be written"
		print "to the end of the stats file.  Otherwise, stats file is ro."
		print ""
		sys.exit(-1)
	r = main(sys.argv[1], (sys.argv[2]=='f' or sys.argv[2]=='F'))
	sys.exit(r)

