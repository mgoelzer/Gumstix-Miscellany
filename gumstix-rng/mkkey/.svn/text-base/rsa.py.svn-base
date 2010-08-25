#!/usr/bin/python

import sys, struct, os, string, signal, subprocess, logging, logging.handlers, time, getopt

def usage():
	print
	print "Short message RSA encryption wrapper (uses openssl)"
	print
	print "Usage:  %s [-e|-d] keyfile -i infile -o outfile"%sys.argv[0]
	print
	print "Keyfile options:"
	print
	print "  -e,--enrypt-pubkey=             Public key for encryption"
	print "  -d,--decrypt-privkey=           Private key for decryption"
	print 
	print "Example:  echo \"Hello\" | %s -e public.pem > cyphertextfile"%sys.argv[0]
	print "          %s -d private.pem -i cyphertextfile"%sys.argv[0]
	print
	print "Input defaults to stdin, output defaults to stdout."
	print "Message cannot be longer than RSA key length -- few bytes only."
	print
	

def encrypt_rsa(pubkey_filename, fp_src, fp_dst):
	openssl = subprocess.Popen(
		['/usr/bin/openssl', 
		'rsautl', 
		'-encrypt', 
		'-inkey', pubkey_filename, 
		'-pubin'], 
		shell=False, 
		stdin=fp_src,
		stdout=subprocess.PIPE, 
                stderr=sys.stderr
		)
	b64 = subprocess.Popen(
                ['/usr/bin/base64'],
                shell=False, 
		stdin=openssl.stdout, 
		stdout=subprocess.PIPE,
                stderr=sys.stderr
                )
	(sout,serr) = b64.communicate()
	print >>fp_dst, sout

def decrypt_rsa(privkey_filename, fp_src, fp_dst):
        b64 = subprocess.Popen(
                ['/usr/bin/base64', '-d'],
                shell=False,
                stdin=fp_src,
                stdout=subprocess.PIPE,
                stderr=sys.stderr
                )
	openssldec = subprocess.Popen(
		['/usr/bin/openssl', 'rsautl', '-decrypt', 
		'-inkey', privkey_filename],
		shell=False,
        	stdin=b64.stdout,
        	stdout=subprocess.PIPE, 
        	stderr=sys.stderr
		)
	decrypted_out = openssldec.communicate()[0]
	print >>fp_dst, decrypted_out

def main():
	try:
		fpo = sys.stdout
		fpi = sys.stdin
		key_filename = ''
		direction = ""
		try:
			opts, args = getopt.getopt(sys.argv[1:], "he:d:i:o:", ["help", "encrypt-pubkey=", "decrypt-privkey=","infile=", "outfile="])
			#print opts
			for o in opts:
				if (o[0]=="--help") or (o[0]=="-h"):
					usage()
					return 1
				if (o[0]=="--encrypt") or (o[0]=="-e"):
					key_filename = o[1]
					direction = "encrypt"
				if (o[0]=="--decrypt") or (o[0]=="-d"):
					key_filename = o[1]
					#print o[1]
					direction = "decrypt"
				if (o[0]=="--outfile") or (o[0]=="-o"):
					fpo = open(o[1],'wb')
				if (o[0]=="--infile") or (o[0]=="-i"):
					fpi = open(o[1],'rb')

		except getopt.GetoptError, err:
			print str(err)
			usage()
			return 1

		if (direction=="encrypt"):
			encrypt_rsa(key_filename, fpi, fpo)
		elif (direction=="decrypt"):
			decrypt_rsa(key_filename, fpi, fpo)
		else:
			print "You must specify encrypt or decrypt"
		 	usage()
			return 1

	except KeyboardInterrupt:
		return 0

if __name__ == "__main__":
	sys.exit(main())
