#include <stdio.h>
#include <string.h>
#include "main.h"

struct test {
	int ione;
	int itwo;
	int ithree;
};

CHECK_XML_INT_INIT(test, itwo, struct test, 0, 10)
CHECK_XML_INT_INIT(test, ione, struct test, 0, 10)
CHECK_XML_INT_INIT(test, ithree, struct test, 0, 50)

struct check_range_node_parameter testrange[] =
{
	HASH_RANGE_NODE_PARAM_ADD(test, ione, TYPE_XML_INT),
	HASH_RANGE_NODE_PARAM_ADD(test, itwo, TYPE_XML_INT),
	HASH_RANGE_NODE_PARAM_ADD(test, ithree, TYPE_XML_INT),
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

int main(int argc, char **argv) {
	struct test a;

	a.ione = 10;
	a.itwo = 1;
	a.ithree = 5;	
	xml_test_check_range((void *)&a);
	return 0;
}
