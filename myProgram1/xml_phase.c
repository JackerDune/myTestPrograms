#include <stdio.h>
#include <memory.h>

#include "libxml/xmlmemory.h"
#include "libxml/parser.h"

unsigned short crc16(unsigned short iv, unsigned long p)
{
	int i,j;
	unsigned int b;
	unsigned short poly = 0x1021;

	for (i=7; i>=0; i--) {
		b = (p >> (i*8)) & 0xFF;
		for (j = 7; j>=0; j--) {
			iv = ((iv << 1) ^ ((((iv >> 15)&1) ^ ((b >> j)&1)) ? poly : 0)) & 0xFFFF;
		}
	}

	return iv;
}


unsigned short hw_ipv4_hash(unsigned int Sip, unsigned int Dip, unsigned short Sport, unsigned short Dport, unsigned char proto, int noportflag)
{
	unsigned short src_crc = 0xFFFF;
	unsigned short dst_crc = 0xFFFF;
	unsigned short prot_crc = 0xFFFF;
	unsigned short result = 0;

	printf("sip: 0x%x, dip: 0x%x, sp: %d, dp: %d\n", Sip, Dip, Sport, Dport);
	src_crc = crc16(src_crc, ((unsigned long)Sip << 32));
	dst_crc = crc16(dst_crc, ((unsigned long)Dip << 32));

	if (noportflag) {
		src_crc = crc16(src_crc, ((unsigned long)Sport << 48));
		dst_crc = crc16(dst_crc, ((unsigned long)Dport << 48));
	}

#if 0
	prot_crc = crc16(prot_crc, ((unsigned long)proto << 56));
	
	result = (src_crc ^ dst_crc ^ prot_crc) & 0xFFFF;
#else
	result = (src_crc ^ dst_crc ^ prot_crc) & 0xFFFF;
#endif	
	result = (result & 0xFF) ^ (result >> 8);
	return result;
}


unsigned short hw_ipv6_hash(unsigned int *Sip, unsigned int *Dip, unsigned short Sport, unsigned short Dport, unsigned char proto, int noportflag)
{
	unsigned short src_crc = 0xFFFF;
	unsigned short dst_crc = 0xFFFF;
	unsigned short prot_crc = 0xFFFF;
	unsigned short result = 0;
//	unsigned long tmps[2] = {0x2009000000000000, 0x9};
//	unsigned long tmpd[2] = {0xff02000000000000, 0x1ff000001};

	printf("sip: 0x%lx, 0x%lx, dip:0x%lx 0x%lx\n", *((unsigned long*)&Sip[0]), *((unsigned long*)(&Sip[2])), *((unsigned long*)&Dip[0]), *((unsigned long*)(&Dip[2])));
	src_crc = crc16(src_crc, *((unsigned long*)&Sip[0]));
	src_crc = crc16(src_crc, *((unsigned long*)&Sip[2]));
	dst_crc = crc16(dst_crc, *((unsigned long*)&Dip[0]));
	dst_crc = crc16(dst_crc, *((unsigned long*)&Dip[2]));

	if (noportflag) {
		src_crc = crc16(src_crc, ((unsigned long)Sport << 48));
		dst_crc = crc16(dst_crc, ((unsigned long)Dport << 48));
	}

	result = (src_crc ^ dst_crc ^ prot_crc) & 0xFFFF;
	
	return result;
}


unsigned short hw_ipv4_hash_proto(unsigned int Sip, unsigned int Dip, unsigned short Sport, unsigned short Dport, unsigned char proto, int noportflag)
{
	unsigned short src_crc = 0xFFFF;
	unsigned short dst_crc = 0xFFFF;
	unsigned short prot_crc = 0xFFFF;
	unsigned short result = 0;

	src_crc = crc16(src_crc, ((unsigned long)Sip << 32));
	dst_crc = crc16(dst_crc, ((unsigned long)Dip << 32));

	if (noportflag) {
		src_crc = crc16(src_crc, ((unsigned long)Sport << 48));
		dst_crc = crc16(dst_crc, ((unsigned long)Dport << 48));
	}

#if 1
	prot_crc = crc16(prot_crc, ((unsigned long)proto << 56));
	
	result = (src_crc ^ dst_crc ^ prot_crc) & 0xFFFF;
#else
	result = (src_crc ^ dst_crc) & 0xFFFF;
#endif	
	return result;
}

#define output_func(x) \
	printf("hash: 0x%x(%d), coreid: %d\n", x, x, coreid);



