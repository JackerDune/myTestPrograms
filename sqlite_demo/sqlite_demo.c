#include <stdio.h>

#if 0
unsigned char  bitmap[128] = {0};
#define SOCKFD_SET(f,b)       (b[(f)/8] |=  (1 << ((f) & 7)))
#define SOCKFD_CLR(f,b)       (b[(f)/8] &= ~(1 << ((f) & 7)))
#define IS_SOCKFD_SET(f,b)    (b[(f)/8] &   (1 << ((f) & 7)))
#define TB_BITS_PER_LONG      		(sizeof(unsigned char) << 3)
#define TB_BITS_PER_CHAR      		(sizeof(unsigned char) << 3)
static int ffzb(unsigned long num)
{
	int i;
	int loopnum = ((sizeof(unsigned long) << 3) - 1);

	for(i = 0; i < loopnum; i++)
	{
		if((num & ((unsigned long)1 << i)) == 0)
			break;
	}

	return i;
}

static int find_first_zero(const unsigned char *addr, unsigned long size)
{
	const unsigned char *p = addr;
	unsigned char result = 0;
	unsigned char tmp;

	while (size & ~(TB_BITS_PER_CHAR - 1))
	{
		if (~(tmp = *(p++)))
			goto found;
		result += TB_BITS_PER_CHAR;
		size -= TB_BITS_PER_CHAR;
	}
	if (!size)
		return result;

	tmp = (*p) | (~0UL << size);
	if (tmp == ~0UL)	/* Are any bits zero? */
		return result + size;	/* Nope. */
found:
	return result + ffzb(tmp);
}
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "sqlite3.h"

int key = 0x1234;

int rscallback(void * p, int column, char ** column_val,char ** column_name)
{
	int i;
	*(int *)p = 0;

	for(i = 0; i < column; i++)
		printf("%s > %s\n",column_name[i], column_val[i]);
	printf("\n"); 
	
	return 0; 
}

int db_show(sqlite3 *db, char *ptable)
{
	int ret = 0;
	
	char *errmsg = NULL;
	int empty = 1;
	char sql[128];
	char *pSql;

	pSql = sql;

	strcpy(pSql, "select * from ");
	strcat(pSql, ptable);
	strcat(pSql, ";");

	printf("%s\n", pSql);

	ret = sqlite3_exec(db, pSql, rscallback, &empty, &errmsg);
	if (ret != SQLITE_OK) {
		printf("sqlite db error: ret=%d, errmsg: %s\n", ret, errmsg);
		return -1;
	}

	if (empty)
		printf("%s table is empty!\n", ptable);

	return 0;


}

int main(int argc, char *argv[])
{
#if 0
	int i = 0;
	SOCKFD_SET(0, bitmap);
	SOCKFD_SET(1, bitmap);
	SOCKFD_SET(2, bitmap);
	SOCKFD_SET(3, bitmap);
	SOCKFD_SET(8, bitmap);
	SOCKFD_SET(31, bitmap);
	i = find_first_zero(bitmap, 32);
	printf("i: %d, longsize: %d memory:\n", i, sizeof(unsigned long));
	for (i = 0; i < 4; i++)
		printf("addr:%p  %lx ", ((unsigned long *)bitmap+i),*((unsigned long *)bitmap+i));
	printf("\n");
	printf("bitmap addr is : %p\n", bitmap);
	for (i = 0; i < 4; i++) {
		printf("addr: %p %02x ", (unsigned char *)bitmap+i, *((unsigned char *)bitmap+i));
	}
	printf("\n");
	for (i = 0; i < 32; i++) {
		printf("%02x ", bitmap[i]);
	}
	printf("\n");
	return 0;
#endif

#if 0	
	int log_shmid; /* ipc shared memory id */
	char *buff = NULL;
	int i = 0;
	
	log_shmid = shmget(key, 1024, IPC_CREAT| 0666);
	if (log_shmid == -1)
		printf("create share memory error\n");


	buff = shmat(log_shmid, NULL, 0);
	if (buff == (void *)-1)
		printf("get shared memory failed\n");	

	printf("buff: %p\n", buff);
	for (i = 0; i < 10 ; i++)
		buff[i] = 0x55;

	while (1) {
		sleep(1);
	}
#else 
#if 0
	int log_shmid; /* ipc shared memory id */
	char *buff = NULL;
	int i = 0;
	
	log_shmid = shmget(key, 0,0);
	if (log_shmid == -1)
		printf("create share memory error\n");


	buff = shmat(log_shmid, NULL, 0);
	if (buff == (void *)-1)
		printf("get shared memory failed\n");	

	for (i = 0; i < 10 ; i++)
		printf("%x ", buff[i]);
#endif
	sqlite3 *db;
	int ret = 0;
	char *pTable = NULL;

	if (argc < 2) {
		printf("useage: dump_sqlite TABLENAME: \n");
		printf("TABLENAME: \n");
		printf("event_log\nconfig_log\nsecurity_log\napp_log\nloadblance_log\nnat_log");
		return 0;
	}

	

	ret = sqlite3_open("event_log.db", &db);
	if (ret != SQLITE_OK) {
		printf("open sqlite db error\n");
	}

	pTable = argv[1];
	db_show(db, pTable);


#endif
}
