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
#include <memory>

inline void dprint(std::string const& s)
{
	fprintf(stderr,s.c_str());
}
inline void dprint(std::string const& s,int i)
{
	fprintf(stderr,s.c_str(),i);
}

void keygen_cb(int signal,int nth_prime,void*)
{
//	static bool b_found1 = false;
//	if (signal==0) dprint(".");
//	else if (signal==1) dprint("\nTesting candidate for primality");
//	else if (signal==2) dprint("\nRejected liar non-prime");
//	else if (signal==3) {
//		if (!b_found1) {
//			dprint("\nFound large random prime #1");
//			dprint("\nBeginning search for second prime");
//			b_found1 = true;
//		} else {
//			dprint("\nFound large random prime #2\n");
//			b_found1 = false;
//		}
//	}
}

void usage(char** argv)
{
        printf("\n");
        printf("This program generates maximally entropic RSA and AES keys using\n");
        printf("hardware RNG randomness.\n");
        printf("\n");
        printf("Usage:  %s [rsa|aes] SIZE TIMEOUT\n\n",argv[0]);
        printf("where SIZE is [1024|2048|4096] for RSA, [128|256] for AES\n");
        printf("and TIMEOUT is seconds to wait for sufficient hardware entropy.\n");
//        printf("  -p              Indicates that stdin supplies a passphrase (RSA only)\n");
        printf("\nExamples:\n");
	printf("  %s aes 128 60      Generate AES 128 key, 60-second timeout\n",argv[0]);
        printf("  %s rsa 1024 0      Generate 1024-bit RSA key, don't wait\n",argv[0]);
        printf("\n");
        printf("\n");
}

//
// Retrieves bcnt bytes of entropy and adds to openssl internal RNG state
//
void feed_rng(int bcnt,int timeout_sec)
{
	static bool b_first = true;

	std::auto_ptr<char> ap_buf( new char[bcnt] );

	memset(ap_buf.get(),0,bcnt);  // TODO:  get random bytes!
	
	RAND_add(ap_buf.get(),bcnt,bcnt);
}

void openssl_last_error()
{
	long err = ERR_get_error();
	fprintf(stderr,"\nopenssl error:  %d\n",err);
}

int emit_rsa_private_key(FILE* fp,int key_bcnt, std::string const& passphrase)
{
	//dprint("Generating two large random primes for RSA key...\n");

	RSA* rsa = RSA_generate_key((8*key_bcnt),65537,keygen_cb,NULL);
	//RSA_print_fp(stdout,rsa,2);

	int ret;
	if (passphrase.size() == 0)
	{
		ret  = PEM_write_RSAPrivateKey(fp, rsa, NULL,NULL,0,NULL,
			NULL);
	} else {
		
		ret  = PEM_write_RSAPrivateKey(fp, rsa, EVP_aes_128_cbc(), //EVP_des_ede3_cbc(),
			NULL,0,NULL, (void*)passphrase.c_str());
	}
	if (ret==0) { openssl_last_error(); return 1; }
	
	RSA_free(rsa);

	return 0;
}

int emit_aes_key(FILE* fp, int key_bcnt, std::string const& pubkey)
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

	if (pubkey.size() > 0)
	{
		std::string aeskey_plaintext;
		BUF_MEM *bptr;
		BIO_get_mem_ptr(bio, &bptr);
		char *pstr = new char[ bptr->length+1 ];
		memset(pstr,0,bptr->length+1);
		memcpy(pstr,bptr->data,bptr->length);
		aeskey_plaintext = pstr;
		delete pstr;

		// Invoke OpenSSL binary to see if this is a valid public key
		std::cout << aeskey_plaintext << std::endl;
		std::cout << "size=" << aeskey_plaintext.size() << std::endl;
		execl("/usr/bin/openssl", "openssl", "help", (char*)0 );

	}

	BIO_free_all(bio);

	return 0;
}

typedef enum {CIPHER_UNKNOWN,CIPHER_RSA,CIPHER_AES} cipher_t;

int main(int argc,char** argv) 
{
	OpenSSL_add_all_algorithms();

	if (argc<4) { 
		fprintf(stderr,"\nNot enough arguments, amigo\n");
		usage(argv); 
		return 1; 
	}

	int key_bcnt = atoi(argv[2])/8;
	int to_sec = atoi(argv[3]);

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
	feed_rng(key_bcnt,to_sec);

	// Read passphrase (for RSA keys) or public key (for AES) from stdin
	std::string pw;
	char buf[1024];
	memset(buf,0,sizeof(buf));
	while (!std::cin.getline(buf,sizeof(buf)-1,'\n').eof())
	{
		pw += buf;
		if (cipher==CIPHER_RSA) break; // RSA passphrases, 
					      // cannot span >1 line
		memset(buf,0,sizeof(buf));
	}

	// Generate and emit the key
	int ret;
	if (cipher==CIPHER_RSA) ret=emit_rsa_private_key(stdout,key_bcnt,pw);
	else if (cipher==CIPHER_AES) ret=emit_aes_key(stdout,key_bcnt,pw);

	RAND_cleanup();

	if (ret != 0) fprintf(stderr,"\nExiting - FAILED\n");
	return ret;
}