unsigned short disp_crc16(unsigned short iv, unsigned long p)
{
	int i,j;
	unsigned int b;
	unsigned short poly = 0x1021;

	for (i=7; i>=0; i--) {
		b = (p >> (i*8)) & 0xFF;
		for (j = 7; j>=0; j--) {
			iv = ((iv << 1) ^ ((((iv >> 15)&1) ^ ((b >> j)&1)) ? poly : 0)) & 0xFFFF;
		}
	}

	return iv;
}

typedef enum
{
    DISP_HASH_ADDR_IPV4,
    DISP_HASH_ADDR_IPV6    
}DISP_HASH_ADDR_TYPE;


unsigned short __disp_hardware_hash_function_tuple_five(unsigned int *sip, unsigned int *dip, unsigned short sp, unsigned short dp, DISP_HASH_ADDR_TYPE type)
{
	unsigned short src_crc = 0xFFFF;
	unsigned short dst_crc = 0xFFFF;
	unsigned short prot_crc = 0xFFFF;
	unsigned short hash = 0;

    printf("%s, %d\n", __FUNCTION__, __LINE__);
    if (type == DISP_HASH_ADDR_IPV4) {
		src_crc = disp_crc16(src_crc, ((unsigned long)sip[0] << 32));
		dst_crc = disp_crc16(dst_crc, ((unsigned long)dip[0] << 32));
	}
	else {
		src_crc = disp_crc16(src_crc, *((unsigned long *)&sip[0]));
		src_crc = disp_crc16(src_crc, *((unsigned long *)&sip[2]));
		dst_crc = disp_crc16(dst_crc, *((unsigned long *)&dip[0]));
		dst_crc = disp_crc16(dst_crc, *((unsigned long *)&dip[2]));
	}
	
	src_crc = disp_crc16(src_crc, ((unsigned long)sp << 48));
	dst_crc = disp_crc16(dst_crc, ((unsigned long)dp << 48));
	hash = (src_crc ^ dst_crc ^ prot_crc) & 0xFFFF;
	return hash;
}


struct core_weight_edge
{
	int down;
	int up;
};

int core_weight_total = 10;
int core_weight[2] = {4,6};
struct core_weight_edge core_edge[2] = {{0}};

void disp_calc_weight_edge()
{
	int i = 0;
	for (i = 0; i < 2; i++) {
		if (i == 0)
			core_edge[i].down = 0;
		else
			core_edge[i].down = core_edge[i - 1].up;

		if (i == 3)
			core_edge[i].up = core_weight_total;
		else
			core_edge[i].up = core_edge[i].down + core_weight[i];	
	}
}
int disp_get_coreid_by_hash(unsigned short hash)
{
    int temp = 0;
    int i = 0;

    temp = hash % core_weight_total;
    for (i = 0; i < 2; i++) {
        if ((temp < core_edge[i].up) && (temp >= core_edge[i].down))
            return i;
    }

    return 0; //return core 0 when error happens
}


static int32_t open_xml_file(xmlDocPtr *doc, char *xml_file_path)
{
	*doc = xmlParseFile((const char *)xml_file_path);

	if (NULL == *doc) {
		return -1;
	} else {
		return 0;
	}
}

static int32_t move_to_dst_node(xmlDocPtr *doc, xmlNodePtr *cur, char *dst)
{
	xmlNodePtr node = NULL;
	node = xmlDocGetRootElement(*doc);

	if (NULL == node) {
		goto err;
	}

	if (xmlStrcmp(node->name, (const xmlChar *) "root")) {
		goto err;
	}

	node = node->xmlChildrenNode;
	while (NULL != node) {
		if ((!xmlStrcmp(node->name, (const xmlChar *)dst))) {
			*cur = node->xmlChildrenNode; /* skip to item */
			goto success; /* found at least one item */
		}
		node = node->next;
	}
success:
	return 0;
err:
	return -1;

}

static xmlChar* get_content_by_name(xmlNodePtr *cur, char *name)
{
	xmlNodePtr node = *cur;
	xmlChar *value = NULL;

	while (NULL != node) {
		if ((!xmlStrcmp(node->name, (const xmlChar *)name))) {
			value = xmlNodeGetContent(node);
			goto out;
		}
		node = node->next;
	}
out:
	return value;

}

