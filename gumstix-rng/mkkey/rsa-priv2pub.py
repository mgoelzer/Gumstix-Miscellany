#!/usr/bin/python

import sys, struct, os, string, signal, subprocess, logging, logging.handlers, time, getopt

def usage():
	print
	print "Generates RSA public key based on private key"
	print
	print "Usage:  %s [-a] -i private_keyfile -o public_keyfile"%sys.argv[0]
	print
	print "Options:"
	print
	print "  -a,--aes-128             Private key passphrase read from stdin"
	print
	print "Example:  %s -i private.pem > public.pem"%sys.argv[0]
	print
	print "If '-a' is not specified, we assume private key has no password."
	print
	
def make_pubkey(fp_src_filename, fp_dst_filename, priv_uses_aes128):
	args = ['/usr/bin/openssl', 'rsa', '-pubout',
		'-in', fp_src_filename, '-out', fp_dst_filename]
	if (priv_uses_aes128):
		args.append('-aes128')
		args.append('-passin')
		args.append('stdin')
	passphrase_source = sys.stdin
	openssl = subprocess.Popen(
		args,
		shell=False, 
		stdin=sys.stdin)
	openssl.wait()

def main():
	try:
		b_aes128 = 0 
		try:
			opts, args = getopt.getopt(sys.argv[1:], "hai:o:", ["help", "aes128", "infile=", "outfile="])
			#print opts
			for o in opts:
				if (o[0]=="--help") or (o[0]=="-h"):
					usage()
					return 1
				if (o[0]=="--outfile") or (o[0]=="-o"):
					outfilename = o[1]
				if (o[0]=="--infile") or (o[0]=="-i"):
					infilename = o[1]
				if (o[0]=="--aes128") or (o[0]=="-a"):
					b_aes128 = 1

			if ((len(infilename)==0) or (len(outfilename)==0)):
				print "Required arugments missing."
				usage()
				return 1

		except getopt.GetoptError, err:
			print str(err)
			usage()
			return 1

		make_pubkey(infilename, outfilename, b_aes128)

	except KeyboardInterrupt:
		return 0

if __name__ == "__main__":
	sys.exit(main())
