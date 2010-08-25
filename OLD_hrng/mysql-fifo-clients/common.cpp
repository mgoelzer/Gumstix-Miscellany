#include <mysql.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <string>
#include <sstream>
#include "common.hh"

#define RD_TARGET_ROW_COUNT				256*10000  // Must be multiple of 256, >=256
#define RD_MIN_ROW_COUNT				0
#define RD_MAX_ROW_COUNT				256*1000000  // Must be multiple of 256

#define RD256_MAX_ROW_COUNT				0

int purge_rd256_extra_rows(MYSQL *conn)
{
   char s[1024];
   int rd256_row_cnt = count_tbl(conn,"rd256");
   int cnt_delete_required = (rd256_row_cnt>RD256_MAX_ROW_COUNT) ? (rd256_row_cnt-RD256_MAX_ROW_COUNT) : 0;
   assert (cnt_delete_required >= 0);  // never negative

   if (cnt_delete_required > 0)
   {
	MYSQL_RES* res;
	MYSQL_ROW row;
   	memset(s,0,sizeof(s));
	sprintf(s,"SELECT id FROM rd256 ORDER BY id ASC LIMIT %d",cnt_delete_required);
        if (mysql_query(conn, s)) { fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn)); return -1; }
        res = mysql_use_result(conn);
	int max_delete_id = -1;
        while ((row = mysql_fetch_row(res)) != NULL) 
	{
        	int id = atoi(row[0]);
		max_delete_id = (id>max_delete_id) ? id : max_delete_id;
	}
	mysql_free_result(res);

	fprintf(stdout,"Deleteing %d rows from rd256\n", cnt_delete_required);
	sprintf(s,"DELETE FROM rd256 WHERE id<=%d",max_delete_id);
	if (mysql_query(conn, s)) { fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn)); return 1; }
   }
   return 0;
}

int atomic_enqueue_block(MYSQL *conn, std::string const& hstr)
{
   assert( hstr.size() == 512 ); // 512 hex chars = 256 bytes

   char s[1024];
   memset(s,0,sizeof(s));
   sprintf(s,"INSERT INTO rd256 (bx) VALUES ('%s')",hstr.c_str());
   if (mysql_query(conn, s)) { fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn)); return 1; }

   return 0;
}

int count_tbl(MYSQL* conn,std::string const& tblname)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  std::string q("SELECT COUNT(*) FROM ");
  q += tblname;
  if (mysql_query(conn, q.c_str()))
  {
        fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn));
        return -1;
  }
  res = mysql_use_result(conn);
  if ((row = mysql_fetch_row(res)) == NULL) {
	fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn));
        return -1;
  }
  int rows = atoi(row[0]);
  mysql_free_result(res);
  return rows;
}

void append_to_hex_string(int b, std::string& hexchars)
{
        char hh[3];
        hh[0] = (tohex((unsigned char)((b & 0xf0)>>4)));
        hh[1] = (tohex((unsigned char)(b&0x0f)));
        hh[2] = '\0';
        hexchars += hh;
}

int pack_rd_to_rd256(MYSQL *conn)
{
	std::string hexchars = "";
	while (hexchars.size() < 256*2)
	{ 
        	int b = atomic_dequeue_byte(conn);
		if (b<0) { fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn)); return 1; }
		append_to_hex_string(b, hexchars);
	} 

	int ret = atomic_enqueue_block(conn,hexchars);
	if (ret != 0) { fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn)); return 1; }
	else return 0;
}

int unpack_rd256_to_rd(MYSQL *conn)
{
	fprintf(stdout,"using rd256 to populate rd\n");
      std::string hex256;
      int ret = atomic_dequeue_block(conn,hex256);
      if (0 != ret)
      {
        fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn));
        return -1;
      }
      ///fprintf(stdout,"  hex256=%s (%d bytes)\n", hex256.c_str(), hex256.size()/2 );
      assert( hex256.size()/2 == 256 );
      for(int i=0; i<hex256.size(); i+=2)
      {
        char c1 = hex256.c_str()[i];
	char c2 = hex256.c_str()[i+1];
	int b0 = int( fromhex((char)hex256.c_str()[i]) );
	int b4 = int( fromhex((char)hex256.c_str()[i+1]) );
	int b = (b0<<4) | b4;
        int ret = atomic_enqueue_byte(conn,b);
	if (0 != ret) { fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn)); return 1; }
      }
      return 0;
}