#if 0
static int get_child_node_by_name(xmlNodePtr *cur, xmlNodePtr *child, char *name)
{
	xmlNodePtr node = *cur;

	while (NULL != node) {
		if ((!xmlStrcmp(node->name, (const xmlChar *)name))) {
			*child = node->xmlChildrenNode;
			return 0;
		}
		node = node->next;
	}

	return -1;
}
static xmlChar* get_child_node_value_by_name(xmlNodePtr *cur, char *name)
{
	xmlNodePtr child;
	xmlNodePtr node = *cur;
	xmlChar *value = NULL;

	if ((!xmlStrcmp(node->name, (const xmlChar *)"item"))) {
		goto out;
	}

	if (!get_child_node_by_name(&node, &child, "item")) {
		value = get_content_by_name(&child, name);
	}

out:
	return value;

}
static int32_t free_xml_item(int argc, ...)
{
	va_list ap;
	xmlChar *tmp = NULL;
	int i = 0;

	va_start(ap, argc);
	for (i = 0; i < argc; i++) {
		tmp = va_arg(ap, xmlChar *);
		if (tmp) {
			xmlFree(tmp);
		}
	}
	va_end(ap);

	return 0;
}

#endif
int main(int argc, char **argv)
{
#if 0
	unsigned short hash = 0;
	int ipv6_saddr[4] = {0};
	int ipv6_daddr[4] = {0};
    int sip = 0x01010101;
    int dip = 0x02020202;
    int coreid  = 0;

	unsigned long tmps[2] = {0x2009000000000000, 0x9};
	unsigned long tmpd[2] = {0xff02000000000000, 0x1ff000001};
	
	hash = hw_ipv4_hash(0x01010102, 0x72727272, 0, 0, 1, 0);
	output_func(hash);
	hash = hw_ipv4_hash(0x01010102, 0xc0a800f4, 0, 0, 1, 0);
	output_func(hash);
	hash = hw_ipv4_hash(0x01010102, 0xDCB59C22, 0xc9f5, 0x35, 17, 1);
	output_func(hash);

	hash = hw_ipv4_hash(0x7C0E1512, 0xAC10009F, 0x50, 0x45B0, 6, 1);
	output_func(hash);
	
	hash = hw_ipv4_hash(0x01010101, 0x02020202, 0x1, 0x1, 1, 1);
	output_func(hash);


	hash = __disp_hardware_hash_function_tuple_five(&sip, &dip, 1, 1, DISP_HASH_ADDR_IPV4);
	output_func(hash);
    
	hash = hw_ipv4_hash_proto(0xAC113CC7, 0xAC10009E, 0x303, 0xc0e1, 1, 0); 
	output_func(hash);

	ipv6_saddr[0] = 0x20090000;
	ipv6_saddr[1] = 0x0;
	ipv6_saddr[3] = 0x9;
	ipv6_daddr[0] = 0xFF020000;
	ipv6_daddr[2] = 0x1;
	ipv6_daddr[3] = 0xFF000001;
	memcpy(ipv6_saddr, tmps, 16);
	memcpy(ipv6_daddr, tmpd, 16);
	hash = hw_ipv6_hash(ipv6_saddr, ipv6_daddr, 0x7, 0x8, 58, 0);
	output_func(hash);
	
	printf("long size is : %lu, value: 0x%lx, sip: 0x%lx, 0x%lx\n", sizeof(unsigned long), 0x1000000000000000, *(((unsigned long *)&ipv6_saddr[0])), *(((unsigned long *)&ipv6_saddr[2])));

	int i = 0;
	disp_calc_weight_edge();
	sip = 0x01010102;
	dip = 0x02020202;
	for (i = 0; i < 20; i++) {
		hash = hw_ipv4_hash(sip, dip, 7, 7, 0, 1);
		coreid = disp_get_coreid_by_hash(hash);
		output_func(hash);		
		sip += 1;
	}
#else
#if 0
	int ret = 0;
	xmlDocPtr doc;
	xmlNodePtr node;
	xmlChar *value;

	ret = open_xml_file(&doc, "app_main.xml");
	if (ret == -1) {
		printf("open xml file error!\n");
		return 0;
	}

	ret = move_to_dst_node(&doc, &node, "app_para_conf");	
	if (ret == -1) {
		printf("%s, %d\n", __FUNCTION__, __LINE__);
		return -1;
	}

	value = get_content_by_name(&node, "DplaneStartCoreId");
	if (value != NULL)
		printf("%s, %d, StartDpCoreId is : %d\n", __FUNCTION__, __LINE__, atoi(value));


	free_xml_item(1, value);
	xmlFreeDoc(doc);
	xmlCleanupParser();
#endif	
	while (1) {
		;
	}
#endif
	return 0;
}
