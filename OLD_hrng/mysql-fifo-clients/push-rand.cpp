#include <mysql.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <string>
#include "common.hh"

// Accumulators
static int one_bits = 0;
static int zero_bits = 0;
static int bytevals[255];

void accumulate_tallys(int b)
{
	for(int i=0; i<8; ++i)
	{
		if (((b>>i) & (0x01))!=0) one_bits++;
		else zero_bits++;
	}

	bytevals[b]++;
}

int update_tallys(MYSQL *conn)
{
   char s[1024];
   memset(s,0,sizeof(s));
   sprintf(s,"INSERT INTO stats_bit (one_bits,zero_bits) VALUES (%d,%d)",one_bits,zero_bits);
   if (mysql_query(conn, s))
   {
        fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn));
        return -1;
   }

   for(int i=0; i<256; ++i)
   {
	if (bytevals[i]==0) continue;
   	memset(s,0,sizeof(s));
   	sprintf(s,"INSERT INTO stats_byte (val,cnt) VALUES (%d,%d)",i,bytevals[i]);
   	if (mysql_query(conn, s))
   	{
        	fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn));
        	return -1;
   	}
   }
}

int main(int argc,char** argv) {
   MYSQL *conn = NULL;
   if (!(conn=connect_rand_db())) return 1;

   zero_bits = one_bits = 0;
   memset(bytevals,0,sizeof(bytevals));

   int n = 0;
   char buf[32];
   std::string hexbuf;
   memset(buf,0,sizeof(buf));
   while(!std::cin.getline(buf,sizeof(buf)-1,'\n').eof()) 
   {
     // printf("%s ",buf);
     int b = atoi(buf);
     assert((b>=0)&&(b<=255));

     char hh[3];
     hh[0] = (tohex((unsigned char)((b & 0xf0)>>4)));
     hh[1] = (tohex((unsigned char)(b&0x0f)));
     hh[2] = '\0';
     hexbuf += hh;

     // fprintf(stdout,"b=0x%02x,hexbuf=%s\n", b, hexbuf.c_str());

     accumulate_tallys(b);

     if (hexbuf.size() == 512)
     {
     	fprintf(stdout,"rd256:  writing hexbuf with %d length bytes\n", (hexbuf.size()/2) );
	atomic_enqueue_block(conn,hexbuf);
	n += 256;
	hexbuf = "";
     	//fprintf(stdout,"Rebalancing:  ");
        if (rebalance(conn)!=0) { fprintf(stderr, "[%d] Rebalance failed; last mysql error=%s\n", __LINE__, mysql_error(conn)); break; } 
     }

     memset(buf,0,sizeof(buf));
   }

   for(int i=0; i<hexbuf.size(); i+=2)
   {
     int b0 = int( fromhex((char)hexbuf.c_str()[i]) );
     int b4 = int( fromhex((char)hexbuf.c_str()[i+1]) );
     int b = (b0<<4) | b4;
     //fprintf(stdout,"Writing:  %d\n",int(b));
     if (atomic_enqueue_byte(conn,b)!=0) { fprintf(stderr, "[%d] Atomic enqueue byte failed; last mysql error=%s\n", __LINE__, mysql_error(conn)); return 1; }
     n++;
   }
   //fprintf(stdout,"rd: wrote %d bytes\n", (hexbuf.size()/2) );

   //fprintf(stdout,"Rebalancing again:  ");
   rebalance(conn);

   update_tallys(conn);

   printf("DONE -- enqueued %d bytes for final rd rows=%i, rd256 rows=%i (includes any rebal/purge effects)\n", n, count_tbl(conn,"rd"), count_tbl(conn,"rd256"));

   mysql_close(conn);

}
