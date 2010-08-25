#include <mysql.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <map>
#include <algorithm>
#include <iostream>
#include "common.hh"

void usage(char** argv)
{
	printf("Usage:  %s\n",argv[0]);
	printf("\n");
}

int main(int argc,char** argv) {
   MYSQL *conn = NULL;
   if (!(conn=connect_rand_db())) return 1;

   MYSQL_RES *res;
   MYSQL_ROW row;

	if (mysql_query(conn, "SELECT COUNT(*) FROM rd"))
        {
                fprintf(stderr, "%s\n", mysql_error(conn));
                return -1;
        }
        res = mysql_use_result(conn);
        if ((row = mysql_fetch_row(res)) == NULL) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		return -1;
        }
        int n = atoi(row[0]);
        mysql_free_result(res);
	if (n<1024)
		printf("Buffering %d random bytes\n",n);
	else if (n<(1024*1024))
		printf("Buffering %.2f kb random data\n",float(n)/1024);
	else
		printf("Buffering %.2f mb random data\n",float(n)/float(1024*1024));

	//
	// Show relative frequencies of each possible bit and each byte
	//
//	if (mysql_query(conn, "SELECT IFNULL(SUM(one_bits),0),IFNULL(SUM(zero_bits),0) FROM stats_bit"))
	if (mysql_query(conn, "SELECT SUM((b&1) + ((b>>1)&1) + ((b>>2)&1) + ((b>>3)&1) + ((b>>4)&1) + ((b>>5)&1) + ((b>>6)&1) + ((b>>7)&1)) AS onebits, (COUNT(*)*8) AS allbits FROM rd"))
        {
                fprintf(stderr, "%s\n", mysql_error(conn));
                return -1;
        }
        res = mysql_use_result(conn);
        if ((row = mysql_fetch_row(res)) == NULL) {
                fprintf(stderr, "%s\n", mysql_error(conn));
                return -1;
        }
        int ones_cnt = atoi(row[0]);
        int total_cnt = atoi(row[1]);
        mysql_free_result(res);
	if (total_cnt>0)
	{
		float percent_ones = 100.0*float(ones_cnt)/float(total_cnt);
		float percent_zeros = 100.0 - percent_ones;
        	printf("Ones:  %d / %d = %.2f%%\n",ones_cnt,total_cnt,percent_ones);
	}

	printf("\n");
	printf(" Val | Count   \n");
	printf("-----+---------\n");

	std::map<int,int> valsum;
	for(int i=0; i<256; ++i) valsum.insert(std::make_pair<int,int>(i,0));
	//memset(valsum,0,sizeof(valsum));
//	if (mysql_query(conn, "SELECT val,IFNULL(SUM(cnt),0) FROM stats_byte GROUP BY val"))
	if (mysql_query(conn, "SELECT b,COUNT(b) AS cnt FROM rd GROUP BY b ORDER BY cnt DESC"))
        {
                fprintf(stderr, "%s\n", mysql_error(conn));
                return -1;
        }
        res = mysql_use_result(conn);
        while ((row = mysql_fetch_row(res)) != NULL) {
        	int val = atoi(row[0]);
//        	valsum[val] = atoi(row[1]);
		int cnt = atoi(row[1]);
		//printf("'%s'   '%s'\n",row[0],row[1]);
        	printf(" %3d | %6d\n",val,cnt);
	}
        mysql_free_result(res);

/*
	//for(int i=0; i<256; ++i)
	std::multimap<int,int> valsum_inverted;
	//std::sort(valsum.begin(), valsum.end());
	std::map<int,int>::const_iterator it = valsum.begin();
	for(;it!=valsum.end();++it)
	{
		valsum_inverted.insert(std::make_pair<int,int>(it->second,it->first));
	}
	std::multimap<int,int>::reverse_iterator itmm = valsum_inverted.rbegin();
	for(;itmm!=valsum_inverted.rend();++itmm)
	{
        	printf(" %3d | %6d\n",itmm->second, itmm->first);
		//valsum[i]);
	}
*/

   mysql_close(conn);
   printf("\n");
}
