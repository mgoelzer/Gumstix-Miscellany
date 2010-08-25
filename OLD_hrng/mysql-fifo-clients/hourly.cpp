#include <mysql.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include "common.hh"

// Accumulators
static int bytevals[255];

int get_current_max_id(MYSQL* conn)
{
	MYSQL_RES *res;
        MYSQL_ROW row;

	char s[1024];
        memset(s,0,sizeof(s));
        sprintf(s,"SELECT IFNULL(MAX(id),0) FROM stats_byte");
        if (mysql_query(conn, s))
	{
		fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn));
		return 0;
	}
	res = mysql_use_result(conn);
        if ((row = mysql_fetch_row(res)) == NULL) {
		fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn));
                return 0;
	}

        int maxid = atoi(row[0]);
        mysql_free_result(res);

	return maxid;
}

int insert_single_row(int byteval,int cnt,MYSQL *conn)
{
	char s[1024];
   	memset(s,0,sizeof(s));
   	sprintf(s,"INSERT INTO stats_byte (val,cnt) VALUES (%d,%d)",byteval,cnt);
	if (mysql_query(conn, s))
   	{
		fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn));
       		return -1;
   	}
}

int delete_rows(int byteval,int maxid,MYSQL* conn)
{
	char s[1024];
	sprintf(s,"DELETE FROM stats_byte WHERE val=%d AND id<=%d",byteval,maxid);
       	if (mysql_query(conn, s)) {
               	fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn));
       	        return -1;
       	}
	return 0;
}

int consolidate_byte(int byteval,MYSQL *conn)
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	int cnt = 0;
	int orig_maxid = get_current_max_id(conn);
	if (orig_maxid<=0) return 0;  // table empty!

	char dbg[1024];
	sprintf(dbg,"byte %d:  ",byteval);

	char s[1024];
   	memset(s,0,sizeof(s));
	sprintf(s,"SELECT IFNULL(cnt,0) FROM stats_byte WHERE val=%d",byteval);
	if (mysql_query(conn, s)) {
		fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn));
		return -1;
	}
	res = mysql_use_result(conn);
        while ((row = mysql_fetch_row(res)) != NULL) {
		cnt += atoi(row[0]);
        }
        mysql_free_result(res);

	sprintf(dbg+strlen(dbg),"cnt=%d",cnt);

	int ret = delete_rows(byteval,orig_maxid,conn);
	if (ret!=0) return ret;

	if (cnt>0) 
	{
		ret = insert_single_row(byteval,cnt,conn);
		if (ret!=0) return ret;
	}
	sprintf(dbg+strlen(dbg),"\n");

	if (cnt>0) printf("%s",dbg);

	return 0;
}

int main(int argc,char** argv) {
   MYSQL *conn = NULL;
   if (!(conn=connect_rand_db())) return 1;

   for(int i=0; i<256; ++i) consolidate_byte(i,conn);

   mysql_close(conn);
}
