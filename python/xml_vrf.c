#include "xml_common.h"
#include "xml_zebra.h"
#include "memory.h"
#include "../zebra/zebra_vty.h"
#include "if_operate.h"
#include "../lib/if_operate.h"
#include "global_conf.h"
#include "linux/tbk_local_access.h"
#include "if_msg.h"
#include <device_stat_save.h>
#include <netns_msg.h>
#include "aaa_msg.h"

#include "../aaa/aaa_vty.h"

#include "xml_vsys.h"
#include "net/fw_objects.h"
#include "sunya_module.h"

#define VRF_SAVE_TMP "/tmp/.xml_vrf"
#define VRF_SPEC_TMP "/tmp/.xml_vsys"
#define VRF_VSYSIF_TMP "/tmp/.xml_vsysif"
#define VRF_VSYSSWITCH_TMP "/tmp/.xml_vsysswitch"

extern int nl_send_zone (struct zone_trans *if_grp, enum nl_op_type type);
extern int tb_vs_nat_used_check(char *ifname);
extern int tb_ha_sysmon_used_check(char * ifname);
extern int dhcp_vrf_del_proc(int vrfid);

/**************用于解析XML数据结构**********************/
struct xml_ifname
{
	char ifname[XML_MAX_NAME_LEN];
	struct xml_ifname *next;
};
struct xml_vrf
{
	char vrf_name[XML_MAX_NAME_LEN];
	char desc[XML_MAX_DESC_LEN];
	int vrf_id;
	struct xml_ifname *vrf_ifname_items;
};
/****************************************************************/


static struct  xml_vrf vrf_cfg;


/*****************解析接口名的子结构*******************/

PARSE_XML_STRING(vrf_ifname_items, ifname, struct xml_ifname, XML_MAX_NAME_LEN)

struct element_node_parameter vrf_ifname_element_parameter[] =
{
	HASH_NODE_PARAM_ADD(vrf_ifname_items, ifname, TYPE_XML_STRING),

	{"", NULL, NULL, NULL, TYPE_XML_INT}
};
/***************************************************************/

/*******************解析组名的父结构******************/
PARSE_XML_STRING(vrf, vrf_name, struct xml_vrf, XML_MAX_NAME_LEN)
PARSE_XML_STRING(vrf, desc, struct xml_vrf, XML_MAX_DESC_LEN)

PARSE_XML_INT(vrf, vrf_id, struct xml_vrf)
PARSE_XML_STRUCT(vrf, vrf_ifname_items, struct xml_vrf, struct xml_ifname)


