#include <mysql.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <map>
#include <algorithm>
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

	if (mysql_query(conn, "SELECT id,LENGTH(bx) FROM rd256"))
        {
                fprintf(stderr, "%s\n", mysql_error(conn));
                return -1;
        }
        res = mysql_use_result(conn);
	float total_len_rd256 = 0.0;
	int rd256_rows = 0;
        while ((row = mysql_fetch_row(res)) != NULL) {
		int id = atoi(row[0]);
		total_len_rd256 = total_len_rd256 + (atoi(row[1]))/2.0;
		rd256_rows++;
		if (atoi(row[1]) != 512)
		{
			printf("Warning:  some rows in rd256 are not 256 bytes id=%d)\n",id);
		}
        }
        mysql_free_result(res);

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
	printf("Buffering %f random bytes:",(n+total_len_rd256));
	printf("  %f in rd256 (%d rows), %d in rd\n", total_len_rd256, rd256_rows, n);

	//
	// Show relative frequencies of each possible bit and each byte
	//
	if (mysql_query(conn, "SELECT IFNULL(SUM(one_bits),0),IFNULL(SUM(zero_bits),0) FROM stats_bit"))
        {
                fprintf(stderr, "%s\n", mysql_error(conn));
                return -1;
        }
        res = mysql_use_result(conn);
        if ((row = mysql_fetch_row(res)) == NULL) {
                fprintf(stderr, "%s\n", mysql_error(conn));
                return -1;
        }
        float ones_cnt = atof(row[0]);
        float total_cnt = ones_cnt+atof(row[1]);
        mysql_free_result(res);
	if (total_cnt>0)
	{
		float percent_ones = 100*ones_cnt/total_cnt;
		float percent_zeros = 100 - percent_ones;
        	printf("Ones to zeros:  %.6f%% : %.6f%%\n",percent_ones,percent_zeros);
	}

	printf("\n");
	printf(" Val | Count   \n");
	printf("-----+---------\n");

	std::map<int,int> valsum;
	for(int i=0; i<256; ++i) valsum.insert(std::make_pair<int,int>(i,0));
	//memset(valsum,0,sizeof(valsum));
	if (mysql_query(conn, "SELECT val,IFNULL(SUM(cnt),0) FROM stats_byte GROUP BY val"))
        {
                fprintf(stderr, "%s\n", mysql_error(conn));
                return -1;
        }
        res = mysql_use_result(conn);
        while ((row = mysql_fetch_row(res)) != NULL) {
        	int val = atoi(row[0]);
        	valsum[val] = atoi(row[1]);
		//printf("'%s'   '%s'\n",row[0],row[1]);
	}
        mysql_free_result(res);

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

   mysql_close(conn);
   printf("\n");
}
