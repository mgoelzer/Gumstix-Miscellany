#include <openssl/rsa.h>
#include <openssl/bio.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/bn.h>
#include <openssl/pem.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include "common.hh"

inline void dprint(std::string const& s)
{
	fprintf(stderr,s.c_str());
}
inline void dprint(std::string const& s,int i)
{
	fprintf(stderr,s.c_str(),i);
}

void usage(char** argv)
{
        printf("\n");
        printf("This program generates random RSA and AES keys.\n");
        printf("\n");
        printf("Usage:  %s port\n\n",argv[0]);
        printf("\n");
        printf("\n");
}

//
// Retrieves bcnt bytes of entropy and adds to openssl internal RNG state
//
int feed_rng(int bcnt)
{
	MYSQL *conn = NULL;
	if (!(conn=connect_rand_db())) return 1;

	std::auto_ptr<char> ap_buf = get_rand_buffer(bcnt,conn);

	RAND_add(ap_buf.get(),bcnt,bcnt);

	mysql_close(conn);

	return 0;
}

void openssl_last_error()
{
	long err = ERR_get_error();
	fprintf(stderr,"\nopenssl error:  %d\n",err);
}

int emit_aes_key(FILE* fp, int key_bcnt)
{
	std::auto_ptr<unsigned char> ap_bytes( new unsigned char[key_bcnt] );
	memset(ap_bytes.get(),0,key_bcnt);
	int ret = RAND_bytes(ap_bytes.get(), key_bcnt);
	if (ret != 1) { openssl_last_error(); return 1; }

	BIO *bio, *b64;
	b64 = BIO_new(BIO_f_base64());
	bio = BIO_new(BIO_s_mem()); //BIO_new_fp(fp, BIO_NOCLOSE);
	bio = BIO_push(b64, bio);
	BIO_write(bio, ap_bytes.get(), key_bcnt);
	BIO_flush(bio);

	std::string aeskey_plaintext;
	BUF_MEM *bptr;
	BIO_get_mem_ptr(bio, &bptr);
	char *pstr = new char[ bptr->length+1 ];
	memset(pstr,0,bptr->length+1);
	memcpy(pstr,bptr->data,bptr->length);
	aeskey_plaintext = pstr;
	delete pstr;

	std::cout << aeskey_plaintext << std::endl;

	BIO_free_all(bio);

	return 0;
}


int main(int argc,char** argv) 
{
	OpenSSL_add_all_algorithms();

	if (argc<3) { 
		fprintf(stderr,"\nNot enough arguments, amigo\n");
		usage(argv); 
		return 1; 
	}

	int key_bcnt = atoi(argv[2])/8;

	// Seed openssl internal RNG non-cryptographically at first
	//dprint("Seeding internal rng\n");
	RAND_load_file("/dev/urandom",1024);

	bool bklen_valid = true;
	cipher_t cipher = CIPHER_UNKNOWN; 
	if (strcasecmp(argv[1],"rsa")==0) 
	{
		cipher = CIPHER_RSA;
		if ((key_bcnt<=512/8)||(key_bcnt > 16384/8)) bklen_valid = false;

	} else if (strcasecmp(argv[1],"aes")==0) {
		cipher = CIPHER_AES;
		if ((key_bcnt!=16)&&(key_bcnt!=32)) bklen_valid = false;
	} 

	if ((cipher==CIPHER_UNKNOWN) || (!bklen_valid))
	{
		fprintf(stderr,"\nUnknown cipher or illegal key length requested\n");
		usage(argv);
		return 1;
	}

	// Add entropy to RNG
	feed_rng(key_bcnt);

	// Read passphrase (for RSA keys only) from stdin
	std::string pw;
	if (cipher==CIPHER_RSA)
	{
		char buf[1024];
		memset(buf,0,sizeof(buf));
		while (!std::cin.getline(buf,sizeof(buf)-1,'\n').eof())
		{
			pw += buf;
			if (cipher==CIPHER_RSA) break; // RSA passphrases, 
						      // cannot span >1 line
			memset(buf,0,sizeof(buf));
		}
	}

	// Generate and emit the key
	int ret;
	if (cipher==CIPHER_RSA) ret=emit_rsa_private_key(stdout,key_bcnt,pw);
	else if (cipher==CIPHER_AES) ret=emit_aes_key(stdout,key_bcnt);

	RAND_cleanup();

	if (ret != 0) fprintf(stderr,"\nExiting - FAILED\n");
	return ret;
}
