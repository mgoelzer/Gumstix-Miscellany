#include <mysql.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <unistd.h>
#include "common.hh"

void usage(char** argv)
{
	printf("Usage:  %s [-b] [BCNT]\n",argv[0]);
	printf("where BCNT specifies how many random bytes to return (default=1)\n");
	printf("\nIf -b is specified, output is binary.  Otherwise, output is newline-\n");
	printf("separated ASCII integers written base-10.\n");
	printf("\n");
}

int main(int argc,char** argv) {
   // How many bytes requested?
   int bcnt = 1; // default
   int f_binary = 0;
   if (argc>2)
   {
     bcnt = atoi(argv[argc-1]);
     f_binary = 1;
     if ((bcnt==0) || (strcmp(argv[1],"-b")!=0))
     {
	usage(argv);
	return 1;
     }
   } else if (argc>1) {
     bcnt = atoi(argv[argc-1]);
     if (bcnt==0)
     {
	usage(argv);
	return 1;
     }
   }
   
   // Print out bytes
   MYSQL *conn = NULL;
   if (!(conn=connect_rand_db())) return 1;

   for(int i=0; i<bcnt; ++i)
   {
	if (count_tbl(conn,"rd")==0) { fprintf(stderr,"\n\nNo more data; quiting\n"); return 1; }

	int b = atomic_dequeue_byte(conn);
	if (f_binary)
		printf("%c",(char)b);
	else
		printf("%d\n",b);
   }

   //printf("Final total:  %i bytes\n", count_tbl(conn,"rd"));

   mysql_close(conn);
   return 0;
}
