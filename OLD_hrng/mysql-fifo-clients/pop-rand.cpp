#include <mysql.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include "common.hh"

void usage(char** argv)
{
	printf("Usage:  %s [NUM_BYTES]\n",argv[0]);
	printf("where NUM_BYTES specifies how many random bytes to return; default=1\n");
	printf("\n");
}

int main(int argc,char** argv) {
   // How many bytes requested?
   int bcnt = 1; // default
   if (argc>1)
   {
     bcnt = atoi(argv[1]);
     if (bcnt==0)
     {
	usage(argv);
	return 1;
     }
   }
   
   MYSQL *conn = NULL;
   if (!(conn=connect_rand_db())) return 1;

   for(int i=0; i<bcnt; ++i)
   {
	if (rebalance(conn)!=0) { fprintf(stderr,"Rebalance failed; quiting\n"); return 1; }
	if (count_tbl(conn,"rd")==0) { fprintf(stderr,"No data; quiting\n"); return 1; }

	int b = atomic_dequeue_byte(conn);
   	printf("%d\n",b);
   }

   printf("Final totals are rd_rows=%i, rd256_rows=%i (includes any rebal/purge effects)\n", count_tbl(conn,"rd"), count_tbl(conn,"rd256"));

   mysql_close(conn);
   return 0;
}
