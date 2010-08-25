// gcc -o fftw fftw.c -lfftw3 -lm

// FFTW demo with wav file
// (C) 2005 www.captain.at

#include <stdio.h>
#include <stdlib.h>
#include <fftw3.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>

static double *rdata = NULL, *idata = NULL;
static fftw_plan rplan, iplan;
static int last_fft_size = 0;

int i,m,k;
char erg[35];
char erg2[35];
double absval;

int cnt_zeros = 0, cnt_ones = 0;
int cnt_00s = 0, cnt_11s = 0, cnt_01s = 0, cnt_10s = 0;

#define ZZZZ			0
#define ZZZO			1
#define ZZOZ			2
#define ZZOO			3
#define ZOZZ			4
#define ZOZO			5
#define ZOOZ			6
#define ZOOO			7
#define OZZZ			8
#define OZZO			9
#define OZOZ			10
#define OZOO			11
#define OOZZ			12
#define OOZO			13
#define OOOZ			14
#define OOOO			15
int cnt4_arr[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int tally_bit_pattern(int b0, int b1, int b2, int b3) {
	return (b0==0 ? 0 : 8) + (b1==0 ? 0 : 4) + (b2==0 ? 0 : 2) + (b3==0 ? 0 : 1);
}
int bits_stack[4];
int bits_stack_pos = 0;
void push_bit(char bwhich) {
	bits_stack[bits_stack_pos++] = ((bwhich==0x00) ? 0 : 1);
	if(bits_stack_pos==4) {
		int bit_pattern_bucket = tally_bit_pattern(bits_stack[0],bits_stack[1],bits_stack[2],bits_stack[3]);
		if (bit_pattern_bucket<0 || bit_pattern_bucket>15) printf("ERROR!!\n");
		cnt4_arr[bit_pattern_bucket]++;
		bits_stack_pos = 0;
	}
}

void usage() {
	printf("Usage:  fft input_file sameple_rate rand_bits_outfile [FFT|NOFFT}\n\n");
	printf("Ex:     fft file.wav 44100 /media/ram/bits.bin NOFFT\n\n");
	printf("Options:\n");
	printf("   input_file               WAV file, 16-bit/sample, LE\n");
	printf("                            Must be stripped of WAV header,\n");
        printf("                            use --separate-files in a record\n");
	printf("   sample_rate              eg, 44100, 88200, 10000 samples/sec\n");
	printf("   rand_bits_outfile        file to write random bits to, may\n");
	printf("                            be /dev/null\n");
	printf("   FFT/NOFFT                specify one of these; data.fft/data.in\n");
	printf("                            will only be produced if FFT spec'd\n");
	printf("                            NOFFT => much faster\n\n");
}

int main(int argc, char** argv) {
	int fDoFft = 0;
	double cc;
	char wavfile_str[512];
	char bitfile_str[512];
	int v, val, val2;
	double* A = NULL;
	int n = -1;
	FILE *wavfile;
	FILE *raw_outfile;
	FILE *fft_outfile;
	FILE *bitfile;
	int samplerate;
	struct stat st;
	int bandwidth = samplerate / 2;
	int nhalf;
	double correction;
	memset(wavfile_str,0,512);
	memset(bitfile_str,0,512);
	if (argc < 5) {
		usage();
		return -1;
	}

	strcpy(wavfile_str,argv[1]);
	strcpy(bitfile_str,argv[3]);
	if (argv[4][0]=='N' || argv[4][0]=='n') 
		fDoFft = 0;
	else
		fDoFft = 1;

	stat(wavfile_str,&st);
	n = st.st_size / 2;
	samplerate = atoi(argv[2]);

	A = (double*)fftw_malloc(n * sizeof(double));
	if (A==NULL) { 
		fprintf(stderr,"Error:  no mem\n"); return -1; 
	}
	fprintf(stderr,"input file:  %s, %d bytes, %d samples\n",wavfile_str,st.st_size, n);
	fprintf(stderr,"bit file:  %s, FFT:  %s\n",bitfile_str,(fDoFft?"yes":"no"));

	nhalf = n/2;
	correction = (double)samplerate / (double)n;
	wavfile = fopen(wavfile_str, "r");

	if (fDoFft) {
		raw_outfile = fopen("data.in", "w");
		fft_outfile = fopen("data.fft", "w");
	}
	bitfile = fopen(bitfile_str, "a+");

	// Clear out wave file header (46 bytes)
	for(v=0;v<46;v++) fgetc(wavfile);

	// read data and fill "A"-array
	int toggle = 0; // for counting non-overlapping 2-bit sequences
	char b = 0, bit0 = 0;
	for (v=0;v<n;v++) {
		char val[4];
		memset(val,0,4);
		val[0] = fgetc(wavfile); // little endian means
		val[1] = fgetc(wavfile); // read LSB before MSB
		signed int si = *(short int*)val;
		A[v] = (si+32768);
		//printf("%06d %02x%02x => %f\n", v, val[0], val[1], A[v]);
		if (fDoFft) {
			sprintf(erg2, "%d %f\n", v, A[v]);
			fputs(erg2, raw_outfile);
		}
		if ((v>0)&&(v%8==0)) {
			//printf("%d Wrote:  %x\n",v,b);
			fprintf(bitfile,"%c",b);
			b = 0;
		}

		char bit1 = ((si+32768)%2 == 0) ? (char)0x01 : (char)0x00;

		// ----------------- EXPERIMENTAL -----------------
		push_bit(bit1);
		// ------------------------------------------------

		if (toggle) {
			if ((bit0==0)&&(bit1==0)) cnt_00s++;
			else if ((bit0==1)&&(bit1==0)) cnt_10s++;
			else if ((bit0==0)&&(bit1==1)) cnt_01s++;
			else if ((bit0==1)&&(bit1==1)) cnt_11s++;
			toggle = 0;
		} else {
			toggle = 1;
		}

		if (bit1==0) cnt_zeros++; else cnt_ones++;

		b |= bit1;
		/*
		printf("%d) shift in %c ==> %c%c%c%c%c%c%c%c 0x%02x\n",
			v,
			(bit1==0)?'0':'1',
			((b&0x00000080)?'1':'0'),
			(b&0x00000040)?'1':'0',
			(b&0x00000020)?'1':'0',
			(b&0x00000010)?'1':'0',
			(b&0x00000008)?'1':'0',
			(b&0x00000004)?'1':'0',
			(b&0x00000002)?'1':'0',
			(b&0x00000001)?'1':'0',
			b);
		*/
		if (v%8!=7) b <<= 1;

		bit0 = bit1; // for next iteration
	}
	
	if (fDoFft) {
		// prepare fft with fftw
		rdata = idata = NULL; rplan = (void*)NULL;
		rdata = (double *)fftw_malloc(n * sizeof(double));
		idata = (double *)fftw_malloc(n * sizeof(double));
		// we have no imaginary data, so clear idata
		memset((void *)idata, 0, n * sizeof(double));
		// fill rdata with actual data
		for (i = 0; i < n; i++) { rdata[i] = A[i]; }

		// create the fftw plan
		rplan = fftw_plan_r2r_1d(n, rdata, idata, FFTW_R2HC, FFTW_ESTIMATE);
		if (rplan==NULL) { printf("Failed to make plan\n"); return -1; }
		// make fft
		fftw_execute(rplan);
		
		// post-process FFT data: make absolute values, and calculate
		//   real frequency of each power line in the spectrum
		m = 0;
		for (i = 0; i < (n-2); i++) {
			absval = sqrt(idata[i] * idata[i]);
			cc = (double)m * correction;
			sprintf(erg, "%f %f\n", cc, absval);
			fputs(erg, fft_outfile);
		m++;
		}
	}

	int sum = cnt_zeros + cnt_ones;
	/*
	printf("0/1, 00/01/10/11:  %02f%% %02f%%  %02f%% %02f%% %02f%% %02f%%\n",
		((float)cnt_zeros / (float)sum),
		((float)cnt_ones / (float)sum),
		((float)cnt_00s / (float)sum),
		((float)cnt_01s / (float)sum),
		((float)cnt_10s / (float)sum),
		((float)cnt_11s / (float)sum) );
	*/
	printf("0/1 00/01/10/11: %6d %6d  %6d %6d %6d %6d [ %6d,%6d,%6d,%6d,%6d,%6d,%6d,%6d,%6d,%6d,%6d,%6d,%6d,%6d,%6d,%6d ]\n",
		cnt_zeros, cnt_ones, cnt_00s,cnt_01s,cnt_10s,cnt_11s,
		cnt4_arr[0],cnt4_arr[1],cnt4_arr[2],cnt4_arr[3],
		cnt4_arr[4],cnt4_arr[5],cnt4_arr[6],cnt4_arr[7],
		cnt4_arr[8],cnt4_arr[9],cnt4_arr[10],cnt4_arr[11],
		cnt4_arr[12],cnt4_arr[13],cnt4_arr[14],cnt4_arr[15]
		);

	// housekeeping
	if (fDoFft) {
		fclose(raw_outfile);
		fclose(fft_outfile);
		fftw_free(rdata);
		fftw_free(idata);
		fftw_destroy_plan(rplan);
	}
	fclose(wavfile);
	fclose(bitfile);

	fftw_free(A);
	return 1;
}
