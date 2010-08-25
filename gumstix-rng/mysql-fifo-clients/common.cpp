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

std::auto_ptr<char> get_rand_buffer(int bcnt,MYSQL *conn)
{
	std::auto_ptr<char> ap_buf( new char[ bcnt ] );
	memset(ap_buf.get(),0,bcnt);

	for(int i=0; i<bcnt; ++i)
	{
		if (count_tbl(conn,"rd")==0) { 
			fprintf(stderr,"\n\nNo more data; quiting\n"); 
			break;
		}

		int b = atomic_dequeue_byte(conn);
		ap_buf.get()[i] = (char)b;
	}

	return ap_buf;
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
                        fprintf(stderr, "[%d] %s\n", __LINE__, mysql_error(conn));       
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