int rebalance(MYSQL *conn)
{
  int rd_rows = count_tbl(conn,"rd");
  int rd256_rows = count_tbl(conn,"rd256");
  //fprintf(stdout,"rd_rows=%i, rd256_rows=%i, ", rd_rows, rd256_rows);

  //
  // Adjust rd to proper level (going to/from rd256)
  // 
  if (rd_rows > RD_MAX_ROW_COUNT)
  {
    // Collapse 256-row chunks from rd to rd256 until rb is back below target
    //fprintf(stdout,"using rd to populate rd256\n");
    do {
	if (pack_rd_to_rd256(conn)!=0) { fprintf(stderr, "Error during pack_rd_to_rd256()\n"); return 1; }
    } while (count_tbl(conn,"rd") > (256+RD_TARGET_ROW_COUNT));
  } else if ((rd_rows < RD_MIN_ROW_COUNT) && (rd256_rows > 0)) {
    // Expand rows from rd256 into rb until rb is back above target
    //fprintf(stdout,"using rd256 to populate rd\n");
    do {
	if (unpack_rd256_to_rd(conn)!=0) { fprintf(stderr, "Error during unpack_rd256()\n"); return 1; }
    } while ((count_tbl(conn,"rd") < RD_TARGET_ROW_COUNT) && (count_tbl(conn,"rd256")>0));
  } else {
    //fprintf(stdout,"no rebalance required\n");
  }

  int ret = purge_rd256_extra_rows(conn);
  if (ret!=0) { fprintf(stderr, "Error during purge()\n"); return 1; }

  // If anything changed, dbg print new totals
  //if ((rd_rows!=count_tbl(conn,"rd")) || (rd256_rows!= count_tbl(conn,"rd256")))
  //{
  //  fprintf(stdout,"New totals are rd_rows=%i, rd256_rows=%i after rebalance or rd256 purge\n", count_tbl(conn,"rd"), count_tbl(conn,"rd256"));
  //}

  return 0;
}

int atomic_enqueue_byte(MYSQL *conn, int b)
{
   //fprintf(stdout,"atomic_enqueue_byte:  b=%d\n",b);
   char s[256];
   memset(s,0,sizeof(s));
   sprintf(s,"INSERT INTO rd (b) VALUES (%d)",b);
   if (mysql_query(conn, s))
   {
	fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn));
	return -1;
   }
   return 0;
}

int atomic_dequeue_byte(MYSQL* conn)
{
	int b = -1;
	MYSQL_RES *res;
  	MYSQL_ROW row;
	while (1)
	{
		std::string q("SELECT id,b FROM rd WHERE id=(SELECT MIN(id) FROM rd)");
		if (mysql_query(conn, q.c_str()))
		{
		  fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn));
		  return -1;
		}
		res = mysql_use_result(conn);
		if ((row = mysql_fetch_row(res)) == NULL) {
			fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn));
			return -1;
		}
		int id = atoi(row[0]);
		b = atoi(row[1]);
		mysql_free_result(res);

		std::stringstream ss;
		ss << "DELETE FROM rd WHERE id=" << id;
		if (mysql_query(conn, ss.str().c_str()))
                {
                  fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn));
                  return -1;
                }

		if (mysql_query(conn, "SELECT ROW_COUNT()"))
                { 
                  fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn));
                  return -1;
                }
		res = mysql_use_result(conn);
                if ((row = mysql_fetch_row(res)) == NULL) {
                        fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn)
);       
                        return -1;
                }
                int cnt = atoi(row[0]);
                mysql_free_result(res);

		if (cnt==1) {
			break;
		} else {
			b = -1; // keep looping
			usleep(100*1000);
		}
	}
	return b;
}

int atomic_dequeue_block(MYSQL* conn, std::string & hex256)
{
	MYSQL_RES *res;
  	MYSQL_ROW row;
	while (1)
	{
		std::string q("SELECT id,bx FROM rd256 WHERE id=(SELECT MIN(id) FROM rd256)");
		if (mysql_query(conn, q.c_str()))
		{
		  fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn));
		  return -1;
		}
		res = mysql_use_result(conn);
		if ((row = mysql_fetch_row(res)) == NULL) {
			fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn));
			return -1;
		}
		int id = atoi(row[0]);
		hex256 = "";
		hex256.append(row[1]);
		mysql_free_result(res);

		std::stringstream ss;
		ss << "DELETE FROM rd256 WHERE id=" << id;
		if (mysql_query(conn, ss.str().c_str()))
                {
                  fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn));
                  return -1;
                }

		if (mysql_query(conn, "SELECT ROW_COUNT()"))
                { 
                  fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn));
                  return -1;
                }
		res = mysql_use_result(conn);
                if ((row = mysql_fetch_row(res)) == NULL) {
                        fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn)
);       
                        return -1;
                }
                int cnt = atoi(row[0]);
                mysql_free_result(res);

		if ((cnt==1)&&(hex256!="")) {
			break;
		} else {
			// keep looping
			usleep(100*1000);
		}
	}
	return 0;
}

static char htbl[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                         'a', 'b', 'c', 'd', 'e', 'f' };

char tohex(unsigned char c)
{
	assert(uint(c)>=0 && uint(c)<16);
	return htbl[uint(c)];
}

unsigned char fromhex(char c)
{
	assert((c>='0' && int(c)<='9') || 
		(c>='a' && int(c)<='f') ||
		(c>='A' && int(c)<='F'));
	if (int(c)>='0' && int(c)<='9') return char(int(c)-int('0'));
	else if (c>='a' && int(c)<='f') return char(int(c)-int('a')+10);
	else if (c>='A' && int(c)<='F') return char(int(c)-int('A')+10);
	else return char(0);
}

MYSQL* connect_rand_db()
{
   const char *server = "localhost";
   const char *user = DB_USER;
   const char *password = DB_PASS;
   const char *database = DB_NAME;

   MYSQL* conn = mysql_init(NULL);
   
   if (!mysql_real_connect(conn, server,
         user, password, database, 0, NULL, 0)) {
      fprintf(stderr, "%s\n", mysql_error(conn));
      return NULL;
   }

   return conn; 
}
