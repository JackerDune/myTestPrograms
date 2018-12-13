#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "main.h"

struct test {
	int ione;
	int itwo;
	int ithree;
	char ipaddr[20];
};

CHECK_XML_INT_INIT(test, itwo, struct test, 0, 10)
CHECK_XML_INT_INIT(test, ione, struct test, 0, 10)
CHECK_XML_INT_INIT(test, ithree, struct test, 0, 50)
CHECK_XML_IP(test, ipaddr, struct test)
struct check_range_node_parameter testrange[] = 
{	
	HASH_RANGE_NODE_PARAM_ADD(test, ione, TYPE_XML_INT),
	HASH_RANGE_NODE_PARAM_ADD(test, itwo, TYPE_XML_INT),
	HASH_RANGE_NODE_PARAM_ADD(test, ithree, TYPE_XML_INT),
	HASH_RANGE_NODE_PARAM_ADD(test, ipaddr, TYPE_XML_INT),
	{"", NULL,TYPE_XML_INT}
};

static inline int xml_test_check_range(void *ptr)
{
	int i = 0;
	int ret = 0;

	for (i = 0; i< sizeof(testrange)/sizeof(struct check_range_node_parameter); i++) {
		if (testrange[i].check_func != NULL) {
			ret = testrange[i].check_func((void *) ptr);
			if (ret != 0) {
				printf("ret is : %d, name is %s\n", ret, testrange[i].name);
				return ret;
			}
		}
	}	

	return ret;
}
#define NIPQUAD(addr)	\
	((unsigned char *)&addr)[0],	\
	((unsigned char *)&addr)[1],	\
	((unsigned char *)&addr)[2],	\
	((unsigned char *)&addr)[3]
#define NIPQUAD_FMT "%u.%u.%u.%u"

#define STRRCHR(ptr, s, c) \
	if ( (ptr = (char *)strrchr(s, c)) == NULL) \
	{	return -1; } 

#define STRTOL(A, ptr) \
	if ( *ptr == '\0' ) { \
		return -1; \
	} \
	else { \
		A = strtol(ptr, (char **)NULL, 10); \
	};
#define STRCKE(ptr) \
       if ((*ptr == '0') && (*(ptr+1) != '\0')) \
         {return -1;}  


inline int ip_address_common_check(char *ip)
{
	char *ptr, buf[20];
	int A, B, C, D, E;
	char *last;
	
	if ((ip[0] < 48) || (ip[0] > 57))
           return -1;
	strncpy(buf, ip, 20);
	if ( buf != NULL ){
		STRRCHR(ptr, buf, '.');
		*ptr = '\0';
		++ptr;
       		STRCKE(ptr);
		STRTOL(D, ptr);
		STRRCHR(ptr, buf, '.');
		*ptr = '\0';
		++ptr;
                STRCKE(ptr);
		STRTOL(C, ptr);
		STRRCHR(ptr, buf, '.');
		*ptr = '\0';
		++ptr;
                STRCKE(ptr);
		STRTOL(B, ptr);
	
		ptr = (char *)strrchr(buf, '.');//IF 'X.X.X.X.X' 
		if (ptr) {
			printf("%s, %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		STRTOL(A, buf);
		ptr = buf;
        STRCKE(ptr);

		if ((A > 255) || (C > 255) || (B > 255) || (D > 255))
			return -1;
		else if ((A == 0) || (D == 0))
			return -1;
		else return 1;
	}
	return -1;
}

int main(int argc, char **argv) {
	struct test a;
	a.ione = 10;
	a.itwo = 1;
	a.ithree = 5;	
	int ret = 0;
	int aa,bb,cc,dd;

	strcpy(a.ipaddr, "1.1.1.1.1");
	ret = xml_test_check_range((void *)&a);
	printf("ret: %d\n", ret);
	strcpy(a.ipaddr, "1.0.0.1");
	ret = xml_test_check_range((void *)&a);
	printf("ret: %d\n", ret);

	sscanf("111.3.123.243", "%d.%d.%d.%d", &aa, &bb, &cc, &dd);
	printf("%d-%d-%d-%d\n", aa, bb, cc, dd);

	return 0;
}
