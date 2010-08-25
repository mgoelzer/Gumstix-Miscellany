#ifndef COMMON_H
#define COMMON_H

#include <mysql.h>
#include <string>

#define DB_NAME					"rand_volatile"
#define DB_USER					"root"
#define DB_PASS					""

MYSQL* connect_rand_db();
int atomic_enqueue_block(MYSQL *conn, std::string const& hstr);
int atomic_dequeue_block(MYSQL* conn, std::string & hex256);
int count_tbl(MYSQL* conn,std::string const& tblname);
int rebalance(MYSQL *conn);
int atomic_enqueue_byte(MYSQL *conn, int b);
int atomic_dequeue_byte(MYSQL* conn);
char tohex(unsigned char c);
unsigned char fromhex(char c);

#endif
