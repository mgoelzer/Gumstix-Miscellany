#ifndef COMMON_H
#define COMMON_H

#include <mysql.h>
#include <string>
#include <memory>

#define DB_NAME					"rdb"
#define DB_USER					"root"
#define DB_PASS					""

MYSQL* connect_rand_db();
int count_tbl(MYSQL* conn,std::string const& tblname);
int atomic_enqueue_byte(MYSQL *conn, int b);
int atomic_dequeue_byte(MYSQL* conn);
char tohex(unsigned char c);
unsigned char fromhex(char c);
std::auto_ptr<char> get_rand_buffer(int bcnt,MYSQL *conn);

#endif