struct element_node_parameter vrf_element_parameter[] =
{
	HASH_NODE_PARAM_ADD(vrf, vrf_name, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(vrf, desc, TYPE_XML_STRING),	
	HASH_NODE_PARAM_ADD(vrf, vrf_id, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(vrf, vrf_ifname_items, TYPE_XML_STRUCT),

	{"", NULL, NULL, NULL, TYPE_XML_INT}
};
/***************************************************************/

//vsys specification 结构体
struct xml_vsys_spec
{
    char vsys_name[XML_MAX_NAME_LEN];

	/* userobj 对象规格和当前计数 */
	int userobj_max;
	/* usergroup 对象规格和当前计数 */
	int usergrp_max;
	/* keyword 对象规格和当前计数 */
	int keyword_max;
	/* appgroup 对象规格和当前计数 */
	int appgrp_max;

	/* service 对象规格和当前计数 */
	int sevobj_max;
	/* absolute /periodic time对象规格和当前计数 */
	int trobj_max;
	/* radius server认证规格和当前计数 */
	int radius_sev_max;
	/* ldap server认证规格和当前计数 */
	int ldap_sev_max;
	/* fire_wall policy规格和当前计数 */
	int fw_plcy_max;
	/* fire_wall policy v6规格和当前计数 */
	int fw_plcy_v6_max;
	/* flog log policy规格和当前计数 */
	int floglog_max;
	/* address object规格和当前计数 */
	int address_max;
	/* address group规格和当前计数 */
	int addressgrp_max;
	/* address pool 规格和当前计数*/
	int addr_pool_max;
	/* 流总数  规格 */
    int flow_max;
    /* nat rule  规格 */
    int nat_rule_max;
    /* security zone 规格 */
    int security_zone_max;
    /* 用户认证策略 规格 */
    int uplcy_max;
    /*arp ip-mac 规格*/
    int ipmac_max;

    int vsysid;
};

static struct  xml_vsys_spec  vsys_spec_data;

PARSE_XML_STRING(vsys_spec, vsys_name, struct xml_vsys_spec, XML_MAX_NAME_LEN)
PARSE_XML_INT(vsys_spec, userobj_max, struct xml_vsys_spec)
PARSE_XML_INT(vsys_spec, usergrp_max, struct xml_vsys_spec)
PARSE_XML_INT(vsys_spec, keyword_max, struct xml_vsys_spec)
PARSE_XML_INT(vsys_spec, appgrp_max, struct xml_vsys_spec)
PARSE_XML_INT(vsys_spec, sevobj_max, struct xml_vsys_spec)
PARSE_XML_INT(vsys_spec, trobj_max, struct xml_vsys_spec)
PARSE_XML_INT(vsys_spec, radius_sev_max, struct xml_vsys_spec)
PARSE_XML_INT(vsys_spec, ldap_sev_max, struct xml_vsys_spec)
PARSE_XML_INT(vsys_spec, fw_plcy_max, struct xml_vsys_spec)
PARSE_XML_INT(vsys_spec, fw_plcy_v6_max, struct xml_vsys_spec)
PARSE_XML_INT(vsys_spec, floglog_max, struct xml_vsys_spec)
PARSE_XML_INT(vsys_spec, address_max, struct xml_vsys_spec)
PARSE_XML_INT(vsys_spec, addressgrp_max, struct xml_vsys_spec)
PARSE_XML_INT(vsys_spec, addr_pool_max, struct xml_vsys_spec)
PARSE_XML_INT(vsys_spec, flow_max, struct xml_vsys_spec)
PARSE_XML_INT(vsys_spec, nat_rule_max, struct xml_vsys_spec)
PARSE_XML_INT(vsys_spec, security_zone_max, struct xml_vsys_spec)
PARSE_XML_INT(vsys_spec, uplcy_max, struct xml_vsys_spec)
PARSE_XML_INT(vsys_spec, ipmac_max, struct xml_vsys_spec)

struct element_node_parameter vsys_spec_element_parameter[] =
{
    HASH_NODE_PARAM_ADD(vsys_spec, vsys_name, TYPE_XML_STRING),
    HASH_NODE_PARAM_ADD(vsys_spec, userobj_max, TYPE_XML_INT),
    HASH_NODE_PARAM_ADD(vsys_spec, usergrp_max, TYPE_XML_INT),
    HASH_NODE_PARAM_ADD(vsys_spec, keyword_max, TYPE_XML_INT),
    HASH_NODE_PARAM_ADD(vsys_spec, appgrp_max, TYPE_XML_INT),
    HASH_NODE_PARAM_ADD(vsys_spec, sevobj_max, TYPE_XML_INT),
    HASH_NODE_PARAM_ADD(vsys_spec, trobj_max, TYPE_XML_INT),
    HASH_NODE_PARAM_ADD(vsys_spec, radius_sev_max, TYPE_XML_INT),
    HASH_NODE_PARAM_ADD(vsys_spec, ldap_sev_max, TYPE_XML_INT),
    HASH_NODE_PARAM_ADD(vsys_spec, fw_plcy_max, TYPE_XML_INT),
    HASH_NODE_PARAM_ADD(vsys_spec, fw_plcy_v6_max, TYPE_XML_INT),
    HASH_NODE_PARAM_ADD(vsys_spec, floglog_max, TYPE_XML_INT),
    HASH_NODE_PARAM_ADD(vsys_spec, address_max, TYPE_XML_INT),
    HASH_NODE_PARAM_ADD(vsys_spec, addressgrp_max, TYPE_XML_INT),
    HASH_NODE_PARAM_ADD(vsys_spec, addr_pool_max, TYPE_XML_INT),
    HASH_NODE_PARAM_ADD(vsys_spec, flow_max, TYPE_XML_INT),
    HASH_NODE_PARAM_ADD(vsys_spec, nat_rule_max, TYPE_XML_INT),
    HASH_NODE_PARAM_ADD(vsys_spec, security_zone_max, TYPE_XML_INT),
    HASH_NODE_PARAM_ADD(vsys_spec, uplcy_max, TYPE_XML_INT),
    HASH_NODE_PARAM_ADD(vsys_spec, ipmac_max, TYPE_XML_INT),
    {"", NULL, NULL, NULL, TYPE_XML_INT}
};

int xml_vsys_spec_get_info=0;

struct element_node_parameter vsys_spec_get_info_element_parameter[] =
{
    {"", NULL, NULL, NULL, TYPE_XML_INT}
};

struct xml_vsys_switch_ui{
    int vsysid;
    char vsys_name[XML_MAX_NAME_LEN];
};

static struct xml_vsys_switch_ui vsys_switch_data;

PARSE_XML_INT(vsys_switch_ui, vsysid, struct xml_vsys_switch_ui)
PARSE_XML_STRING(vsys_switch_ui, vsys_name, struct xml_vsys_switch_ui, XML_MAX_NAME_LEN)

struct element_node_parameter vsys_switch_element_parameter[] =
{
    HASH_NODE_PARAM_ADD(vsys_switch_ui, vsysid, TYPE_XML_INT),
    HASH_NODE_PARAM_ADD(vsys_switch_ui, vsys_name, TYPE_XML_STRING),
    {"", NULL, NULL, NULL, TYPE_XML_INT}
};

struct xml_vsys_ifname{
};

static struct xml_vsys_ifname vsys_ifname_data;

struct element_node_parameter vsys_ifname_element_parameter[] =
{
    {"", NULL, NULL, NULL, TYPE_XML_INT}
};

extern int no_if_dhcpd_enable_cmd(void *ptr);
extern int if_address_config_clear(char *ifname);
extern int if_ipv6_address_config_clear(char *ifname);


//check interface is used by all policy
int tb_if_used_by_all_policy_check(char *name)
{
	int ret=0;

	ret = tb_vs_nat_used_check(name);
	if(ret) return ret;

	/* for ha sysmon interface */
	ret = tb_ha_sysmon_used_check(name);
	if(ret) return ret;

	return ret;

}

static int guish_netns_check(char *ifname)
{
	int ret = 0;
	
	if( !ifname || ifname[0]=='\0' || ifname[0]=='\n'){
		return 857;
	}

	if(strncmp(ifname,"ge",2) && strncmp(ifname,"xge",3) 
		&& strncmp(ifname,"vlan",4) && strncmp(ifname,"tunl",4)){
		//&& strncmp(ifname,"tvi",3) && strncmp(ifname,"tunl",4)){ //该版本只支持 物理口 vlan口加入vsys
		return 856;
	}

	if(interface_find_proc(ifname) <= 0) {
		return 858;
	}
#if 0		
	/* 存在子接口时,主接口不允许加入VRF */
	if(interface_sub_find_proc(ifname)){
		return 859;
	}
#endif
	ret = if_can_use_by_vrf(ifname);
	if( ret )
		return ret;

}

static int guish_interface_used_check(char *ifname)

{
    int ret;
	ret = tb_if_used_by_all_policy_check(ifname);
    if(ret)
        return ret;

	return 0;
}

int guish_vsys_routetable_set(int opt, u_int32_t id, char *name)
{
	struct {
		struct oam_data_st oh;
		struct zebra_routetable_st data;
	} cmd;

	memset(&cmd,0,sizeof cmd);
	cmd.oh.cmd_code = ZEBRA_SET_ROUTETABLE;
	cmd.oh.mod_id = VTYSH_INDEX_ZEBRA;
	switch(opt) {
	case 0:
		cmd.data.cmd = 0;	/*delete route table*/
		break;
	case 1:
		cmd.data.cmd = 1;	/*add route table*/
		break;
	case 2:
		cmd.data.cmd = 2;	/*modify route table name*/
		break;
	}

	cmd.data.vrf_id = id;
	strncpy(cmd.data.vrf_name,name,strlen(name));

	write(client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd,sizeof cmd);

	while(1) {
		int nbytes;

		nbytes = read(client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd, sizeof(cmd)-1);
		if( nbytes <= 0 && errno != EINTR )
			return 0;

		if( nbytes > 0 ) {
			if( cmd.oh.cmd_code != 0) 
				return cmd.oh.cmd_code;
			break;
		}
	}
	
	return 0;
}

#ifdef CONFIG_VSYS
int guish_vsys_interface_set(int opt, u_int32_t id, char *ifname)
{
	struct {
		struct oam_data_st oh;
		struct zebra_vsysif_st data;
	} cmd;

	memset(&cmd,0,sizeof cmd);
	cmd.oh.cmd_code = ZEBRA_SET_VSYSIF;
	cmd.oh.mod_id = VTYSH_INDEX_ZEBRA;
	if( opt )
		cmd.data.cmd = 1;	/*add interface to VSYS*/
	else
		cmd.data.cmd = 0;	/*delete interface from vsys*/
	cmd.data.vrf_id = id;
	strcpy(cmd.data.ifname,ifname);

	write(client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd, sizeof cmd);

	while(1) {
		int nbytes;

		nbytes = read(client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd, sizeof(cmd)-1);
		if( nbytes<=0 && errno!= EINTR) 
			return 0;
		
		if( nbytes > 0 ) {
			if( cmd.oh.cmd_code != 0 )
				return cmd.oh.cmd_code;
			break;
		}
	}
	
	return 0;
}
#endif


int guish_netns_add_if(int vrf_id, char *ifname)
{
	int ret = 0;

	ret = guish_netns_check(ifname);
	if (ret)
		return ret;
			
	/*clear if's IP/IPv6 address*/
	if_address_config_clear(ifname);
	if_ipv6_address_config_clear(ifname);
	/*disable dhcp server*/
	no_if_dhcpd_enable_cmd(ifname);

	ret = dp_netns_add_if(vrf_id, ifname);
	if (ret)
		return ret;

	return guish_vsys_interface_set(1, vrf_id, ifname);
}

int guish_netns_del_if(int vrf_id, char *ifname)
{
	int ret;
	
	ret = guish_netns_check(ifname);
	if (ret)
		return ret;

	/*clear if's IP address*/
	if_address_config_clear(ifname);
	if_ipv6_address_config_clear(ifname);
	/*disable dhcp server*/
	no_if_dhcpd_enable_cmd(ifname);

	ret = dp_netns_del_if(vrf_id, ifname);
	if (ret)
		return ret;

	return guish_vsys_interface_set(0, vrf_id, ifname);
}
static int alloc_interface_to_vrf(char *vrf_name, char *ifname)
{
	int vrf_id = 0;
	char vrfname[MAX_NETNS_NAME_LEN];
	int ret = 0;

	vrf_id = dp_netns_get_vrfid_by_name(vrf_name);
	if(NETNS_ID_VAILD(vrf_id)) {
		memset(vrfname, 0, MAX_NETNS_NAME_LEN);
		ret = dp_netns_get_vrfname_by_ifname(ifname, vrfname);
		if (ret)
			return guish_netns_add_if(vrf_id, ifname);
		else 
			return 860;
	}		
	else 
		return 861;

	return 0;
}

static int no_alloc_interface_to_vrf(char *vrf_name, char *ifname)
{
	int vrf_id = 0;
	
	vrf_id = dp_netns_get_vrfid_by_name(vrf_name);
	if(NETNS_ID_VAILD(vrf_id)){
		return guish_netns_del_if(vrf_id, ifname);
	}
	else
		return 861;

	return 0;
}
int xml_get_zone_by_ifname(char *ifname)
{
	struct zone_trans zone;
	int ret;

	memset(&zone, 0, sizeof(struct zone_trans));
	strcpy(zone.ifname, ifname);//strncpy(zone.ifname, ifname, MAX_IFNAME_LEN);

	ret = nl_send_zone(&zone, NL_GET_ZONE);
	return ret;
}

int xml_if_reset_to_vrf0(char *ifname)
{
	int ret = 0;
	char vrfname[MAX_NETNS_NAME_LEN];

	memset(vrfname, 0, MAX_NETNS_NAME_LEN);
	ret = dp_netns_get_vrfname_by_ifname(ifname, vrfname);
	if (ret || !vrfname[0])
		return 0;

	ret = no_alloc_interface_to_vrf(vrfname, ifname);
	return ret;
}

/*页面修改接口时当某一个接口新增失败或删除失败，都继续执行*/
static int xml_vrf_del_ifname_from_list(struct  xml_vrf *vrf_info)
{	
	int rc, retval;
	int i, outlen;
	int del_flag;
	struct netns_set_msg *req;
	struct netns_msg_reply *reply;
	char real_name[MAX_NETNS_NAME_LEN];

	struct xml_ifname *ifgroup = NULL;

	req = get_prealloc_buf();
	memset(req, 0, sizeof(*req));
	req->msg.opt = IPCMSG_DP_NETNS_SHOWONE;
	strncpy(req->data.name, vrf_info->vrf_name, sizeof(vrf_info->vrf_name));
	req->data.pos = 0;
	
	do {
		reply = get_prealloc_outbuf();
		outlen = sizeof(*reply);
		rc = ipc_send_and_recv(APP_DPLANE, IPCMSG_NETNS_NOTIFY,
					req, sizeof(*req), &retval, reply, &outlen);
		if (rc < 0 || retval < 0) {
			return -1;
		}
		
		for (i = 0; i < reply->count; i++) {
			struct netns_entry *entry = &reply->entry[i];
			ifgroup = vrf_info->vrf_ifname_items;
			del_flag = 1;
			while(ifgroup)
			{
				memset(real_name, 0, MAX_NETNS_NAME_LEN);
				if_get_name_by_alias(ifgroup->ifname, real_name);
				if (!strcmp(entry->ifname, (real_name[0]=='\0')?ifgroup->ifname:real_name)){
					del_flag = 0;
					break;
				}

				ifgroup = ifgroup->next;
			}
			if (del_flag){
				if(!(xml_get_zone_by_ifname(entry->ifname)))
				no_alloc_interface_to_vrf(vrf_info->vrf_name, entry->ifname);
			}
		}
		
		req->data.pos += reply->count;
	} while (reply->count > 0);

	return 0;
}

static int xml_vrf_add_ifname_to_list(struct  xml_vrf *vrf_info)
{	
	int rc, retval;
	int i, outlen;
	struct netns_set_msg *req;
	struct netns_msg_reply *reply;
	char real_name[MAX_NETNS_NAME_LEN];

	struct xml_ifname *ifgroup = NULL;

	req = get_prealloc_buf();	
	memset(req, 0, sizeof(*req));
	req->msg.opt = IPCMSG_DP_NETNS_SHOWONE;
	strncpy(req->data.name, vrf_info->vrf_name, sizeof(vrf_info->vrf_name));
	req->data.pos = 0;
	
	do {
		reply = get_prealloc_outbuf();
		outlen = sizeof(*reply);
		rc = ipc_send_and_recv(APP_DPLANE, IPCMSG_NETNS_NOTIFY,
					req, sizeof(*req), &retval, reply, &outlen);
		if (rc < 0 || retval < 0) {
			return -1;
		}
		
		for (i = 0; i < reply->count; i++) {
			struct netns_entry *entry = &reply->entry[i];
			ifgroup = vrf_info->vrf_ifname_items;
			while(ifgroup)
			{
				memset(real_name, 0, MAX_NETNS_NAME_LEN);
				if_get_name_by_alias(ifgroup->ifname, real_name);
				if (!strcmp(entry->ifname, (real_name[0]=='\0')?ifgroup->ifname:real_name)){
					ifgroup->ifname[0] = '\0';
					break;
				}

				ifgroup = ifgroup->next;
			}		
		}
		
		req->data.pos += reply->count;
	} while (reply->count > 0);

	ifgroup = vrf_info->vrf_ifname_items;
	while(ifgroup)
	{
		if (ifgroup->ifname[0] != '\0'){
			memset(real_name, 0, MAX_NETNS_NAME_LEN);
			if_get_name_by_alias(ifgroup->ifname, real_name);
			if(xml_get_zone_by_ifname((real_name[0]=='\0')?ifgroup->ifname:real_name))
			    return 108;
			alloc_interface_to_vrf(vrf_info->vrf_name, (real_name[0]=='\0')?ifgroup->ifname:real_name);
		}
		ifgroup = ifgroup->next;
	}

	return 0;
}

int vrf_xml_show_one_info(xmlNode *xml_node, char *vrf_name)	
{	
	int rc, retval;
	int i, outlen;
	struct netns_set_msg req;
	struct netns_msg_reply *reply;
	char desc[OBJ_DESC_LEN] = "\0";
	char alias_name[MAX_NETNS_NAME_LEN];
	xmlNode *parent, *child, *sub_child;

	parent = vsos_xmlNewChild(xml_node, NULL, BAD_CAST "group", BAD_CAST NULL);
	vsos_xmlNewChild(parent, NULL, BAD_CAST "vrf_name", BAD_CAST vrf_name);
	dp_netns_get_desc_by_name(vrf_name, desc);
	vsos_xmlNewChild(parent, NULL, BAD_CAST "desc", BAD_CAST desc);

	child = vsos_xmlNewChild(parent, NULL, BAD_CAST "vrf_ifname_items", BAD_CAST NULL);
	memset(&req, 0, sizeof(req));
	req.msg.opt = IPCMSG_DP_NETNS_SHOWONE;
	strncpy(req.data.name, vrf_name, strlen(vrf_name));
	req.data.pos = 0;
	
	do {
		reply = get_prealloc_outbuf();
		outlen = sizeof(*reply);
		rc = ipc_send_and_recv(APP_DPLANE, IPCMSG_NETNS_NOTIFY,
					&req, sizeof(req), &retval, reply, &outlen);
		if (rc < 0 || retval < 0) {
			return -1;
		}

		for (i = 0; i < reply->count; i++) {
			struct netns_entry *entry = &reply->entry[i];
			char buf[16];
			int sys_used = 0;
			/*当显示vrf0的接口信息时，去掉不可被vrf使用的接口 */
			/*if (!entry->id){
				if (guish_netns_check(entry->ifname))
				    continue;
            }*/

            //显示所有接口，不被vrf使用的接口显示灰色			
			if(strncmp(entry->ifname,"ge",2) 
				&& strncmp(entry->ifname,"xge",3) 
				&& strncmp(entry->ifname,"vlan",4)
				&& strncmp(entry->ifname,"tunl",4)){
				continue;
			}
			
			if (if_can_use_by_vrf(entry->ifname))
				continue;
			
            if(guish_interface_used_check(entry->ifname))
                sys_used = 1;

			sub_child = vsos_xmlNewChild(child, NULL,
				BAD_CAST "group", BAD_CAST NULL);

			memset(alias_name, 0, MAX_NETNS_NAME_LEN);
			if_get_alias_by_name(entry->ifname, alias_name);
			
			if(strlen(alias_name))
				vsos_xmlNewChild(sub_child, NULL, BAD_CAST "ifname", BAD_CAST alias_name);
			else									
				vsos_xmlNewChild(sub_child, NULL, BAD_CAST "ifname", BAD_CAST entry->ifname);

            memset(buf, 0, 16);
            snprintf(buf, 16, "%d", sys_used);
            vsos_xmlNewChild(sub_child, NULL, BAD_CAST "sys_used", BAD_CAST buf);

		}
		
		req.data.pos += reply->count;
	} while (reply->count == NETNS_SHOW_NUM);

	return 0;
}


/*增加报错信息*/
unsigned long xml_vrf_del(void *ptr)
{
	int rc;	
	int netns_id = 0;
	struct  xml_vrf *vrf_info = ptr;
	
	if (vrf_info->vrf_name[0] == '\0' || 
			vrf_info->vrf_name[0] == '\n' || 
				strlen(vrf_info->vrf_name) >= MAX_NETNS_NAME_LEN){
		return 862;
	}
	
	if (strncmp(vrf_info->vrf_name, NETNS_DEFAULT_NAME, sizeof(NETNS_DEFAULT_NAME)) == 0){
		return 862;
	}

	/* delete net namespace interface first. */
	netns_id = dp_netns_get_vrfid_by_name(vrf_info->vrf_name);
	if(netns_id && NETNS_ID_VAILD(netns_id)) {
		int retval;
		int outlen;
		struct netns_set_msg *req;
		struct netns_msg_reply *reply;
		int rc;
		int i;
		req = get_prealloc_buf();		
		memset(req, 0, sizeof(*req));
		req->msg.opt = IPCMSG_DP_NETNS_SHOWONE;
		strncpy(req->data.name, vrf_info->vrf_name, strlen(vrf_info->vrf_name));
		req->data.pos = 0;
		do {
			reply = get_prealloc_outbuf();
			outlen = sizeof(*reply);
			rc = ipc_send_and_recv(APP_DPLANE, IPCMSG_NETNS_NOTIFY,
						req, sizeof(*req), &retval, reply, &outlen);
			if (rc < 0 || retval < 0) {
				return 863;
			}
			
			for (i = 0; i < reply->count; i++) {
				struct netns_entry *entry = &reply->entry[i];
				guish_netns_del_if(netns_id, entry->ifname);
			}
			
			req->data.pos += reply->count;
		} while (reply->count > 0);
		
	}

	/* 删除netns对应的ID */
	netns_id = dp_netns_del(vrf_info->vrf_name);
	if (netns_id < 0)
		return 863;

	netns_del_ike_vrfid(netns_id);
	dhcp_vrf_del_proc(netns_id);

	/* 删除netns对应的route-table */
	guish_vsys_routetable_set(0, netns_id, vrf_info->vrf_name);

	return 0;
}

unsigned long xml_vrf_add(void *ptr)
{
	int rc;
	struct xml_ifname *ifgroup = NULL;
	char real_name[MAX_NETNS_NAME_LEN];
	int netns_id = 0;
	struct xml_vrf *vrf_info = ptr;

	netns_id = dp_netns_get_vrfid_by_name(vrf_info->vrf_name);
	if(NETNS_ID_VAILD(netns_id)) 
		return 867;

	netns_id = dp_netns_add(vrf_info->vrf_name, 0);
	if (netns_id < 0)
		return 864;
	
	/* 创建netns对应的route-table */
	guish_vsys_routetable_set(1, netns_id, vrf_info->vrf_name);

	/*添加描述*/	
	rc = dp_netns_add_desp(vrf_info->vrf_name, vrf_info->desc);
	if (rc < 0){
		rc = 865;
		goto ERROR;
	}

	ifgroup = vrf_info->vrf_ifname_items;
	while(ifgroup)
	{
		memset(real_name, 0, MAX_NETNS_NAME_LEN);
		if_get_name_by_alias(ifgroup->ifname, real_name);
		if(xml_get_zone_by_ifname((real_name[0]=='\0')?ifgroup->ifname:real_name)){
            rc = 108;
            goto ERROR;
		}
		rc = alloc_interface_to_vrf(vrf_info->vrf_name, (real_name[0]=='\0')?ifgroup->ifname:real_name);
		if (rc != 0){
			goto ERROR;
		}
		ifgroup = ifgroup->next;
	}

	return rc;
ERROR:
	xml_vrf_del(ptr);
	return rc;
}

unsigned long xml_vrf_mod(void *ptr)
{
	int rc;
	struct  xml_vrf *vrf_info = (struct  xml_vrf *)ptr;
	/*mod不可以修改名称*/

	/*添加描述*/	
	rc = dp_netns_add_desp(vrf_info->vrf_name, vrf_info->desc);
	if (rc < 0)
		return 865;

	xml_vrf_del_ifname_from_list(vrf_info);
	return xml_vrf_add_ifname_to_list(vrf_info);
}

static char * xml_vrf_showone(void *ptr)
{
	struct xml_vrf *xml_vrf = (struct xml_vrf *)ptr;
	xmlNode *root;
	xmlDoc *global_doc;
	
	root = xml_start_more(&global_doc, "vrf");
	vrf_xml_show_one_info(root, xml_vrf->vrf_name);
		
	return xml_end(global_doc, VRF_SAVE_TMP);
}

static char *xml_vrf_show(void *ptr)
{
	xmlNode *root;
	xmlNode *parent;
	xmlDoc *global_doc;

	int rc, retval;
	int i, outlen;
	int  vrf_num = 0;
	struct netns_set_msg req;
	struct netns_msg_reply *reply;
	
	const struct gui_xml_attr *sattr = gui_get_attr(ptr);
	int page_count = sattr->count;
	
	req.msg.opt = IPCMSG_DP_NETNS_SHOWALL;
	req.data.pos = attr_calc_pos(sattr);
	
	root = xml_start_more(&global_doc, "vrf");

	do {
		reply = get_prealloc_outbuf();		
		outlen = sizeof(*reply);
		rc = ipc_send_and_recv(APP_DPLANE, IPCMSG_NETNS_NOTIFY,
					&req, sizeof(req), &retval, reply, &outlen);
		if (rc < 0 || retval < 0) {
			break;
		}
		
		vrf_num = reply->count;
		char vrf_name[vrf_num][XML_MAX_NAME_LEN];
		for (i = 0; i < vrf_num; i++) {
			strncpy(vrf_name[i], reply->entry[i].name, MAX_NETNS_NAME_LEN);
		}

		for (i = 0; i < vrf_num; i++) {
			/*sattr->count <= 0 代表不需要分页*/
			if (page_count > 0 || sattr->count <= 0){
				vrf_xml_show_one_info(root, vrf_name[i]);
				page_count--;
			}
		}
		
		req.data.pos += vrf_num;
	} while (vrf_num == NETNS_SHOW_NUM);

	if (sattr->count)
		xml_add_page(root, sattr, req.data.pos);

	return xml_end(global_doc, VRF_SAVE_TMP);
}

#define TMP_BUFLEN 17
static char * xml_vrf_showindex(void *ptr)
{
	struct xml_vrf *xml_vrf = (struct xml_vrf *)ptr;
	xmlNode *root;
	xmlNode *parent;
	xmlDoc *global_doc;
	int rc, retval;
	int i, outlen;
	struct netns_set_msg req;
	struct netns_msg_reply *reply;	
	char alias_name[MAX_NETNS_NAME_LEN];
	
	root = xml_start_more(&global_doc, "vrf");

	memset(&req, 0, sizeof(req));
	req.msg.opt = IPCMSG_DP_NETNS_SHOWONE;
	if (xml_vrf->vrf_name[0] != '\0')
		strncpy(req.data.name, xml_vrf->vrf_name, MAX_NETNS_NAME_LEN);
	else
		strncpy(req.data.name, NETNS_DEFAULT_NAME, MAX_NETNS_NAME_LEN);
	req.data.pos = 0;
	
	do {
		reply = get_prealloc_outbuf();
		outlen = sizeof(*reply);
		rc = ipc_send_and_recv(APP_DPLANE, IPCMSG_NETNS_NOTIFY,
					&req, sizeof(req), &retval, reply, &outlen);
		if (rc < 0 || retval < 0) {
			return 0;
		}
		
		for (i = 0; i < reply->count; i++) {
			struct netns_entry *entry = &reply->entry[i];
			char buf[16];
			int sys_used = 0;
			rc = guish_netns_check(entry->ifname);
			if (rc)
				sys_used = 1;//continue;
			parent = vsos_xmlNewChild(root, NULL,
				BAD_CAST "group", BAD_CAST NULL);

			memset(alias_name, 0, MAX_NETNS_NAME_LEN);
			if_get_alias_by_name(entry->ifname, alias_name);
			
			if(strlen(alias_name))
				vsos_xmlNewChild(parent, NULL, BAD_CAST "name", BAD_CAST alias_name);
			else									
				vsos_xmlNewChild(parent, NULL, BAD_CAST "name", BAD_CAST entry->ifname);

            memset(buf, 0, 16);
            snprintf(buf, 16, "%d", sys_used);
            vsos_xmlNewChild(parent, NULL, BAD_CAST "sys_used", BAD_CAST buf);
		}
		
		req.data.pos += reply->count;
	} while (reply->count == NETNS_SHOW_NUM);

		
	return xml_end(global_doc, VRF_SAVE_TMP);
}
#if 0
{
	xmlNode *root;
	xmlNode *parent;
	xmlDoc *global_doc;

	int rc, retval;
	int i, outlen;
	int  vrf_num = 0;
	struct netns_set_msg req;
	struct netns_msg_reply *reply;
	char buf[TMP_BUFLEN];
	
	req.msg.opt = IPCMSG_DP_NETNS_SHOWALL;
	req.data.pos = 0;
	
	root = xml_start_more(&global_doc, "vrf");

	do {
		reply = get_prealloc_outbuf();
		outlen = sizeof(*reply);
		rc = ipc_send_and_recv(APP_DPLANE, IPCMSG_NETNS_NOTIFY,
					&req, sizeof(req), &retval, reply, &outlen);
		if (rc < 0 || retval < 0) {
			break;
		}
		vrf_num = reply->count;

		for (i = 0; i < reply->count; i++) {
			struct netns_entry *entry = &reply->entry[i];
			parent = vsos_xmlNewChild(root, NULL, 
				BAD_CAST "group", BAD_CAST NULL);
			vsos_xmlNewChild(parent, NULL, 
				BAD_CAST "vrf_name", BAD_CAST entry->name);
			memset(buf, 0, TMP_BUFLEN);
			snprintf(buf, TMP_BUFLEN-1, "%d", entry->id);		
			vsos_xmlNewChild(parent, NULL, BAD_CAST"id", BAD_CAST buf);
		}

		req.data.pos += vrf_num;

	} while (vrf_num == NETNS_SHOW_NUM);

	return xml_end(global_doc, VRF_SAVE_TMP);
}
#endif

unsigned long xml_vsys_spec_mod(void *ptr)
{
 	int rc;
	int outlen, retval = 0;
	struct vsys_set_max *req;
	struct vsys_spec_msg *rsp;
    struct xml_vsys_spec *spec_xml = ptr;
    int vsysid;
    struct
    {
        struct oam_data_st oh;
        struct vsys_spec_msg data;
    }buf;

    vsysid = dp_netns_get_vrfid_by_name(spec_xml->vsys_name);
    if (vsysid < 0) {
        return -1;
    }

    req = get_prealloc_buf();
	rsp = get_prealloc_outbuf();
	bzero(req, sizeof(*req));
	outlen = sizeof(*rsp);

    req->msg.opt = IPCMSG_DP_SPEC_ADD;
    req->data.vsysid = vsysid;
	req->data.spec_max[VSYS_SPEC_USROBJ]    = spec_xml->userobj_max;
	req->data.spec_max[VSYS_SPEC_USRGRP]    = spec_xml->usergrp_max;
	req->data.spec_max[VSYS_SPEC_APPGRP]    = spec_xml->appgrp_max;
	req->data.spec_max[VSYS_SPEC_FLOG]      = spec_xml->floglog_max;
    req->data.spec_max[VSYS_SPEC_KEYWORD]   = spec_xml->keyword_max;
    req->data.spec_max[VSYS_SPEC_SERVEROBJ] = spec_xml->sevobj_max;
    req->data.spec_max[VSYS_SPEC_TROBJ]     = spec_xml->trobj_max;
    req->data.spec_max[VSYS_SPEC_FWPLCY]    = spec_xml->fw_plcy_max;
    req->data.spec_max[VSYS_SPEC_FWPLCY6]   = spec_xml->fw_plcy_v6_max;
    req->data.spec_max[VSYS_SPEC_ADDROBJ]   = spec_xml->address_max;
	req->data.spec_max[VSYS_SPEC_ADDRGRP]   = spec_xml->addressgrp_max;
    req->data.spec_max[VSYS_SPEC_FLOW]      = spec_xml->flow_max;
    req->data.spec_max[VSYS_SPEC_NAT]       = spec_xml->nat_rule_max;
    req->data.spec_max[VSYS_SPEC_POOL]      = spec_xml->addr_pool_max;
    req->data.spec_max[VSYS_SPEC_ZONE]      = spec_xml->security_zone_max;
    req->data.spec_max[VSYS_SPEC_UPLCY]     = spec_xml->uplcy_max;
    req->data.spec_max[VSYS_SPEC_IPMAC]     = spec_xml->ipmac_max;


	rc = ipc_send_and_recv(APP_DPLANE, IPCMSG_NETNS_NOTIFY,
		req, sizeof(*req), &retval, rsp, &outlen);
    if (rc < 0) {
        return -1;
    }

//vsys aaa
  	memset(&buf, 0, sizeof buf);
	buf.data.vsysid = vsysid;
	buf.oh.cmd_code = AAA_VSYS_SPEC_ADD;
	buf.oh.mod_id = VTYSH_INDEX_AAA;

    buf.data.aaa_spec_max[0] = spec_xml->radius_sev_max;
    buf.data.aaa_spec_max[1] = spec_xml->ldap_sev_max;

	write(client_xml[VTYSH_INDEX_AAA].fd, &buf, sizeof buf);

	while (1) {
		int nbytes;
		memset(&buf, 0, sizeof(buf));
		nbytes = read (client_xml[VTYSH_INDEX_AAA].fd, &buf, sizeof(buf));
		if (nbytes <= 0 && errno != EINTR) {
			return CMD_WARNING;
		}
		if(nbytes > 0){
            break;
		}
	}

    return 0;
}

char *xml_vsys_spec_show(void *ptr)
{
 	int rc;
 	struct xml_vsys_spec *data = (struct xml_vsys_spec*)ptr;
	int outlen, retval = 0;
	struct vsys_set_max *req;
	struct vsys_spec_msg *rsp;
    xmlNode *root, *parent;
    xmlDoc *global_doc;
    char tmp[64];
    int vsysid;
    struct
    {
        struct oam_data_st oh;
        struct vsys_spec_msg data;
    }buf;

    req = get_prealloc_buf();
	rsp = get_prealloc_outbuf();
	bzero(req, sizeof(*req));
	outlen = sizeof(*rsp);

    vsysid = dp_netns_get_vrfid_by_name(data->vsys_name);
    if (vsysid < 0) {
        goto out;
    }

    req->msg.opt = IPCMSG_DP_SPEC_SHOWA;
    req->data.vsysid = vsysid;

	root = xml_start_more(&global_doc, "vsys_spec");

	rc = ipc_send_and_recv(APP_DPLANE, IPCMSG_NETNS_NOTIFY,
		req, sizeof(*req), &retval, rsp, &outlen);
    if (rc < 0 || retval < 0) {
        GUI_DEBUG("Error: %s\n", ipc_get_emsg());
    }

    memset(&buf, 0, sizeof buf);
	buf.data.vsysid = vsysid;
	buf.oh.cmd_code = AAA_VSYS_SPEC_SHOWA;
	buf.oh.mod_id = VTYSH_INDEX_AAA;

	write(client_xml[VTYSH_INDEX_AAA].fd, &buf, sizeof buf);
	while (1) {
		int nbytes;
		memset(&buf, 0, sizeof(buf));
		nbytes = read (client_xml[VTYSH_INDEX_AAA].fd, &buf, sizeof(buf));
		if (nbytes <= 0 && errno != EINTR) {
			goto out;
		}
		if(nbytes > 0){
            break;
		}
	}

    parent = vsos_xmlNewChild(root, NULL,
        BAD_CAST "group", BAD_CAST NULL);

    snprintf(tmp, 64, "%d", rsp->spec_max[VSYS_SPEC_USROBJ]);
    vsos_xmlNewChild(parent, NULL,
        BAD_CAST "userobj_max",
        BAD_CAST tmp);
    snprintf(tmp, 64, "%d", rsp->spec_max[VSYS_SPEC_USRGRP]);
    vsos_xmlNewChild(parent, NULL,
        BAD_CAST "usergrp_max",
        BAD_CAST tmp);
    snprintf(tmp, 64, "%d", rsp->spec_max[VSYS_SPEC_APPGRP]);
    vsos_xmlNewChild(parent, NULL,
        BAD_CAST "appgrp_max",
        BAD_CAST tmp);
    snprintf(tmp, 64, "%d", rsp->spec_max[VSYS_SPEC_FLOG]);
    vsos_xmlNewChild(parent, NULL,
        BAD_CAST "floglog_max",
        BAD_CAST tmp);
    snprintf(tmp, 64, "%d", rsp->spec_max[VSYS_SPEC_KEYWORD]);
    vsos_xmlNewChild(parent, NULL,
        BAD_CAST "keyword_max",
        BAD_CAST tmp);
    snprintf(tmp, 64, "%d", rsp->spec_max[VSYS_SPEC_SERVEROBJ]);
    vsos_xmlNewChild(parent, NULL,
        BAD_CAST "sevobj_max",
        BAD_CAST tmp);
    snprintf(tmp, 64, "%d", rsp->spec_max[VSYS_SPEC_TROBJ]);
    vsos_xmlNewChild(parent, NULL,
        BAD_CAST "trobj_max",
        BAD_CAST tmp);
    snprintf(tmp, 64, "%d", rsp->spec_max[VSYS_SPEC_FWPLCY]);
    vsos_xmlNewChild(parent, NULL,
        BAD_CAST "fw_plcy_max",
        BAD_CAST tmp);
    snprintf(tmp, 64, "%d", rsp->spec_max[VSYS_SPEC_FWPLCY6]);
    vsos_xmlNewChild(parent, NULL,
        BAD_CAST "fw_plcy_v6_max",
        BAD_CAST tmp);
    snprintf(tmp, 64, "%d", rsp->spec_max[VSYS_SPEC_ADDROBJ]);
    vsos_xmlNewChild(parent, NULL,
        BAD_CAST "address_max",
        BAD_CAST tmp);
	snprintf(tmp, 64, "%d", rsp->spec_max[VSYS_SPEC_ADDRGRP]);
    vsos_xmlNewChild(parent, NULL,
        BAD_CAST "addressgrp_max",
        BAD_CAST tmp);
    snprintf(tmp, 64, "%d", rsp->spec_max[VSYS_SPEC_FLOW]);
    vsos_xmlNewChild(parent, NULL,
        BAD_CAST "flow_max",
        BAD_CAST tmp);
    snprintf(tmp, 64, "%d", rsp->spec_max[VSYS_SPEC_NAT]);
    vsos_xmlNewChild(parent, NULL,
        BAD_CAST "nat_rule_max",
        BAD_CAST tmp);
    snprintf(tmp, 64, "%d", rsp->spec_max[VSYS_SPEC_POOL]);
    vsos_xmlNewChild(parent, NULL,
        BAD_CAST "addr_pool_max",
        BAD_CAST tmp);
    snprintf(tmp, 64, "%d", rsp->spec_max[VSYS_SPEC_ZONE]);
    vsos_xmlNewChild(parent, NULL,
        BAD_CAST "security_zone_max",
        BAD_CAST tmp);
    snprintf(tmp, 64, "%d", rsp->spec_max[VSYS_SPEC_UPLCY]);
    vsos_xmlNewChild(parent, NULL,
        BAD_CAST "uplcy_max",
        BAD_CAST tmp);
    snprintf(tmp, 64, "%d", rsp->spec_max[VSYS_SPEC_IPMAC]);
    vsos_xmlNewChild(parent, NULL,
        BAD_CAST "ipmac_max",
        BAD_CAST tmp);
    snprintf(tmp, 64, "%d", buf.data.aaa_spec_max[0]);
    vsos_xmlNewChild(parent, NULL,
        BAD_CAST "radius_sev_max",
        BAD_CAST tmp);
    snprintf(tmp, 64, "%d", buf.data.aaa_spec_max[1]);
    vsos_xmlNewChild(parent, NULL,
        BAD_CAST "ldap_sev_max",
        BAD_CAST tmp);
out:
    return xml_end(global_doc, VRF_SPEC_TMP);
}

char *xml_vsys_spec_get_info_show(void *ptr)
{
 	int rc;
    xmlNode *root, *parent;
    xmlDoc *global_doc;
	struct moduleConfig VsysCfg;
    char tmp[64];

	
    root = xml_start_more(&global_doc, "vsys_spec_get_info");

    parent = vsos_xmlNewChild(root, NULL,
        BAD_CAST "group", BAD_CAST NULL);

    global_get_module_config(&VsysCfg, INIT_MODULE_ID_VSYS, VSYS_MODULE_OBJ_OBJECT_MAXNUM);
    snprintf(tmp, 64, "%d", VsysCfg.ObjValue);
    vsos_xmlNewChild(parent, NULL,
        BAD_CAST "vsysObjectMaxNumber",
        BAD_CAST tmp);
    global_get_module_config(&VsysCfg, INIT_MODULE_ID_VSYS, VSYS_MODULE_OBJ_POLICY_MAXNUM);
    snprintf(tmp, 64, "%d", VsysCfg.ObjValue);
    vsos_xmlNewChild(parent, NULL,
        BAD_CAST "vsysPolicyMaxNumber",
        BAD_CAST tmp);
    global_get_module_config(&VsysCfg, INIT_MODULE_ID_VSYS, VSYS_MODULE_OBJ_CTFLOW_MAXNUM);
    snprintf(tmp, 64, "%d", VsysCfg.ObjValue);
    vsos_xmlNewChild(parent, NULL,
        BAD_CAST "vsysCTFlowMaxNumber",
        BAD_CAST tmp);
    global_get_module_config(&VsysCfg, INIT_MODULE_ID_VSYS, VSYS_MODULE_OBJ_SECDOMAIN_MAXNUM);
    snprintf(tmp, 64, "%d", VsysCfg.ObjValue);
    vsos_xmlNewChild(parent, NULL,
        BAD_CAST "vsysSecDomainMaxNumber",
        BAD_CAST tmp);
    global_get_module_config(&VsysCfg, INIT_MODULE_ID_VSYS, VSYS_MODULE_OBJ_ADDRPOOL_MAXNUM);
    snprintf(tmp, 64, "%d", VsysCfg.ObjValue);
    vsos_xmlNewChild(parent, NULL,
        BAD_CAST "vsysAddrPoolMaxNumber",
        BAD_CAST tmp);

    return xml_end(global_doc, VRF_SPEC_TMP);
}

int vrf_logcontent(unsigned char log, void *ptr, char *Operate)
{
    struct xml_vrf* data = (struct xml_vrf*)ptr;
    switch(log)
    {
        case WEB_OPERATION_MOD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Mod vrf.");

        case WEB_OPERATION_ADD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Add vrf.");

        case WEB_OPERATION_DEL:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Del vrf.");

        default:
            return 0;
    }
}

struct module_node_parameter vrf_module_parameter =
{
	"vrf",
	xml_vrf_add,
	xml_vrf_del,
	xml_vrf_mod,
	xml_vrf_show,
	xml_vrf_showindex,
	xml_vrf_showone,
	NULL,
	1,
	sizeof(struct xml_vrf),
	NULL,
	NULL,
	.func_logcontent = vrf_logcontent,
	.categs[XML_OPT_ADD] = CATEG_CONFIG,
	.categs[XML_OPT_DEL] = CATEG_CONFIG,
	.categs[XML_OPT_MOD] = CATEG_CONFIG,
};
// vsys specification value
int vsys_spec_logcontent(unsigned char log, void *ptr, char *Operate)
{
    struct xml_vsys_spec* data = (struct xml_vsys_spec*)ptr;
    switch(log)
    {
        case WEB_OPERATION_MOD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Mod vsys_spec.");

        case WEB_OPERATION_ADD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Add vsys_spec.");

        case WEB_OPERATION_DEL:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Del vsys_spec.");

        default:
            return 0;
    }
}

struct module_node_parameter vsys_spec_module_parameter =
{
	"vsys_spec",
	NULL,
	NULL,
	xml_vsys_spec_mod,
	xml_vsys_spec_show,
	NULL,
	xml_vsys_spec_show,
	NULL,
	1,
	sizeof(struct xml_vsys_spec),
	NULL,
	NULL,
	.func_logcontent = vsys_spec_logcontent,
	.categs[XML_OPT_MOD] = CATEG_CONFIG,
};

struct module_node_parameter vsys_spec_get_module_parameter =
{
	"vsys_spec_get_info",
	NULL,
	NULL,
	NULL,
	xml_vsys_spec_get_info_show,
	NULL,
	NULL,
	NULL,
	1,
	sizeof(struct xml_vsys_spec),
	NULL,
	NULL,
	.func_logcontent = NULL,
};

unsigned long xml_vsys_switch_mod(void *ptr)
{
    struct xml_vsys_switch_ui *data = (struct xml_vsys_switch_ui *)ptr;
    int vsysid, cur_vsysid = GUI_CUR_VSYSID;
    unsigned int login_vsysid;

    xml_vsys_gui_get_login_vsysid(&login_vsysid);

    if (login_vsysid) {
        return 533;
    }

    vsysid = dp_netns_get_vrfid_by_name(data->vsys_name);

    if (data->vsysid) {
        xml_vsys_gui_info_set(vsysid, VSYS_UI_VSYS);
    }
    else {
        xml_vsys_gui_info_set(vsysid, VSYS_UI_ROOT);
    }

    return 0;
}

static char *xml_vsys_switch_show(void *ptr)
{
    xmlNode *root, *parent;
    xmlDoc *global_doc;
    char vsysid[16];

    root = xml_start_more(&global_doc, "vsys_switch_ui");
    parent = vsos_xmlNewChild(root, NULL,
        BAD_CAST "group", BAD_CAST NULL);

    sprintf(vsysid, "%d", GUI_CUR_VSYSID);

    vsos_xmlNewChild(parent, NULL, BAD_CAST"vsysid", BAD_CAST vsysid);

    return xml_end(global_doc, VRF_VSYSSWITCH_TMP);
}

struct module_node_parameter vsys_switch_module_parameter =
{
	"vsys_switch_ui",
	NULL,
	NULL,
	xml_vsys_switch_mod,
	xml_vsys_switch_show,
	NULL,
	NULL,
	NULL,
	1,
	sizeof(struct xml_vsys_switch_ui),
	NULL,
	NULL,
};

static char *xml_vsys_if_show(void *ptr)
{
    xmlNode *root;
	xmlNode *parent;
	xmlDoc *global_doc;
	FILE *fp;
	char buf[1024 + 1];
	int cur_vsysid = GUI_CUR_VSYSID, if_vsysid;

	root = xml_start_more(&global_doc, "vsys_if");

	fp = fopen (_PATH_PROC_NET_DEV, "r");
	if (fp == NULL)
		return NULL;

    /* Drop header lines. */
	fgets (buf, 1024, fp);
	fgets (buf, 1024, fp);

    while (fgets (buf, 1024, fp) != NULL) {
        buf[1024] = '\0';
		char *s, name[IFNAMSIZ];
		s = get_name(name, buf);
		char buf[50];
		struct prefix addr;

		if(if_type_get(name) != IF_ETHERNET)
			continue;
		if(!strcmp(name, "npec"))
			continue;
		if(!strncmp(name, "swi", 3))
			continue;
		if(!strncmp("nsa", name, 3))
			continue;
		if(!strncmp(name, "loop", 4))
			continue;
		if(!strncmp(name, "lo", 2))
			continue;
		if(!strncmp(name, "eth", 3))
			continue;
        if(!strncmp(name, "tun", 3))
			continue;
        if(!strncmp(name, "vlan", 4))
			continue;
		if(strchr(name, '.'))
			continue;

        if_vsysid = dp_netns_get_vrfid_by_ifname(name);
        if (if_vsysid != cur_vsysid) {
            continue;
        }

        parent = vsos_xmlNewChild( root, NULL, BAD_CAST"group", BAD_CAST NULL );
        vsos_xmlNewChild( parent, NULL, BAD_CAST"ifname", BAD_CAST name);
    }

	fclose(fp);

	return xml_end(global_doc, VRF_VSYSIF_TMP);
}

struct module_node_parameter vsys_ifname_module_parameter =
{
	"vsys_if",
	NULL,
	NULL,
	NULL,
	xml_vsys_if_show,
	NULL,
	NULL,
	NULL,
	1,
	sizeof(struct xml_vsys_ifname),
	NULL,
	NULL,
};


/******************记得向xml_module_init中注册*****************/
void xml_vrf_init(void)
{
	struct module_node *master_module;

	master_module = __create_module_node(vrf_element_parameter,
		&vrf_module_parameter, &vrf_cfg, CATEG_MANAGE);

	create_sub_element(master_module, "vrf_ifname_items",
		vrf_ifname_element_parameter, sizeof(struct xml_ifname));
	if (!master_module)
		return;

	master_module = __create_module_node(vsys_spec_element_parameter,
		&vsys_spec_module_parameter, &vsys_spec_data, CATEG_MANAGE);
	if (!master_module)
		return ;
	
    master_module = __create_module_node(vsys_spec_get_info_element_parameter,
		&vsys_spec_get_module_parameter, &xml_vsys_spec_get_info, CATEG_MANAGE);
	if (!master_module)
		return ;

	master_module = __create_module_node(vsys_switch_element_parameter,
		&vsys_switch_module_parameter, &vsys_switch_data, CATEG_MANAGE);
	if (!master_module)
		return ;

    master_module = __create_module_node(vsys_ifname_element_parameter,
		&vsys_ifname_module_parameter, &vsys_ifname_data, CATEG_MANAGE);
	if (!master_module)
		return ;

	GUI_DEBUG("Load xml module: vsys ok.\n");
}

