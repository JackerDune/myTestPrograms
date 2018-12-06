#include "xml_common.h"
#include "xml_zebra.h"
#include "memory.h"
#include <linux/if.h>
#include "../zebra/zebra_vty.h"
#include "../zebra/rib.h"
#include "linklist.h"
#include "if_operate.h"
#include "linux/if_arp.h"
#include "../pppoe/pppoe_vty.h"
#include "../dhcpd/dhcp_vty.h"
#if 0
#include "linux/eth_if_stuff.h"
#endif
#include "mac.h"
#include "network.h"
#include "../anti-spam/db.h"
#include "../ddns/ddns.h"
#include "../ha/ha_vty.h"
#include "../lib/vsos_syslib.h"
#include "if_layout.h"
#include "../link_bind/link_bind_vty.h"
#include <net/fw_objects.h>
#include "../lib/isp_route_lib.h"
#include "../lib/libtrunk.h"
#include <if_msg.h>
#include <device_stat_save.h>
#include <sunya_ipc.h>
#include <sunya_util.h>
#include <netns_msg.h>


#if 0 //#ifdef CONFIG_VSYS
#include "xml_vsys.h"
//unsigned int vsysid_if_show;
#endif

#include <linux/ethtool.h>
#include "sockunion.h"
#include "if.h"

#include "global_conf.h"
#include "linux/tbk_local_access.h"
#include "xml_vsys.h"

#ifdef CONGFIG_ADC_SUPPORT
#include "../bind-9.9.2/bin/named/gtm_vty.h"
#endif


#ifndef _PATH_PROC_NET_DEV
#define _PATH_PROC_NET_DEV        "/proc/net/dev"
#endif

struct interface_node interface_node_data;
struct inf_select inf_select_data;
struct static_route	static_route_data;
struct rib_table	rib_route_data;
struct default_gate	default_gate_data;
struct rename	rename_data;
struct p2p_if	p2p_if_data;

struct interface_status	interface_status_data;


struct static_mroute_t static_mroute;
struct out_interfaces static_mroute_if_sub;

unsigned long xml_loop_interface_del(void *ptr);
unsigned long xml_interface_node_add(void *ptr);
unsigned long xml_interface_node_mod(void *ptr);
unsigned long xml_interface_node_del(void *ptr);
char *xml_interface_node_showall(void *ptr);
char *xml_interface_node_showindex(void *ptr);
char *xml_inf_select_showindex(void *ptr);
char *xml_ptp_if_showindex(void *ptr);
char *xml_p2p_if_showindex(void *ptr);

unsigned long xml_static_route_add(void *ptr);
unsigned long xml_static_route_del(void *ptr);
char *xml_static_route_show(void *ptr);

unsigned long xml_default_gate_add(void *ptr);
unsigned long xml_default_gate_del(void *ptr);
char *xml_default_gate_show(void *ptr);

char *xml_rib_route_show(void *ptr);
unsigned long xml_rename_mod(void *ptr);
unsigned long xml_interface_status_mod(void *ptr);


unsigned long xml_static_mroute_add(void *ptr);
unsigned long xml_static_mroute_del(void *ptr);
unsigned long xml_static_mroute_mod(void *ptr);
char *xml_static_mroute_showall(void *ptr);

extern int if_ipv6_address_config_clear(char *ifname);
extern int vsos_ipv6_check_addr(char *addr_str, struct prefix_ipv6 *p);
extern int if_has_ipv6(char *ifname);
extern int zonename_search (char *name);
extern int if_show_index_select(struct vtysh_client *client, char *ifname, int type);


int isp_route_node_update(char *ifname)
{
	struct {
		struct oam_data_st oh;
		struct isp_rt_cfg data;
	}  buf;

	memset(&buf, 0, sizeof buf);
	buf.oh.cmd_code = ZEBRA_UPDATE_ISP_NODE;
	buf.oh.mod_id = VTYSH_INDEX_ZEBRA;
	strncpy(buf.data.gate_str, ifname, sizeof(buf.data.gate_str)-1);
	write(client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof buf);


	while (1) {
		int nbytes;

		nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof(buf));

		if (nbytes <= 0 && errno != EINTR) {
			return 0;
		}

		if (nbytes > 0) {
			break;
		}
	}

	return 0;
}


int if_address_config_clear(char *ifname)
{
	/*clear static ip address*/
	{
		struct {
			struct oam_data_st oh;
			struct zebra_ip_addr_st data;
		}  buf;

		memset(&buf, 0, sizeof buf);
		buf.oh.cmd_code = ZEBRA_DEL_ADDRESS;
		buf.oh.mod_id = VTYSH_INDEX_ZEBRA;
		strncpy(buf.data.ifname , ifname, 63);
		write( client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof buf);


		while (1) {
			int nbytes;

			nbytes = read ( client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof(buf)-1);

			if (nbytes <= 0 && errno != EINTR) {
				return 0;
			}

			if (nbytes > 0) {
				break;
			}
		}
	}
	/*clear pppoe ip address*/
	{
		struct {
			struct oam_data_st oh;
			struct pppoe_link_st data;
		}  buf;

		memset(&buf, 0, sizeof buf);
		buf.oh.cmd_code = PPPOE_DISABLE;
		buf.oh.mod_id = VTYSH_INDEX_PPPOE;
		strncpy(buf.data.ifname , ifname, IFNAMSIZ-1);
		write(client_xml[VTYSH_INDEX_PPPOE].fd, &buf, sizeof buf);


		while (1) {
			u32 nbytes;

			nbytes = read ( client_xml[VTYSH_INDEX_PPPOE].fd, &buf, sizeof(buf)-1);

			if (nbytes <= 0 && errno != EINTR) {
				return 0;
			}

			if (nbytes > 0) {
				break;
			}
		}
	}
	/*clear dhcp address*/
	{
		struct {
			struct oam_data_st oh;
			char rbuf[VTY_READ_BUFSIZ];
		}  buf;

		memset(&buf, 0, sizeof buf);
		buf.oh.cmd_code = DHCP_IF_CLIENT_DISABLE;
		buf.oh.mod_id = VTYSH_INDEX_DHCP;
		strncpy(buf.rbuf, ifname, IFNAMSIZ);
		write( client_xml[VTYSH_INDEX_DHCP].fd, &buf, sizeof (struct oam_data_st) + IFNAMSIZ);

		while (1) {
			int nbytes;

			nbytes = read ( client_xml[VTYSH_INDEX_DHCP].fd, &buf, sizeof(buf));

			if (nbytes <= 0 && errno != EINTR) {
				return 0;
			}

			if (nbytes > 0) {
				break;
			}
		}
	}
	if_address_type_set(ifname,client_xml[VTYSH_INDEX_ZEBRA].fd, 0);
	return 0;
}

PARSE_XML_INT(inf_select, get_type, struct inf_select)
PARSE_XML_INTERFACE(inf_select, name, struct inf_select, XML_MAX_NAME_LEN)
PARSE_XML_STRING(inf_select, real_name, struct inf_select, IFNAMSIZ)
PARSE_XML_STRING(inf_select, alias_name, struct inf_select, XML_MAX_NAME_LEN)
PARSE_XML_STRING(inf_select, vrf_name, struct inf_select, XML_MAX_NAME_LEN)

PARSE_XML_INT(interface_sub, type, struct interface_node)
PARSE_XML_INTERFACE(interface_sub, name, struct interface_node, XML_MAX_NAME_LEN)
PARSE_XML_STRING(interface_sub, real_name, struct interface_node, IFNAMSIZ)
PARSE_XML_STRING(interface_sub, alias_name, struct interface_node, XML_MAX_NAME_LEN)
PARSE_XML_INT(interface_sub, vlan_id, struct interface_node)
PARSE_XML_INTERFACE(interface_sub, vlan_phy_if, struct interface_node, XML_MAX_NAME_LEN)
#if 0 //#ifdef CONFIG_VSYS
PARSE_XML_STRING(interface_sub, vsys_name, struct interface_node, 32)
#endif
PARSE_XML_STRING(interface_sub, desc, struct interface_node, 4*XML_MAX_NAME_LEN)
PARSE_XML_INT(interface_sub, address_type, struct interface_node)
PARSE_XML_IPMASK(interface_sub, ipaddr, struct interface_node)

PARSE_XML_INT(interface_sub, ddns_on, struct interface_node)
PARSE_XML_INT(interface_sub, ddns_type, struct interface_node)
PARSE_XML_INT(interface_sub, ddns_interval, struct interface_node)
PARSE_XML_INT(interface_sub, ddns_status, struct interface_node)
PARSE_XML_STRING(interface_sub, ddns_domain, struct interface_node, XML_MAX_NAME_LEN)
PARSE_XML_STRING(interface_sub, ddns_user, struct interface_node, XML_MAX_NAME_LEN)
PARSE_XML_STRING(interface_sub, ddns_pass, struct interface_node, XML_MAX_NAME_LEN)
PARSE_XML_IP(interface_sub, ddns_myip, struct interface_node)
PARSE_XML_STRING(interface_sub, ddns_onlinetime, struct interface_node, XML_MAX_NAME_LEN)


PARSE_XML_INT(interface_sub, dhcp_distance, struct interface_node)
PARSE_XML_INT(interface_sub, dhcp_default_gate, struct interface_node)
PARSE_XML_INT(interface_sub, dhcp_dns, struct interface_node)
PARSE_XML_STRING(interface_sub, pppoe_user, struct interface_node, XML_MAX_NAME_LEN)
PARSE_XML_STRING(interface_sub, pppoe_passwd, struct interface_node, XML_MAX_NAME_LEN)
PARSE_XML_INT(interface_sub, pppoe_distance, struct interface_node)
PARSE_XML_INT(interface_sub, pppoe_weight, struct interface_node)
PARSE_XML_IP(interface_sub, pppoe_specify_ip, struct interface_node)
PARSE_XML_INT(interface_sub, pppoe_default_gate, struct interface_node)
PARSE_XML_INT(interface_sub, pppoe_dns, struct interface_node)
PARSE_XML_INT(interface_sub, http, struct interface_node)
PARSE_XML_INT(interface_sub, https, struct interface_node)
PARSE_XML_INT(interface_sub, telnet, struct interface_node)
PARSE_XML_INT(interface_sub, ping, struct interface_node)
PARSE_XML_INT(interface_sub, ssh, struct interface_node)
PARSE_XML_INT(interface_sub, bgp, struct interface_node)
PARSE_XML_INT(interface_sub, ospf, struct interface_node)
PARSE_XML_INT(interface_sub, rip, struct interface_node)
PARSE_XML_INT(interface_sub, dns, struct interface_node)
PARSE_XML_INT(interface_sub, tctrl, struct interface_node)
PARSE_XML_INT(interface_sub, l2tp, struct interface_node)
PARSE_XML_INT(interface_sub, sslvpn, struct interface_node)
PARSE_XML_INT(interface_sub, linkage, struct interface_node)


PARSE_XML_INT(interface_sub, bind_if_enable, struct interface_node)
PARSE_XML_INT(interface_sub, bind_type, struct interface_node)
PARSE_XML_STRING(interface_sub, if_name, struct interface_node, IFNAMSIZ)
PARSE_XML_STRING(interface_sub, monitor_addr, struct interface_node, XML_MAX_NAME_LEN)

PARSE_XML_INT(interface_sub, negotiate, struct interface_node)
PARSE_XML_INT(interface_sub, half, struct interface_node)
PARSE_XML_INT(interface_sub, speed, struct interface_node)
PARSE_XML_INT(interface_sub, mtu, struct interface_node)
PARSE_XML_INT(interface_sub, if_bandwidth, struct interface_node)
PARSE_XML_IPMASK(interface_sub, haipaddr, struct interface_node)
PARSE_XML_INT(interface_sub, shut, struct interface_node)
PARSE_XML_INT(interface_sub, max_speed, struct interface_node)
PARSE_XML_INT(interface_sub, webauth, struct interface_node)
PARSE_XML_INT(interface_sub, cen_monitor, struct interface_node)
PARSE_XML_INT(interface_sub, mgmt_flag, struct interface_node)
PARSE_XML_INT(interface_sub, external, struct interface_node)
PARSE_XML_INT(interface_sub, get_type, struct interface_node)
PARSE_XML_STRUCT(interface_sub, sec_ip, struct interface_node, struct second_ip)
PARSE_XML_IPMASK(second_ip, second_ip, struct second_ip)

/*tb new*/
PARSE_XML_STRUCT(interface_sub, tb_interface_ip, struct interface_node, struct tb_ip_info)
PARSE_XML_STRING(tb_ip_info, tb_ip, struct tb_ip_info, XML_MAX_IP6_LEN)
PARSE_XML_INT(tb_ip_info, is_floating_ip, struct tb_ip_info)
PARSE_XML_INT(tb_ip_info, unit_id, struct tb_ip_info)


PARSE_XML_STRING(static_route, dst_ip, struct static_route, XML_MAX_IP_LEN)
PARSE_XML_INT(static_route, nh_type, struct static_route)
PARSE_XML_STRING(static_route, nh_ip, struct static_route, XML_MAX_IP_LEN)
PARSE_XML_INTERFACE(static_route, oif, struct static_route, XML_MAX_NAME_LEN)
PARSE_XML_STRING(static_route, monitor_name, struct static_route, XML_MAX_NAME_LEN)
PARSE_XML_INT(static_route, distance, struct static_route)
PARSE_XML_INT(static_route, weigh, struct static_route)
PARSE_XML_INT(static_route, page, struct static_route)
PARSE_XML_INT(static_route, pageSize, struct static_route)
PARSE_XML_STRING(static_route, vrf_name, struct static_route, XML_MAX_NAME_LEN)

/*rib_table*/
PARSE_XML_INT(rib_table, type, struct rib_table)
PARSE_XML_IPMASK(rib_table, dst_ip, struct rib_table)
PARSE_XML_IP(rib_table, nh_ip, struct rib_table)
PARSE_XML_INTERFACE(rib_table, oif, struct rib_table, XML_MAX_NAME_LEN)
PARSE_XML_INT(rib_table, distance, struct rib_table)
PARSE_XML_INT(rib_table, weigh, struct rib_table)
PARSE_XML_STRING(rib_table, time, struct rib_table, XML_MAX_NAME_LEN)
PARSE_XML_INT(rib_table, status, struct rib_table)
PARSE_XML_INT(rib_table, page, struct rib_table)
PARSE_XML_INT(rib_table, pageSize, struct rib_table)
PARSE_XML_STRING(rib_table, vrf_name, struct rib_table, XML_MAX_NAME_LEN)


/*default_gate*/
PARSE_XML_STRING(default_gate, gate_ip, struct default_gate, XML_MAX_IP_LEN)
PARSE_XML_STRING(default_gate, oif, struct default_gate, XML_MAX_NAME_LEN)
PARSE_XML_INT(default_gate, get_style, struct default_gate)
PARSE_XML_INT(default_gate, weight, struct default_gate)
PARSE_XML_INT(default_gate, distance, struct default_gate)


/*rename*/
PARSE_XML_STRING(rename, old_name, struct rename, XML_MAX_NAME_LEN)
PARSE_XML_STRING(rename, new_name, struct rename, XML_MAX_NAME_LEN)

/*p2p_interface*/
PARSE_XML_STRING(p2p_if, vrf_name, struct p2p_if, XML_MAX_NAME_LEN)

/*interface_status*/
PARSE_XML_INTERFACE(interface_status, ifname, struct interface_status, XML_MAX_NAME_LEN)
PARSE_XML_INT(interface_status, shut, struct interface_status)

/* mroute table */
PARSE_XML_STRING(static_mroute, multi_group_ip, struct static_mroute_t, XML_MAX_IP_LEN)
PARSE_XML_STRUCT(static_mroute, out_interface, struct static_mroute_t, struct out_interfaces)
PARSE_XML_INTERFACE(static_mroute_if_sub, ifname, struct out_interfaces, IFNAMSIZ)

static struct element_node_parameter fake_element_parameter[] = {
	{"", NULL, NULL, NULL, TYPE_XML_INT}
};

struct element_node_parameter static_mroute_element_parameter[] = {
	HASH_NODE_PARAM_ADD(static_mroute, multi_group_ip, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(static_mroute, out_interface, TYPE_XML_STRUCT),
	{"", NULL, NULL, NULL, TYPE_XML_INT}
};

struct element_node_parameter static_mroute_sub_element_parameter[] = {
	HASH_NODE_PARAM_ADD(static_mroute_if_sub, ifname, TYPE_XML_STRING),
	{"", NULL, NULL, NULL, TYPE_XML_INT}
};

int static_mroute_logcontent(unsigned char log, void *ptr, char *Operate)
{
    struct static_route* data = (struct static_route*)ptr;
    switch(log)
    {
        case WEB_OPERATION_MOD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Mod static_mroute.");

        case WEB_OPERATION_ADD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Add static_mroute.");

        case WEB_OPERATION_DEL:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Del static_mroute.");

        default:
            return 0;
    }
}

struct module_node_parameter static_mroute_module_parameter = {
	"static_mroute",
	xml_static_mroute_add,
	xml_static_mroute_del,
	xml_static_mroute_mod,
	xml_static_mroute_showall,
	NULL,
	xml_static_mroute_showall,
	NULL,
	1,
	sizeof(struct static_route),
	.func_logcontent = static_mroute_logcontent,
};

struct element_node_parameter inf_select_element_parameter[] = {
	HASH_NODE_PARAM_ADD(inf_select, get_type, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(inf_select, name, TYPE_XML_INTERFACE),
	HASH_NODE_PARAM_ADD(inf_select, real_name, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(inf_select, alias_name, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(inf_select, vrf_name, TYPE_XML_STRING),

	{"", NULL, NULL, NULL, TYPE_XML_INT}
};

struct element_node_parameter interface_node_element_parameter[] = {
	HASH_NODE_PARAM_ADD(interface_sub, type, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, name, TYPE_XML_INTERFACE),
	HASH_NODE_PARAM_ADD(interface_sub, real_name, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(interface_sub, alias_name, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(interface_sub, vlan_id, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, vlan_phy_if, TYPE_XML_INTERFACE),
#if 0 //#ifdef CONFIG_VSYS
	HASH_NODE_PARAM_ADD(interface_sub, vsys_name, TYPE_XML_STRING),
#endif
	HASH_NODE_PARAM_ADD(interface_sub, desc, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(interface_sub, address_type, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, ipaddr, TYPE_XML_STRING),

	HASH_NODE_PARAM_ADD(interface_sub, ddns_on , TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, ddns_type , TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, ddns_interval , TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, ddns_status , TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, ddns_domain , TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(interface_sub, ddns_user , TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(interface_sub, ddns_pass , TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(interface_sub, ddns_myip , TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(interface_sub, ddns_onlinetime , TYPE_XML_STRING),


	HASH_NODE_PARAM_ADD(interface_sub, dhcp_distance, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, dhcp_default_gate, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, dhcp_dns, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, pppoe_user, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(interface_sub, pppoe_passwd, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(interface_sub, pppoe_distance, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, pppoe_weight, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, pppoe_specify_ip, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(interface_sub, pppoe_default_gate, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, pppoe_dns, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, http, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, https, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, telnet, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, ping, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, ssh, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, bgp, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, ospf, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, rip, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, dns, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, tctrl, TYPE_XML_INT),

	HASH_NODE_PARAM_ADD(interface_sub, l2tp, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, sslvpn, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, linkage, TYPE_XML_INT),

	HASH_NODE_PARAM_ADD(interface_sub, bind_if_enable, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, bind_type, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, if_name, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(interface_sub, monitor_addr, TYPE_XML_STRING),

	HASH_NODE_PARAM_ADD(interface_sub, negotiate, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, half, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, speed, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, mtu, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, if_bandwidth, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, shut, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, haipaddr, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(interface_sub, max_speed, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, webauth, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, cen_monitor, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, mgmt_flag, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, external, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, get_type, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_sub, sec_ip, TYPE_XML_STRUCT),

	/*tb new*/
	HASH_NODE_PARAM_ADD(interface_sub, tb_interface_ip, TYPE_XML_STRUCT),

	{"", NULL, NULL, NULL, TYPE_XML_INT}
};

struct element_node_parameter second_ip_element_parameter[] = {
	HASH_NODE_PARAM_ADD(second_ip, second_ip, TYPE_XML_STRING),
	{"", NULL, NULL, NULL, TYPE_XML_INT}
};

/*tb new*/
struct element_node_parameter tb_interface_ip_element_parameter[] = {
	HASH_NODE_PARAM_ADD(tb_ip_info, tb_ip, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(tb_ip_info, is_floating_ip, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(tb_ip_info, unit_id, TYPE_XML_INT),
	{"", NULL, NULL, NULL, TYPE_XML_INT}
};

struct element_node_parameter static_route_element_parameter[] = {
	HASH_NODE_PARAM_ADD(static_route, dst_ip, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(static_route, nh_type, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(static_route, nh_ip, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(static_route, oif, TYPE_XML_INTERFACE),
	HASH_NODE_PARAM_ADD(static_route, monitor_name, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(static_route, distance, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(static_route, weigh, TYPE_XML_INT),
    HASH_NODE_PARAM_ADD(static_route, page, TYPE_XML_INT),
    HASH_NODE_PARAM_ADD(static_route, pageSize, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(static_route, vrf_name, TYPE_XML_STRING),
	
	{"", NULL, NULL, NULL, TYPE_XML_INT}
};

struct element_node_parameter rib_route_element_parameter[] = {
	HASH_NODE_PARAM_ADD(rib_table, type, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(rib_table, dst_ip, TYPE_XML_IPMASK),
	HASH_NODE_PARAM_ADD(rib_table, nh_ip, TYPE_XML_IP),
	HASH_NODE_PARAM_ADD(rib_table, oif, TYPE_XML_INTERFACE),
	HASH_NODE_PARAM_ADD(rib_table, distance, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(rib_table, weigh, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(rib_table, time, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(rib_table, status, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(rib_table, page, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(rib_table, pageSize, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(rib_table, vrf_name, TYPE_XML_STRING),
	
	{"", NULL, NULL, NULL, TYPE_XML_INT}
};

struct element_node_parameter default_gate_element_parameter[] = {
	HASH_NODE_PARAM_ADD(default_gate, gate_ip, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(default_gate, oif, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(default_gate, get_style, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(default_gate, weight, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(default_gate, distance, TYPE_XML_INT),
	{"", NULL, NULL, NULL, TYPE_XML_INT}
};

struct element_node_parameter rename_element_parameter[] = {
	HASH_NODE_PARAM_ADD(rename, old_name, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(rename, new_name, TYPE_XML_STRING),
	{"", NULL, NULL, NULL, TYPE_XML_INT}
};

struct element_node_parameter p2p_if_element_parameter[] = {
	HASH_NODE_PARAM_ADD(p2p_if, vrf_name, TYPE_XML_STRING),
	{"", NULL, NULL, NULL, TYPE_XML_INT}
};

struct element_node_parameter interface_status_element_parameter[] = {
	HASH_NODE_PARAM_ADD(interface_status, ifname, TYPE_XML_INTERFACE),
	HASH_NODE_PARAM_ADD(interface_status, shut, TYPE_XML_INT),
	{"", NULL, NULL, NULL, TYPE_XML_INT}
};

int interface_sub_logcontent(unsigned char log, void *ptr, char *Operate)
{
    struct interface_node* data = (struct interface_node*)ptr;
    switch(log)
    {
        case WEB_OPERATION_MOD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Mod interface_sub.");

        case WEB_OPERATION_ADD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Add interface_sub.");

        case WEB_OPERATION_DEL:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Del interface_sub.");

        default:
            return 0;
    }
}

struct module_node_parameter interface_node_module_parameter = {
	"interface_sub",
	xml_interface_node_add,
	xml_interface_node_del,
	xml_interface_node_mod,
	xml_interface_node_showall,
	xml_interface_node_showindex,
	xml_interface_node_showall,
	NULL,
	0,
	sizeof(struct interface_node),
	.func_logcontent = interface_sub_logcontent,
};

struct module_node_parameter ptp_if_module_parameter = {
	"ptp_if",
	NULL,
	NULL,
	NULL,
	NULL,
	xml_ptp_if_showindex,
	NULL,
	NULL,
	0,
	sizeof(struct interface_node),
};

struct module_node_parameter p2p_if_module_parameter = {
	"p2p_if",
	NULL,
	NULL,
	NULL,
	NULL,
	xml_p2p_if_showindex,
	NULL,
	NULL,
	0,
	sizeof(struct p2p_if),
};

struct module_node_parameter inf_select_module_parameter = {
	"inf_select",
	NULL,
	NULL,
	NULL,
	xml_inf_select_showindex,
	xml_inf_select_showindex,
	xml_inf_select_showindex,
	NULL,
	0,
	sizeof(struct interface_node),
};

int static_route_logcontent(unsigned char log, void *ptr, char *Operate)
{
    struct static_route* data = (struct static_route*)ptr;
    switch(log)
    {
        case WEB_OPERATION_MOD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Mod static_route.");

        case WEB_OPERATION_ADD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Add static_route.");

        case WEB_OPERATION_DEL:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Del static_route.");

        default:
            return 0;
    }
}

struct module_node_parameter static_route_module_parameter = {
	"static_route",
	xml_static_route_add,
	xml_static_route_del,
	xml_static_route_add,
	xml_static_route_show,
	NULL,
	xml_static_route_show,
	NULL,
	1,
	sizeof(struct static_route),
	.func_logcontent = static_route_logcontent,
};

struct module_node_parameter rib_route_module_parameter = {
	"rib_table",
	NULL,
	NULL,
	NULL,
	xml_rib_route_show,
	NULL,
	NULL,
	NULL,
	1,
	sizeof(struct rib_table),
};

int default_gate_logcontent(unsigned char log, void *ptr, char *Operate)
{
    struct default_gate* data = (struct default_gate*)ptr;
    switch(log)
    {
        case WEB_OPERATION_MOD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Mod default_gate.");

        case WEB_OPERATION_ADD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Add default_gate.");

        case WEB_OPERATION_DEL:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Del default_gate.");

        default:
            return 0;
    }
}

struct module_node_parameter default_gate_module_parameter = {
	"default_gate",
	xml_default_gate_add,
	xml_default_gate_del,
	NULL,
	xml_default_gate_show,
	NULL,
	NULL,
	NULL,
	1,
	sizeof(struct default_gate),
	.func_logcontent = default_gate_logcontent,
};

int rename_logcontent(unsigned char log, void *ptr, char *Operate)
{
    struct rename* data = (struct rename*)ptr;
    switch(log)
    {
        case WEB_OPERATION_MOD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Mod rename.");

        case WEB_OPERATION_ADD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Add rename.");

        case WEB_OPERATION_DEL:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Del rename.");

        default:
            return 0;
    }
}

struct module_node_parameter rename_module_parameter = {
	"rename",
	NULL,
	NULL,
	xml_rename_mod,
	NULL,
	NULL,
	NULL,
	NULL,
	1,
	sizeof(struct rename),
	.func_logcontent = rename_logcontent,
};

int interface_status_logcontent(unsigned char log, void *ptr, char *Operate)
{
    struct interface_status* data = (struct interface_status*)ptr;
    switch(log)
    {
        case WEB_OPERATION_MOD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Mod interface_status.");

        case WEB_OPERATION_ADD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Add interface_status.");

        case WEB_OPERATION_DEL:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Del interface_status.");

        default:
            return 0;
    }
}

struct module_node_parameter interface_status_module_parameter = {
	"interface_status",
	NULL,
	NULL,
	xml_interface_status_mod,
	NULL,
	NULL,
	NULL,
	NULL,
	0,
	sizeof(struct interface_status),
	.func_logcontent = interface_status_logcontent,
};


unsigned long xml_static_route_add(void *ptr)
{
	struct static_route *route_data= (struct static_route *)ptr;
	struct {
		struct oam_data_st oh;
		struct zebra_static_route_st data;
	}  cmd;
	memset(&cmd, 0, sizeof cmd);
	if(route_data->monitor_name[0] == '\0') {
		cmd.oh.cmd_code = ZEBRA_STATIC_ROUTE;
	} else {
		cmd.oh.cmd_code = ZEBRA_STATIC_ROUTE;
		strncpy(cmd.data.monitor_name, route_data->monitor_name, XML_MAX_NAME_LEN -1);
	}
	cmd.oh.mod_id = 0;
	strcpy(cmd.data.dest_str, route_data->dst_ip );
	if(route_data->nh_type == 0)
		strcpy(cmd.data.gate_str, route_data->nh_ip);
	else if(route_data->nh_type == 1)
		strcpy(cmd.data.gate_str, route_data->oif);
	cmd.data.distance = (int)route_data->distance;
	cmd.data.weight = route_data->weigh;
	cmd.data.add_cmd = 1;

#ifdef CONFIG_VSYS
    /*
	int vrf_id;
	vrf_id = dp_netns_get_vrfid_by_name(route_data->vrf_name);
	if(NETNS_ID_VAILD(vrf_id)){
		cmd.data.vrf_id = vrf_id;
	}
	*/
	cmd.data.vrf_id = GUI_CUR_VSYSID;
#endif

	write(client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd, sizeof cmd);

	while (1) {
		int nbytes;

		nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd, sizeof(cmd)-1);

		if (nbytes <= 0 && errno != EINTR) {
			return CMD_SUCCESS;
		}

		if (nbytes > 0) {
			if(cmd.oh.cmd_code != 0) {
				return cmd.oh.cmd_code;
			}
			break;
		}
	}
	return 0;
}

unsigned long xml_static_route_del(void *ptr)
{
	struct static_route *route_data= (struct static_route *)ptr;
	struct {
		struct oam_data_st oh;
		struct zebra_static_route_st data;
	}  cmd;

	memset(&cmd, 0, sizeof cmd);
	cmd.oh.cmd_code = ZEBRA_STATIC_ROUTE;
	cmd.oh.mod_id = 0;
	strcpy(cmd.data.dest_str, route_data->dst_ip );
	if(route_data->nh_type == 0)
		strcpy(cmd.data.gate_str, route_data->nh_ip);
	else if(route_data->nh_type == 1)
		strcpy(cmd.data.gate_str, route_data->oif);
	cmd.data.distance = (int)route_data->distance;
	cmd.data.add_cmd = 0;

#if 0
	int vrf_id;
	vrf_id = dp_netns_get_vrfid_by_name(route_data->vrf_name);
	if(NETNS_ID_VAILD(vrf_id)){
		cmd.data.vrf_id = vrf_id;
	}
#endif
    cmd.data.vrf_id = GUI_CUR_VSYSID;


	write(client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd, sizeof cmd);

	while (1) {
		int nbytes;

		nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd, sizeof(cmd)-1);

		if (nbytes <= 0 && errno != EINTR) {
			return CMD_SUCCESS;
		}

		if (nbytes > 0) {
			if(cmd.oh.cmd_code != 0) {
				return cmd.oh.cmd_code;
			}
			break;
		}
	}
	return 0;
}

char *xml_static_route_show(void *ptr)
{
	xmlNode *cur, *cur2;
	xmlNode *child;
	list list = NULL;
	listnode node = NULL;
	struct static_route *route_data = NULL;
	struct zebra_static_route_st *get_data = NULL;
	struct zebra_static_route_st *tmp_get_data = NULL;
	route_data = (struct static_route *)ptr;
	char * send_buf;
	xmlDoc *global_doc ;
	int count = 0;
	int sroute_counts = 0;
	int sroute_pages = 0;

	struct {
		struct oam_data_st oh;
		char buf[VTY_READ_BUFSIZ - 200];
	} cmd;

	memset(&cmd, 0, sizeof cmd);
	cmd.oh.cmd_code = ZEBRA_XML_STATIC_ROUTE;
	cmd.oh.mod_id = 0;

#if 0
	int vrf_id;
	vrf_id = dp_netns_get_vrfid_by_name(route_data->vrf_name);
	if(NETNS_ID_VAILD(vrf_id)){
		*((unsigned int *)cmd.buf) = vrf_id;
	}
#endif
    *((unsigned int *)cmd.buf) = GUI_CUR_VSYSID;

	write(client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd, sizeof cmd);

	list = list_new();
	while (1) {
		int nbytes = 0;
		memset(&cmd, 0 , sizeof cmd);
		nbytes = read(client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd,sizeof cmd);
		if (nbytes <= 0 && errno != EINTR) {
			return CMD_SUCCESS;
		}
		if (nbytes > 0) {
			if(cmd.oh.cmd_code == 2) {
				int tmp_len = 0;
				while (tmp_len < cmd.oh.datalen) {
#if 0 //#ifdef CONFIG_VSYS
					if( ((struct zebra_static_route_st *)(&(cmd.buf[tmp_len])))->vrf_id != vsysid ) {
						tmp_len += sizeof(struct zebra_static_route_st );
						continue;
					}
#endif
					get_data = XMALLOC(MTYPE_GUISH_ZEBRA_STATIC_ROUTE, sizeof(struct zebra_static_route_st ));
					memset(get_data, 0 , sizeof(struct zebra_static_route_st ));
					memcpy(get_data, &(cmd.buf[tmp_len]), sizeof(struct zebra_static_route_st ));
					tmp_len += sizeof(struct zebra_static_route_st );
					listnode_add(list, (void *)get_data);
				}
				continue;
			} else {
				break;
			}
		}
	}

    sroute_counts = listcount(list);
    if(route_data->pageSize > 0){
       sroute_pages = sroute_counts/route_data->pageSize+((sroute_counts%route_data->pageSize)?1:0);
    }else
        sroute_pages = 1;

	child = xml_start_more(&global_doc, "static_route");
	LIST_LOOP(list, tmp_get_data, node) {
		if (strchr(tmp_get_data->dest_str, ':'))
			continue;

		char buf[10];
		struct in_addr gate;
		int ret;
		if(strlen(route_data->dst_ip)) {
			if(strncmp(route_data->dst_ip, tmp_get_data->dest_str, XML_MAX_IP_LEN))
				continue;
		}
		if(strlen(route_data->nh_ip)) {
			if(strncmp(route_data->nh_ip, tmp_get_data->gate_str, XML_MAX_IP_LEN))
				continue;
		}

		if(strlen(route_data->oif)) {
			if(strncmp(route_data->oif, tmp_get_data->gate_str, IFNAMSIZ))
				continue;
		}


		count++;
		if ((route_data->page > 0)  && (route_data->pageSize) > 0) {
			if (count <= (route_data->page - 1) * route_data->pageSize)
				continue;
			if (count > route_data->page * route_data->pageSize)
				break;
		}



		//if (count++ > 1000)
		//	break;

		cur = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );
#ifdef CONFIG_VSYS		
		vsos_xmlNewChild( cur, NULL, BAD_CAST"vrf_name", BAD_CAST route_data->vrf_name);
#endif
		vsos_xmlNewChild( cur, NULL, BAD_CAST"dst_ip", BAD_CAST tmp_get_data->dest_str);
		ret = inet_aton (tmp_get_data->gate_str, &gate);
		if (ret) {
			vsos_xmlNewChild( cur, NULL, BAD_CAST"nh_type", BAD_CAST "0");
			vsos_xmlNewChild( cur, NULL, BAD_CAST"nh_ip", BAD_CAST tmp_get_data->gate_str);
		} else {
			vsos_xmlNewChild( cur, NULL, BAD_CAST"nh_type", BAD_CAST "1");
			vsos_xmlNewChild( cur, NULL, BAD_CAST"oif", BAD_CAST tmp_get_data->gate_str);
		}
		memset(buf, 0, 10);
		snprintf(buf, 9, "%d", tmp_get_data->distance);
		vsos_xmlNewChild( cur, NULL, BAD_CAST"distance", BAD_CAST buf);
		memset(buf, 0, 10);
		snprintf(buf, 9, "%d", tmp_get_data->weight);
		vsos_xmlNewChild( cur, NULL, BAD_CAST"weigh", BAD_CAST buf);
		vsos_xmlNewChild( cur, NULL, BAD_CAST"monitor_name", BAD_CAST tmp_get_data->monitor_name);

	}

	/* add page element */
	char tmp_buf[16];
	cur2 = vsos_xmlNewChild(child, NULL, BAD_CAST"page", BAD_CAST NULL );
	memset(tmp_buf, 0, 16);
	snprintf(tmp_buf, 15, "%d", sroute_counts);
	vsos_xmlNewChild(cur2, NULL, BAD_CAST"total", BAD_CAST tmp_buf);

	memset(tmp_buf, 0, 16);
	snprintf(tmp_buf, 15, "%d", route_data->pageSize);
	vsos_xmlNewChild(cur2, NULL, BAD_CAST"count", BAD_CAST tmp_buf);
	memset(tmp_buf, 0, 16);
	snprintf(tmp_buf, 15, "%d", route_data->page);
	vsos_xmlNewChild(cur2, NULL, BAD_CAST"current", BAD_CAST tmp_buf);

	/* add page attribute */
	memset(tmp_buf, 0, 16);
	snprintf(tmp_buf, 15, "%d", sroute_pages);
	xmlNewProp(child , BAD_CAST "page", BAD_CAST tmp_buf);
	memset(tmp_buf, 0, 16);
	snprintf(tmp_buf, 15, "%d", route_data->pageSize);
	xmlNewProp(child , BAD_CAST "count", BAD_CAST tmp_buf);
	memset(tmp_buf, 0, 16);
	snprintf(tmp_buf, 15, "%d", sroute_counts);
	xmlNewProp(child , BAD_CAST "total", BAD_CAST tmp_buf);
	/* end */

	send_buf = xml_end(global_doc, ZEBRA_SAVE_TMP);

	LIST_LOOP(list, get_data, node) {
		XFREE(MTYPE_GUISH_ZEBRA_STATIC_ROUTE, get_data);
	}
	list_delete(list);
	return send_buf;
}



unsigned long xml_static_mroute_add(void *ptr)
{
	struct static_mroute_t *mroute_data= (struct static_mroute_t *)ptr;
	struct out_interfaces *out_interface = NULL;
	int i = 0;
	struct {
		struct oam_data_st oh;
		struct zebra_static_mroute_st data;
	} cmd;

	out_interface = mroute_data->out_interface;

	while(out_interface) {
		memset(&cmd, 0, sizeof cmd);
		cmd.oh.cmd_code = ZEBRA_STATIC_MROUTE;
		cmd.oh.mod_id = 0;
		strcpy(cmd.data.dest_str, mroute_data->multi_group_ip);
		if (i == 0)
			cmd.data.cmd = 3;
		else
			cmd.data.cmd = 1;

		strncpy(cmd.data.gate_str, out_interface->ifname, 7);
		write(client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd, sizeof(cmd));

		while (1) {
			int nbytes;

			nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd, sizeof(cmd));

			if (nbytes <= 0 && errno != EINTR) {
				return 3;
			}

			if (nbytes > 0) {
				if(cmd.oh.cmd_code != 0) {
					return cmd.oh.cmd_code;
				}
				break;
			}
		}
		out_interface = out_interface->next;
		i++;
	}
	return 0;
}

unsigned long xml_static_mroute_mod(void *ptr)
{
	struct static_mroute_t *mroute_data= (struct static_mroute_t *)ptr;
	struct out_interfaces *out_interface = NULL;
	struct {
		struct oam_data_st oh;
		struct zebra_static_mroute_st data;
	} cmd;

	memset(&cmd, 0, sizeof cmd);
	cmd.oh.cmd_code = ZEBRA_STATIC_MROUTE;
	cmd.oh.mod_id = 0;
	strcpy(cmd.data.dest_str, mroute_data->multi_group_ip);
	out_interface = mroute_data->out_interface;

	// ??ɾ??????·??
	cmd.data.cmd = 2;
	write(client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd, sizeof(cmd));
	while (1) {
		int nbytes;

		nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd, sizeof(cmd));

		if (nbytes <= 0 && errno != EINTR) {
			return 3;
		}

		if (nbytes > 0) {
			if(cmd.oh.cmd_code != 0) {
				return cmd.oh.cmd_code;
			}
			break;
		}
	}

	// ?????Ӹ????ӿ?
	while(out_interface) {
		memset(&cmd, 0, sizeof cmd);
		cmd.oh.cmd_code = ZEBRA_STATIC_MROUTE;
		cmd.oh.mod_id = 0;
		strcpy(cmd.data.dest_str, mroute_data->multi_group_ip);
		cmd.data.cmd = 1;
		strncpy(cmd.data.gate_str, out_interface->ifname, 7);
		write(client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd, sizeof(cmd));

		while (1) {
			int nbytes;

			nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd, sizeof(cmd));

			if (nbytes <= 0 && errno != EINTR) {
				return 3;
			}

			if (nbytes > 0) {
				if(cmd.oh.cmd_code != 0) {
					return cmd.oh.cmd_code;
				}
				break;
			}
		}
		out_interface = out_interface->next;
	}
	return 0;
}

unsigned long xml_static_mroute_del(void *ptr)
{
	struct static_mroute_t *mroute_data= (struct static_mroute_t *)ptr;
	struct {
		struct oam_data_st oh;
		struct zebra_static_mroute_st data;
	} cmd;

	memset(&cmd, 0, sizeof cmd);
	cmd.oh.cmd_code = ZEBRA_STATIC_MROUTE;
	cmd.oh.mod_id = 0;
	strcpy(cmd.data.dest_str, mroute_data->multi_group_ip);
	cmd.data.cmd = 2;

	write(client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd, sizeof(cmd));

	while (1) {
		int nbytes;

		nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd, sizeof(cmd));

		if (nbytes <= 0 && errno != EINTR) {
			return 3;
		}

		if (nbytes > 0) {
			if(cmd.oh.cmd_code != 0) {
				return cmd.oh.cmd_code;
			}
			break;
		}
	}

	return 0;
}

char * xml_static_mroute_showall(void *ptr)
{
	xmlNode *cur, *cur1, *cur2;
	xmlNode *child;
	char * send_buf;
	assert (NULL != ptr);
	struct static_mroute_t *data = ptr;

	struct {
		struct oam_data_st oh;
		struct mroute_entry data;
	}  buf;
	xmlDoc *global_doc ;

	memset(&buf, 0, sizeof buf);
	buf.oh.cmd_code = ZEBRA_MROUTE_SHOW;
	buf.oh.mod_id = VTYSH_INDEX_ZEBRA;
	write(client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof buf);

	child = xml_start_more(&global_doc, "static_mroute");
	while (1) {
		int nbytes;

		nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof(buf));

		if (nbytes <= 0 && errno != EINTR) {
			return NULL;
		}

		if (nbytes > 0) {
			if(buf.oh.cmd_code == 2) {
				if(strlen(data->multi_group_ip)) {
					if(strcmp(data->multi_group_ip, inet_ntoa(buf.data.multi_addr)))
						continue;
				}
				cur = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );
				vsos_xmlNewChild( cur, NULL, BAD_CAST"multi_group_ip", BAD_CAST inet_ntoa(buf.data.multi_addr));
				//vsos_xmlNewChild( cur, NULL, BAD_CAST"out_interface", BAD_CAST (buf.data.status ? "1" : "0"));

				{
					int i = 0;
					char aliasname[64] = {0};
					for (i = 0; i < buf.data.out_free; i++) {
						if(i == 0) {
							cur1 = vsos_xmlNewChild( cur, NULL, BAD_CAST"out_interface", BAD_CAST NULL );
						}

						cur2 = vsos_xmlNewChild( cur1, NULL, BAD_CAST"group", BAD_CAST NULL );
						if_get_alias_by_name(buf.data.out_interfaces[i], aliasname);
						vsos_xmlNewChild( cur2, NULL, BAD_CAST"ifname", BAD_CAST aliasname);
					}

				}
				continue;
			} else {
				break;
			}
		}
	}
	send_buf = xml_end(global_doc, ZEBRA_SAVE_TMP);
	return send_buf;
}




unsigned long xml_default_gate_add(void *ptr)
{
	struct default_gate *route_data= (struct default_gate *)ptr;
	struct {
		struct oam_data_st oh;
		struct zebra_static_route_st data;
	}  cmd;

	memset(&cmd, 0, sizeof cmd);
	cmd.oh.cmd_code = ZEBRA_STATIC_ROUTE;
	cmd.oh.mod_id = 0;
	strcpy(cmd.data.dest_str, "0.0.0.0/0" );
	strcpy(cmd.data.gate_str, route_data->gate_ip);
	cmd.data.distance = route_data->distance;
	cmd.data.weight= route_data->weight;
	cmd.data.add_cmd = 1;
#if 0 //#ifdef CONFIG_VSYS
	if(xml_vsys_gui_get_current_vsysid(&cmd.data.vrf_id) < 0 )
		return 535;
#endif
	write(client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd, sizeof cmd);

	while (1) {
		int nbytes;

		nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd, sizeof(cmd)-1);

		if (nbytes <= 0 && errno != EINTR) {
			return CMD_SUCCESS;
		}

		if (nbytes > 0) {
			if(cmd.oh.cmd_code != 0) {
				if(cmd.oh.cmd_code == 40)
					return 45;//for bug 18254
				return cmd.oh.cmd_code;
			}
			break;
		}
	}
	return 0;
}

unsigned long xml_default_gate_del(void *ptr)
{
	struct default_gate *route_data= (struct default_gate *)ptr;
	struct {
		struct oam_data_st oh;
		struct zebra_static_route_st data;
	}  cmd;

	memset(&cmd, 0, sizeof cmd);
	cmd.oh.cmd_code = ZEBRA_STATIC_ROUTE;
	cmd.oh.mod_id = 0;
	strcpy(cmd.data.dest_str, "0.0.0.0/0" );


	if (route_data->gate_ip[0] == '\0') {
		strncpy(cmd.data.gate_str, route_data->oif, 63);
	} else {
		char *pnt;
		pnt = strchr (route_data->gate_ip, ' ');
		if(pnt)
			*pnt = 0;
		strcpy(cmd.data.gate_str, route_data->gate_ip);
	}
	cmd.data.add_cmd = 0;
#if 0 //#ifdef CONFIG_VSYS
	if(xml_vsys_gui_get_current_vsysid(&cmd.data.vrf_id) < 0 )
		return 535;
#endif
	write(client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd, sizeof cmd);

	while (1) {
		int nbytes;

		nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd, sizeof(cmd)-1);

		if (nbytes <= 0 && errno != EINTR) {
			return CMD_SUCCESS;
		}

		if (nbytes > 0) {
			if(cmd.oh.cmd_code != 0) {
				return cmd.oh.cmd_code;
			}
			break;
		}
	}
	return 0;
}

char *xml_default_gate_show(void *ptr)
{
	xmlNode *cur;
	xmlNode *child;
	char * send_buf;
	struct {
		struct oam_data_st oh;
		char buf[VTY_READ_BUFSIZ - 200];
	} cmd;
	xmlDoc *global_doc ;

	memset(&cmd, 0, sizeof cmd);
	cmd.oh.cmd_code = ZEBRA_DUMP_IP_ROUTE_TABLE;
	cmd.oh.mod_id = 0;
#if 0 //#ifdef CONFIG_VSYS
	if(xml_vsys_gui_get_current_vsysid((unsigned int *)(cmd.buf)) < 0 ) {
		vsos_debug_out("xml_default_gate_show: get vsysid failed\r\n");
		return NULL;
	}
#endif
	write(client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd, sizeof cmd);

	child = xml_start_more(&global_doc, "default_gate");
	while (1) {
		int nbytes = 0;
		memset(&cmd, 0 , sizeof cmd);
		nbytes = read(client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd,sizeof(struct oam_data_st)+sizeof(struct zebra_ip_route_st));
		if (nbytes <= 0 && errno != EINTR) {
			break;
		}
		if (nbytes > 0) {
			if(cmd.oh.cmd_code == 2) {
				if(nbytes >= sizeof(struct oam_data_st)) {
					int offset = 0;
					while(nbytes-sizeof(struct oam_data_st) > offset) {
						char buf[50];
						int i;
						struct zebra_ip_route_st * rib_st = (struct zebra_ip_route_st *)(cmd.buf+offset);
						if(rib_st->p.u.prefix4.s_addr == 0) {
							for (i = 0; i < rib_st->nexthop_num; i++) {
								cur = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );
								switch (rib_st->nexthop[i].type) {
								case NEXTHOP_TYPE_IPV4:
								case NEXTHOP_TYPE_IPV4_IFINDEX:
									sprintf (buf, "%s",inet_ntoa (rib_st->nexthop[i].gate.ipv4));
									vsos_xmlNewChild( cur, NULL, BAD_CAST"gate_ip", BAD_CAST buf);
									if (strlen(rib_st->nexthop[i].ifname))
										vsos_xmlNewChild( cur, NULL, BAD_CAST"oif", BAD_CAST rib_st->nexthop[i].ifname);
									break;
								case NEXTHOP_TYPE_IFINDEX:
								case NEXTHOP_TYPE_IFNAME:
									sprintf (buf, "%s",rib_st->nexthop[i].ifname);
									vsos_xmlNewChild( cur, NULL, BAD_CAST"oif", BAD_CAST buf);
									break;
								case NEXTHOP_TYPE_BLACKHOLE:
									sprintf (buf, "is directly connected, Null0");
									vsos_xmlNewChild( cur, NULL, BAD_CAST"oif", BAD_CAST buf);
									break;
								default:
									sprintf (buf, "unknown");
									vsos_xmlNewChild( cur, NULL, BAD_CAST"oif", BAD_CAST buf);
								}

								if(rib_st->type == ZEBRA_ROUTE_STATIC)
									vsos_xmlNewChild( cur, NULL, BAD_CAST"get_style", BAD_CAST "0");
								else if(rib_st->type == ZEBRA_ROUTE_PPPOE)
									vsos_xmlNewChild( cur, NULL, BAD_CAST"get_style", BAD_CAST "1");
								else if(rib_st->type == ZEBRA_ROUTE_DHCP)
									vsos_xmlNewChild( cur, NULL, BAD_CAST"get_style", BAD_CAST "2");
								else if(rib_st->type == ZEBRA_ROUTE_RIP)
									vsos_xmlNewChild( cur, NULL, BAD_CAST"get_style", BAD_CAST "3");
								else if(rib_st->type == ZEBRA_ROUTE_OSPF)
									vsos_xmlNewChild( cur, NULL, BAD_CAST"get_style", BAD_CAST "4");
								buf[0] = 0;
								snprintf(buf, 9, "%d", rib_st->distance);
								vsos_xmlNewChild( cur, NULL, BAD_CAST"distance", BAD_CAST buf);
								buf[0] = 0;
								snprintf(buf, 9, "%d", rib_st->nexthop[i].weight);
								vsos_xmlNewChild( cur, NULL, BAD_CAST"weight", BAD_CAST buf);

							}
						}
						offset = offset + sizeof(struct zebra_ip_route_st );
					}
				}
				continue;
			} else {
				break;
			}
		}
	}
	send_buf = xml_end(global_doc, ZEBRA_SAVE_TMP);

	return send_buf;
}

unsigned long xml_rename_mod(void *ptr)
{
	int ret = 0;
	struct rename *data= (struct rename *)ptr;
	if(if_check_aliasname_by_rules(data->new_name))
		return 434;
	char realname[100];
	if_get_name_by_alias(data->old_name, realname);
	if(strcmp(realname, data->new_name)) {
		ret = if_check_aliasname(data->new_name);
		if(ret == 1)
			return 84;
		else if(ret == 2)
			return 85;
		else if(ret == 3)
			return 130;
	}
	if_rename(data->old_name, data->new_name);
	return 0;
}

unsigned long xml_interface_status_mod(void *ptr)
{
	struct interface_status *data= (struct interface_status *)ptr;
	int ret = check_vm_mode();

	if(data->shut) {
		if(ret && !strncmp(data->ifname, "mgt", 3)) {
			return 0;
		}
		if_shutdown(data->ifname);
	} else {
		if_no_shutdown(data->ifname);
	}
	return 0;
}


char *xml_rib_route_show(void *ptr)
{
	xmlNode *cur, *cur2;
	xmlNode *child;
	char * send_buf;
	list list = NULL;
	listnode node = NULL;
	struct rib_table *rtable = (struct rib_table *)ptr;
	struct zebra_ip_route_st *get_data = NULL;
	struct zebra_ip_route_st *tmp_get_data = NULL;
    struct zebra_ip_route_st * rib_st  = NULL;

	struct {
		struct oam_data_st oh;
		char buf[VTY_READ_BUFSIZ - 200];
	} cmd;
	xmlDoc *global_doc ;
	int count = 0, total = 0;
	int rtable_counts = 0;
	int rtable_pages = 0;
    int flag = 0;

	memset(&cmd, 0, sizeof cmd);
	cmd.oh.cmd_code = ZEBRA_DUMP_IP_ROUTE_TABLE;
	cmd.oh.mod_id = 0;

#if 0
	int vrf_id;
	vrf_id = dp_netns_get_vrfid_by_name(rtable->vrf_name);
	if(NETNS_ID_VAILD(vrf_id)){
		*((unsigned int *)cmd.buf) = vrf_id;
	}
#endif
    *((unsigned int *)cmd.buf) = GUI_CUR_VSYSID;

	write(client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd, sizeof cmd);

    list = list_new();
    while(1){
        int nbytes = 0;
        memset(&cmd, 0 , sizeof cmd);
		nbytes = read(client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd,sizeof(struct oam_data_st)+sizeof(struct zebra_ip_route_st));
        if (nbytes <= 0 && errno != EINTR) {
            break;
        }
        if (nbytes > 0) {
            if(cmd.oh.cmd_code == 2) {
                if(nbytes >= sizeof(struct oam_data_st)){
                    int offset = 0;
                    int i = 0;
                    while (nbytes-sizeof(struct oam_data_st) > offset) {
                        struct zebra_ip_route_st * rib_st = (struct zebra_ip_route_st *)(cmd.buf+offset);
                        for (i = 0; i < rib_st->nexthop_num; i++) {
                            flag = 0;
                            if(rtable->type > 0) {
                                if(rib_st->type != rtable->type){
                                    flag = 1;
                                    continue;
                                }
                            }
                            if(strlen(rtable->nh_ip) != 0) {
                                u32 nexthop = ip_addr(rtable->nh_ip);
                                if(rib_st->nexthop[i].gate.ipv4.s_addr != nexthop){
                                    flag = 1;
                                    continue;
                                }
                            }
                            if(strlen(rtable->dst_ip) != 0) {
                                struct prefix p;
                                str2prefix(rtable->dst_ip, &p);
                                if(!prefix_match(&p, (struct prefix *)&rib_st->p)){
                                    flag = 1;
                                    continue;
                                }
                            }

                            if ( (rib_st->type == ZEBRA_ROUTE_OSPF || rib_st->type == ZEBRA_ROUTE_OSPF6) &&
                                    (rib_st->nexthop[i].type == NEXTHOP_TYPE_IFINDEX || rib_st->nexthop[i].type == NEXTHOP_TYPE_IFNAME || rib_st->nexthop[i].type == NEXTHOP_TYPE_BLACKHOLE))
                            {
                                flag = 1;
                                continue;
                            }

                            if(!flag){
                                total++;
                            }

						}
						get_data = XMALLOC(MTYPE_GUISH_ZEBRA_STATIC_ROUTE, sizeof(struct zebra_ip_route_st ));
                        memset(get_data, 0 , sizeof(struct zebra_ip_route_st ));
                        memcpy(get_data, rib_st, sizeof(struct zebra_ip_route_st ));
                        listnode_add(list, (void *)get_data);
						offset = offset + sizeof(struct zebra_ip_route_st );
                    }
                }
                continue;
            } else {
                break;
            }
        }
    }

	rtable_counts = total;
    if(rtable->pageSize > 0){
       rtable_pages = rtable_counts/rtable->pageSize+((rtable_counts%rtable->pageSize)?1:0);
    }else
        rtable_pages = 1;


	child = xml_start_more(&global_doc, "rib_table");
	LIST_LOOP(list, tmp_get_data, node){
		char buf[50];
		int i;
		for (i = 0; i < tmp_get_data->nexthop_num; i++) {
		    flag = 0;
			if(rtable->type > 0) {
				if(tmp_get_data->type != rtable->type)
					continue;
			}
			if(strlen(rtable->nh_ip) != 0) {
				u32 nexthop = ip_addr(rtable->nh_ip);
				if(tmp_get_data->nexthop[i].gate.ipv4.s_addr != nexthop)
					continue;
			}
			if(strlen(rtable->dst_ip) != 0) {
				struct prefix p;
				str2prefix(rtable->dst_ip, &p);
				if(!prefix_match(&p, (struct prefix *)&tmp_get_data->p))
					continue;
			}

			if ( (tmp_get_data->type == ZEBRA_ROUTE_OSPF || tmp_get_data->type == ZEBRA_ROUTE_OSPF6) &&
					(tmp_get_data->nexthop[i].type == NEXTHOP_TYPE_IFINDEX || tmp_get_data->nexthop[i].type == NEXTHOP_TYPE_IFNAME || tmp_get_data->nexthop[i].type == NEXTHOP_TYPE_BLACKHOLE))
				continue;

            count++;
            if ((rtable->page > 0)  && (rtable->pageSize) > 0) {
                if (count <= (rtable->page - 1) * rtable->pageSize)
                    continue;
                if (count > rtable->page * rtable->pageSize){
                    flag = 1;
                    break;
                }
            }

			cur = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );
			buf[0] = 0;
			snprintf(buf, 9, "%d", tmp_get_data->type);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"type", BAD_CAST buf);
			prefix2str((struct prefix *)&tmp_get_data->p, buf, 50);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"dst_ip", BAD_CAST buf);
			switch (tmp_get_data->nexthop[i].type) {
			case NEXTHOP_TYPE_IPV4:
			case NEXTHOP_TYPE_IPV4_IFINDEX:
				vsos_xmlNewChild( cur, NULL, BAD_CAST"nh_ip", BAD_CAST inet_ntoa (tmp_get_data->nexthop[i].gate.ipv4));
				if (strlen(tmp_get_data->nexthop[i].ifname))
					vsos_xmlNewChild( cur, NULL, BAD_CAST"oif", BAD_CAST tmp_get_data->nexthop[i].ifname);
				break;
			case NEXTHOP_TYPE_IFINDEX:
			case NEXTHOP_TYPE_IFNAME:
				vsos_xmlNewChild( cur, NULL, BAD_CAST"oif", BAD_CAST tmp_get_data->nexthop[i].ifname);
				break;
			case NEXTHOP_TYPE_BLACKHOLE:
				vsos_xmlNewChild( cur, NULL, BAD_CAST"oif", BAD_CAST "is directly connected, Null0");
				break;
			default:
				vsos_xmlNewChild( cur, NULL, BAD_CAST"oif", BAD_CAST "unknown");
			}

			buf[0] = 0;
			snprintf(buf, 9, "%d", tmp_get_data->distance);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"distance", BAD_CAST buf);
			buf[0] = 0;
			snprintf(buf, 9, "%d", tmp_get_data->nexthop[i].weight);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"weigh", BAD_CAST buf);
			{
				time_t uptime;
				struct tm *tm;

				uptime = time (NULL);
				uptime -= tmp_get_data->uptime;
				tm = gmtime (&uptime);

#define ONE_DAY_SECOND 60*60*24
#define ONE_WEEK_SECOND 60*60*24*7

				if (uptime < ONE_DAY_SECOND)
					sprintf (buf, "%02d:%02d:%02d",
							 tm->tm_hour, tm->tm_min, tm->tm_sec);
				else if (uptime < ONE_WEEK_SECOND)
					sprintf (buf,  "%dd%02dh%02dm",
							 tm->tm_yday, tm->tm_hour, tm->tm_min);
				else
					sprintf (buf, "%02dw%dd%02dh",
							 tm->tm_yday/7,
							 tm->tm_yday - ((tm->tm_yday/7) * 7), tm->tm_hour);
			}
			vsos_xmlNewChild( cur, NULL, BAD_CAST"time", BAD_CAST buf);
			if( CHECK_FLAG (tmp_get_data->flags, ZEBRA_FLAG_SELECTED) &&  CHECK_FLAG (tmp_get_data->nexthop[i].flags, NEXTHOP_FLAG_ACTIVE))
				vsos_xmlNewChild( cur, NULL, BAD_CAST"status", BAD_CAST "0");
			else
				vsos_xmlNewChild( cur, NULL, BAD_CAST"status", BAD_CAST "1");
		}
        if(flag)
            break;
	}

	/* add page element */
	char tmp_buf[16];//show
	cur2 = vsos_xmlNewChild(child, NULL, BAD_CAST"page", BAD_CAST NULL );
	memset(tmp_buf, 0, 16);
	snprintf(tmp_buf, 15, "%d", rtable_counts);
	vsos_xmlNewChild(cur2, NULL, BAD_CAST"total", BAD_CAST tmp_buf);

	memset(tmp_buf, 0, 16);
	snprintf(tmp_buf, 15, "%d", rtable->pageSize);
	vsos_xmlNewChild(cur2, NULL, BAD_CAST"count", BAD_CAST tmp_buf);
	memset(tmp_buf, 0, 16);
	snprintf(tmp_buf, 15, "%d", rtable->page);
	vsos_xmlNewChild(cur2, NULL, BAD_CAST"current", BAD_CAST tmp_buf);

	/* add page attribute *///tou
	memset(tmp_buf, 0, 16);
	snprintf(tmp_buf, 15, "%d", rtable_pages);
	xmlNewProp(child , BAD_CAST "page", BAD_CAST tmp_buf);
	memset(tmp_buf, 0, 16);
	snprintf(tmp_buf, 15, "%d", rtable->pageSize);
	xmlNewProp(child , BAD_CAST "count", BAD_CAST tmp_buf);
	memset(tmp_buf, 0, 16);
	snprintf(tmp_buf, 15, "%d", rtable_counts);
	xmlNewProp(child , BAD_CAST "total", BAD_CAST tmp_buf);
	/* end */
	send_buf = xml_end(global_doc, ZEBRA_SAVE_TMP);
	LIST_LOOP(list, get_data, node) {
		XFREE(MTYPE_GUISH_ZEBRA_STATIC_ROUTE, get_data);
	}
	list_delete(list);

	return send_buf;
}

#if 0
int xml_eth_if_config_set(char *interface_name, struct eth_if_config *config)
{
#ifdef OLD_PLAT
	struct eth_if_config current_config;

	if(config->eth_if_config_type <= ETHIF_CFG_DUPLEX) {
		memset(&current_config, 0, sizeof(struct eth_if_config));
		current_config.eth_if_config_type = ETHIF_CFG_ALL;
		strncpy(current_config.eth_if_name, interface_name, IFNAMSIZ-1);

		if((kernel_request(CTRL_ETHIF_CFG_GET, (char *)&current_config, sizeof(struct eth_if_config), sizeof(struct eth_if_config), NLM_F_ACK))<0) {
			vsos_debug_out("Fail to get current config from kernel.\r\n");
			return 1;
		}
	}

	/*vty_out(vty, "Vthsh: config get: auto %d speed %d duplex %d%s", current_config.eth_if_config_value[ETHIF_CFG_AUTO_OFFSET], current_config.eth_if_config_value[ETHIF_CFG_SPEED_OFFSET], current_config.eth_if_config_value[ETHIF_CFG_DUPLEX_OFFSET], VTY_NEWLINE);*/

	if(((config->eth_if_config_type == ETHIF_CFG_DUPLEX)||(config->eth_if_config_type == ETHIF_CFG_SPEED)) && \
			(current_config.eth_if_config_value[ETHIF_CFG_AUTO_OFFSET] == AUTO)) {
		//vty_out(vty, "Cannot change duplex or speed parameter when auto-negotiate is on.%s", VTY_NEWLINE);
		return 1;
	}

	if(((config->eth_if_config_type == ETHIF_CFG_DUPLEX)||(config->eth_if_config_type == ETHIF_CFG_SPEED)||(config->eth_if_config_type == ETHIF_CFG_AUTO)) && \
			(current_config.eth_if_config_value[ETHIF_CFG_MEDIA_TYPE_OFFSET] == FIBER)) {
		return 1;
	}

	if((config->eth_if_config_type == ETHIF_CFG_AUTO) && \
			(config->eth_if_config_value[ETHIF_CFG_AUTO_OFFSET] == current_config.eth_if_config_value[ETHIF_CFG_AUTO_OFFSET])) {
#if 0
		if(config->eth_if_config_value[ETHIF_CFG_AUTO_OFFSET] == AUTO)
			vty_out(vty, "Auto negotiate is already on!%s", VTY_NEWLINE);
		if(config->eth_if_config_value[ETHIF_CFG_AUTO_OFFSET] == NO_AUTO)
			vty_out(vty, "Auto negotiate is already off!%s", VTY_NEWLINE);
#endif
		return 1;
	}

	if((config->eth_if_config_type == ETHIF_CFG_SPEED) && \
			(config->eth_if_config_value[ETHIF_CFG_SPEED_OFFSET] == current_config.eth_if_config_value[ETHIF_CFG_SPEED_OFFSET])) {
#if 0
		if(config->eth_if_config_value[ETHIF_CFG_SPEED_OFFSET] == SPEED10)
			vty_out(vty, "Speed is already 10!%s", VTY_NEWLINE);
		if(config->eth_if_config_value[ETHIF_CFG_SPEED_OFFSET] == SPEED100)
			vty_out(vty, "Speed is already 100!%s", VTY_NEWLINE);
		if(config->eth_if_config_value[ETHIF_CFG_SPEED_OFFSET] == SPEED1000)
			vty_out(vty, "Speed is already 1000!%s", VTY_NEWLINE);
#endif
		return 1;
	}

	if((config->eth_if_config_type == ETHIF_CFG_DUPLEX) && \
			(config->eth_if_config_value[ETHIF_CFG_DUPLEX_OFFSET] == current_config.eth_if_config_value[ETHIF_CFG_DUPLEX_OFFSET])) {
#if 0
		if(config->eth_if_config_value[ETHIF_CFG_DUPLEX_OFFSET] == DUPLEX)
			vty_out(vty, "Duplex is already full!%s", VTY_NEWLINE);
		if(config->eth_if_config_value[ETHIF_CFG_DUPLEX_OFFSET] == HAFLEX)
			vty_out(vty, "Duplex negotiate is already half!%s", VTY_NEWLINE);
#endif
		return 1;
	}

	if((config->eth_if_config_type == ETHIF_CFG_SPEED) && (current_config.eth_if_config_value[ETHIF_CFG_SPEED_MAX_OFFSET] != SPEED1000)&&(config->eth_if_config_value[ETHIF_CFG_SPEED_OFFSET] == SPEED1000)) {
		//vty_out(vty, "Speed 1000 Mbps is not supported by %s%s", (char *)vty->index, VTY_NEWLINE);
		return 1;
	}

	if(((config->eth_if_config_type == ETHIF_CFG_SPEED) && \
			(current_config.eth_if_config_value[ETHIF_CFG_DUPLEX_OFFSET] == HAFLEX) && \
			(config->eth_if_config_value[ETHIF_CFG_SPEED_OFFSET] == SPEED1000)) || \
			((config->eth_if_config_type == ETHIF_CFG_DUPLEX) && \
			 (current_config.eth_if_config_value[ETHIF_CFG_SPEED_OFFSET] == SPEED1000) && \
			 (config->eth_if_config_value[ETHIF_CFG_DUPLEX_OFFSET] == HAFLEX))) {
		//vty_out(vty, "Invalid parameter: speed 1000 /duplex half %s", VTY_NEWLINE);
		return 1;
	}


	if(kernel_request(CTRL_ETHIF_CFG_SET, (char *)config, sizeof(struct eth_if_config), sizeof(struct eth_if_config), NLM_F_ACK)) {
		vsos_debug_out("Fail to set config from kernel.\r\n");
		return 1;
	}
#endif

	return 0;

}
#endif


int xml_interface_get_ipcfg(char *ifname, int type, void *ipcfg)
{
	if(type == 2) { /*dhcp ip config*/
		struct if_dhcp_config *dhcp_ipcfg = (struct if_dhcp_config *)ipcfg;

		struct {
			struct oam_data_st oh;
			char sbuf[VTY_READ_BUFSIZ];
		}  cmd;

		memset(&cmd, 0, sizeof cmd);
		cmd.oh.cmd_code = DHCP_IF_CONFIG_DUMP;
		cmd.oh.mod_id = VTYSH_INDEX_DHCP;
		strncpy(cmd.sbuf, ifname, IFNAMSIZ);
		write(client_xml[VTYSH_INDEX_DHCP].fd, &cmd, sizeof (struct oam_data_st) + IFNAMSIZ);

		while (1) {
			int nbytes;
			nbytes = read (client_xml[VTYSH_INDEX_DHCP].fd, &cmd, sizeof(struct oam_data_st)+sizeof(struct if_dhcp_config));

			if (nbytes <= 0 && errno != EINTR) {
				break;
			}

			if (nbytes > 0) {
				if(cmd.oh.cmd_code == DHCP_IF_CONFIG_DUMP) {
					if(nbytes >= sizeof(struct oam_data_st)) {
						cmd.sbuf[nbytes - sizeof(struct oam_data_st)] = 0;
						struct if_dhcp_config *ifd = (struct if_dhcp_config *)cmd.sbuf;
						if (ifd->flags & DHCPC_CFLAG_ENABLE) {
							dhcp_ipcfg->metric = ifd->metric;
							dhcp_ipcfg->flags = ifd->flags;
						}
					}
					continue;
				}
				break;
			}
		}
	}

	return 0;
}

unsigned long xml_interface_node_add(void *ptr)
{
#if 0
	assert (NULL != ptr);
	struct interface_node *if_node = ptr;
#if 0 //#ifdef CONFIG_VSYS
	unsigned int vsysid = 0;
	enum vsys_ui_type ui_type;

	if( xml_vsys_gui_get_current_uitype(&ui_type) < 0 ) {
		vsos_debug_out("xml_interface_node_showall: get vsys_ui_type failed\r\n");
		return 535;
	}
#endif

	if(if_check_aliasname_by_rules(if_node->name))
		return 434;
	/*handle vlan add*/
	if(if_node->type == 1) {
		int ret;
		char realname[100];
		ret = if_check_aliasname(if_node->name);
		if(ret == 1)
			return 84;
		else if(ret == 2)
			return 85;
		else if(ret == 3)
			return 130;
		/* whether the master has belong to a bridge-group --by Tim*/
		if(if_belong_to_br(if_node->vlan_phy_if)) {
			return 128;
		}
#if 0 //#ifdef CONFIG_VSYS
		if( if_vsys_check(if_node->vlan_phy_if, 0 )) {
			return 544;
		}
#endif

#ifndef CONFIG_TRUNK
		if(tr_if_belong_to_tr(if_node->vlan_phy_if)) {
			return 444;
		}
#endif

		sprintf(realname, "%s.%d",if_node->vlan_phy_if, if_node->vlan_id);
		ret = if_check_aliasname(realname);
		if(ret == 1)
			return 152;
		else if(ret == 3)
			return 153;
		else if(ret == 2)
			return 203;
		if(!vlan_device_number_check())
			return 185;
		if(if_node->sec_ip && !strlen(if_node->ipaddr))
			return 213;

		ret=vlan_device_add_ioctl(if_node->vlan_phy_if,if_node->vlan_id);
		if(ret)
			return ret;
		if_rename(realname, if_node->name);
		strncpy(if_node->name, realname, XML_MAX_NAME_LEN);
	}
#if 0 //#ifdef CONFIG_VSYS
	if( (ui_type == VSYS_UI_SYSTEM_CONFIG)
			&&strlen(if_node->vsys_name) != 0) {
		int ret1 = 0;
		if(vsys_get_id_by_vsysname(if_node->vsys_name,&vsysid) < 0 )
			return 520;
		if( vsysid != 0 )
			ret1 = xml_vsys_add_if(vsysid,if_node->name);
		if( ret1 )
			return ret1;
	}
#endif
	/*handle ip address*/
	if_address_config_clear(if_node->name);
	if_address_type_set(if_node->name,client_xml[VTYSH_INDEX_ZEBRA].fd, if_node->address_type);
	if(if_node->address_type == 1 && strlen(if_node->ipaddr)) { /*static ip config*/
		int ret = 0;
		struct {
			struct oam_data_st oh;
			struct zebra_ip_addr_st data;
		}  buf;
		if_node = (struct interface_node *)ptr;
		memset(&buf, 0, sizeof buf);
		buf.oh.cmd_code = ZEBRA_ADD_ADDRESS;
		buf.oh.mod_id = 0;
		strncpy(buf.data.ifname , if_node->name, XML_MAX_NAME_LEN);
		strncpy(buf.data.addr_str , if_node->ipaddr, XML_IP_LEN );
		write(client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof buf);

		while (1) {
			int nbytes;

			nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof(buf)-1);

			if (nbytes <= 0 && errno != EINTR) {
				return 0;
			}

			if (nbytes > 0) {
				ret = buf.oh.cmd_code;
				if (ret != 0)
					return  buf.oh.cmd_code;
				break;
			}
		}
	} else if(if_node->address_type == 2) { /*dhcp ip config*/
		int ret = 0;
		struct {
			struct oam_data_st oh;
			struct dhcp_config_st data;
		}  buf;
		if_node = (struct interface_node *)ptr;
		memset(&buf, 0, sizeof buf);
		buf.oh.cmd_code = DHCP_IF_CLIENT_ENABLE;
		buf.oh.mod_id = 0;
		sprintf(buf.data.dhcpargv[0], "%d", if_node->dhcp_distance);
		if (if_node->dhcp_default_gate)
			strcpy(buf.data.dhcpargv[1], "reset");
		else
			strcpy(buf.data.dhcpargv[1], "default");
		if (if_node->dhcp_dns)
			strcpy(buf.data.dhcpargv[2], "reset");
		else
			strcpy(buf.data.dhcpargv[2], "default");
		strncpy(buf.data.dhcpargv[3], if_node->name, IFNAMSIZ);
		write(client_xml[VTYSH_INDEX_DHCP].fd, &buf, sizeof buf);

		while (1) {
			int nbytes;

			nbytes = read (client_xml[VTYSH_INDEX_DHCP].fd, &buf, sizeof(buf)-1);

			if (nbytes <= 0 && errno != EINTR) {
				return 0;
			}

			if (nbytes > 0) {
				ret = buf.oh.cmd_code;
				if (ret != 0)
					return  buf.oh.cmd_code;
				break;
			}
		}
	}
	if(if_node->address_type == 3) { /*pppoe ip config*/
		int ret = 0;
		struct {
			struct oam_data_st oh;
			struct pppoe_link_st data;
		}  buf;
		if_node = (struct interface_node *)ptr;
		memset(&buf, 0, sizeof buf);
		buf.oh.cmd_code = PPPOE_ENABLE;
		buf.oh.mod_id = 0;
		buf.data.enable = 1;
		strncpy(buf.data.ifname , if_node->name, IFNAMSIZ);

		if(strlen(if_node->pppoe_user) > 0) {
			strncpy(buf.data.username, if_node->pppoe_user, strlen(if_node->pppoe_user));
			buf.data.flags |= username_enable;
		}
		if(strlen(if_node->pppoe_passwd) > 0) {
			strncpy(buf.data.pswd, if_node->pppoe_passwd, strlen(if_node->pppoe_passwd));
			buf.data.flags |= pswd_enable;
		}
		if(strlen(if_node->pppoe_specify_ip) > 0) {
			memcpy(buf.data.addr_str, if_node->pppoe_specify_ip, XML_MAX_IP_LEN);
			buf.data.flags |= local_addr_enable;
		}
		if(if_node->pppoe_default_gate) {
			buf.data.gateway = if_node->pppoe_default_gate;
			buf.data.flags |= pppoe_gateway_enable;

			buf.data.distance = if_node->pppoe_distance;
			buf.data.flags |= pppoe_distance_enable;
			buf.data.weight= if_node->pppoe_weight;
			buf.data.flags |= pppoe_weight_enable;
		}
		if(if_node->pppoe_dns) {
			buf.data.dns = if_node->pppoe_dns;
			buf.data.flags |= pppoe_dns_enable;
		}
		buf.data.flags |= pppoe_dialer_enable;

		write(client_xml[VTYSH_INDEX_PPPOE].fd, &buf, sizeof buf);

		while (1) {
			int nbytes;

			nbytes = read (client_xml[VTYSH_INDEX_PPPOE].fd, &buf, sizeof(buf)-1);

			if (nbytes <= 0 && errno != EINTR) {
				return 0;
			}

			if (nbytes > 0) {
				ret = buf.oh.cmd_code;
				if (ret != 0)
					return  buf.oh.cmd_code;
				break;
			}
		}
	}

	/* ip ddns */
	struct {
		struct oam_data_st oh;
		struct ddns_cfg dc;
	} dbuf;

	memset(&dbuf, 0, sizeof dbuf);
	dbuf.oh.mod_id = VTYSH_INDEX_DDNS;

	if (if_node->ddns_on == 1) {
		dbuf.oh.cmd_code = DDNS_SET_CFG;
		dbuf.dc.cmd = DDNS_SET_ALL;
		dbuf.dc.on =  if_node->ddns_on;
	} else {
		dbuf.oh.cmd_code = DDNS_NOSET_CFG;
		dbuf.dc.cmd = DDNS_DISABLE;
	}

	if (if_node->ddns_type > 0)
		dbuf.dc.type =  if_node->ddns_type;

	if (if_node->ddns_interval > 0)
		dbuf.dc.interval =  if_node->ddns_interval;

	if (if_node->ddns_myip[0] != 0) {
		dbuf.dc.iptype = 1;
		dbuf.dc.myip =  ip_addr(if_node->ddns_myip);
	}

	if (if_node->ddns_user[0] != 0)
		strncpy(dbuf.dc.username, if_node->ddns_user, XML_MAX_NAME_LEN);

	if (if_node->ddns_pass[0] != 0)
		strncpy(dbuf.dc.password, if_node->ddns_pass, XML_MAX_NAME_LEN);

	if (if_node->ddns_domain[0] != 0)
		strncpy(dbuf.dc.domain, if_node->ddns_domain, XML_MAX_NAME_LEN);

	strncpy(dbuf.dc.myif, if_node->name, IFNAMSIZ);

	write(client_xml[VTYSH_INDEX_DDNS].fd, &dbuf, sizeof (dbuf));

	while (1) {
		int nbytes;

		nbytes = read (client_xml[VTYSH_INDEX_DDNS].fd, &dbuf, sizeof(dbuf));

		if (nbytes <= 0 && errno != EINTR) {
			break;
		}

		if (nbytes > 0) {
			int ddns_ret;
			ddns_ret = dbuf.oh.cmd_code;
			if (ddns_ret != 0)
				return  ddns_ret;

			break;
		}
	}


	/*handle shutdown*/
	struct shutdown_st sbuf;
	if (0 == if_node->shut) {
		sbuf.cmd = 2;
		strncpy(sbuf.ifname, if_node->name, IFNAMSIZ-1);
		kernel_request(CTRL_SHUTDOW, (char *)&sbuf, sizeof(sbuf), sizeof(sbuf), 0);
	} else if(1 == if_node->shut) {
		sbuf.cmd = 1;
		strncpy(sbuf.ifname, if_node->name, IFNAMSIZ-1);
		kernel_request(CTRL_SHUTDOW, (char *)&sbuf, sizeof(sbuf), sizeof(sbuf), 0);
	}
	/*handle local access*/
	struct allowaccess_st abuf;
	memset(&abuf, 0, sizeof(struct allowaccess_st));
	abuf.cmd = 2;
	strncpy(abuf.ifname, if_node->name, IFNAMSIZ-1);
	abuf.access_type = 0xffff;
	kernel_request(CTRL_ALLOWACCESS, (char *)&abuf, sizeof(abuf), sizeof(abuf), 0);
	abuf.cmd = 1;
	abuf.access_type = 0;
	if(if_node->http)
		abuf.access_type |= IFF_ACCESS_HTTP;
	if(if_node->https)
		abuf.access_type |= IFF_ACCESS_HTTPS;
	if(if_node->telnet)
		abuf.access_type |= IFF_ACCESS_TELNET;
	if(if_node->ssh)
		abuf.access_type |= IFF_ACCESS_SSH;
	if(if_node->ping)
		abuf.access_type |= IFF_ACCESS_PING;
	if(if_node->l2tp)
		abuf.access_type |= IFF_ACCESS_L2TP;
	if(if_node->sslvpn)
		abuf.access_type |= IFF_ACCESS_SSLVPN;
	if(if_node->webauth)
		abuf.access_type |= IFF_ACCESS_WEBAUTH;
	if(if_node->cen_monitor)
		abuf.access_type |= IFF_ACCESS_CENMONI;
	kernel_request(CTRL_ALLOWACCESS, (char *)&abuf, sizeof(abuf), sizeof(abuf), 0);
	/*mtu*/
	if(if_node->mtu)
		if_mtu_set(if_node->name,if_node->mtu);

	/*interface bandwidth*/
	if( if_node->if_bandwidth )
		if_bandwidth_set(if_node->name, if_node->if_bandwidth);

	/*desc*/
	if(strlen(if_node->desc))
		if_desc_set(if_node->name,if_node->desc);
	else
		if_desc_unset(if_node->name);

	/*second_ip*/
	int ret = 0;
	struct {
		struct oam_data_st oh;
		struct zebra_ip_addr_st data;
	}  buf;
	memset(&buf, 0, sizeof buf);
	buf.oh.cmd_code = ZEBRA_DEL_ADDRESS;
	buf.oh.mod_id = 0;
	strncpy(buf.data.ifname , if_node->name, XML_MAX_NAME_LEN);
	buf.data.addr_type = 1;
	write(client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof buf);

	while (1) {
		int nbytes;

		nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof(buf)-1);

		if (nbytes <= 0 && errno != EINTR) {
			return 0;
		}

		if (nbytes > 0) {
			ret = buf.oh.cmd_code;
			if (ret != 0)
				return  buf.oh.cmd_code;
			break;
		}
	}

	if(if_node->sec_ip) {
		int ret = 0;
		struct {
			struct oam_data_st oh;
			struct zebra_ip_addr_st data;
		}  buf;
		struct second_ip *sec_ip;
		sec_ip = if_node->sec_ip;
		while(sec_ip) {
			memset(&buf, 0, sizeof buf);
			buf.oh.cmd_code = ZEBRA_ADD_ADDRESS;
			buf.oh.mod_id = 0;
			strncpy(buf.data.ifname , if_node->name, XML_MAX_NAME_LEN);
			strncpy(buf.data.addr_str , sec_ip->second_ip, XML_IP_LEN );
			buf.data.addr_type = 1;
			write(client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof buf);

			while (1) {
				int nbytes;

				nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof(buf)-1);

				if (nbytes <= 0 && errno != EINTR) {
					return 0;
				}

				if (nbytes > 0) {
					ret = buf.oh.cmd_code;
					if (ret != 0)
						return  buf.oh.cmd_code;
					break;
				}
			}
			sec_ip = sec_ip->next;
		}
	}
#endif
	return 0;
}

int link_bind_config (int cmd, struct link_bind_cfg *p_cfg)
{
	struct {
		struct oam_data_st oh;
		struct link_bind_cfg data;
	} buf;

	memset (&buf, 0, sizeof(buf));
	buf.oh.cmd_code = cmd;
	buf.oh.mod_id = VTYSH_INDEX_LINK_BIND;
	memcpy(&buf.data, (char *)p_cfg, sizeof(struct link_bind_cfg));

	write(client_xml[VTYSH_INDEX_LINK_BIND].fd, &buf, sizeof(buf));

	while (1) {
		int nbytes;

		nbytes = read (client_xml[VTYSH_INDEX_LINK_BIND].fd, &buf, sizeof(buf)-1);

		if (nbytes <= 0 && errno != EINTR) {
			return 0;
		}

		if (nbytes > 0) {
			if (buf.oh.cmd_code ==1) {
				//	vsos_debug_out("XML-DEBUG: Delet L3-BIND\r\n");
			}
			break;
		}
	}
	return 0;
}

int  ctrl_bind_interface(int cmd ,char * if1,char *if2)
{
#if 0
	struct bind_interface_st sbuf;
	memset(&sbuf, 0, sizeof(struct bind_interface_st));
	sbuf.cmd = cmd;
	strcpy(sbuf.ifname1,if1);
	strcpy(sbuf.ifname2,if2);
	kernel_request(CTRL_SET_BIND_IF, (char *)&sbuf, sizeof(sbuf), sizeof(sbuf), 0);
#endif
	return 0;
}

char linkbind_name[XML_MAX_NAME_LEN];
int linkbind_if = 0;
static int bind_L2_show (char* buf, int buflen, void*param)
{
#if 0
	struct bind_interface_st *trans;
	char *ifname = param;
	assert(ifname);

	if (!buf) {
		//vsos_debug_out("XML-DEBUG: bind_L2 not found!\r\n");
		return 0;
	}

	trans = (struct bind_interface_st*)buf;
	linkbind_if = 0;
	while (buflen >= sizeof(struct bind_interface_st)) {
		//vsos_debug_out("XML-DEBUG: bind-interface %s %s\r\n",
		//	trans->ifname1,trans->ifname2);
		if(!strncmp(trans->ifname1, ifname, IFNAMSIZ)) {
			strncpy(linkbind_name, trans->ifname2, IFNAMSIZ);
			linkbind_if = 1;
			break;
		} else if(!strncmp(trans->ifname2, ifname, IFNAMSIZ)) {
			strncpy(linkbind_name, trans->ifname1, IFNAMSIZ);
			linkbind_if = 2;
			break;
		}
		trans++;
		buflen -= sizeof(struct bind_interface_st);
	}
#endif
	return 0;
}

static int bind_L3_show (char* ifname)
{
	struct {
		struct oam_data_st oh;
		struct link_bind_cfg cfg;
	} data;

	struct link_bind_cfg *p_cfg = &data.cfg;
	memset(&data, 0, sizeof data);
	data.oh.cmd_code = DUMP_L3_BIND_CFG;
	data.oh.mod_id = VTYSH_INDEX_LINK_BIND;

	write(client_xml[VTYSH_INDEX_LINK_BIND].fd, &data, sizeof(data.oh));

	for(;;) {
		int nbytes;
		memset(&data, 0, sizeof(data));
		nbytes = read (client_xml[VTYSH_INDEX_LINK_BIND].fd, &data, sizeof(data));
		//vsos_debug_out("XML-DEBUG:bytes=%d,cmd_code=%d\r\n", nbytes, data.oh.cmd_code);
		if (nbytes <= 0 && errno != EINTR) {
			return 0;
		}

		if (nbytes > 0) {
			if(data.oh.cmd_code == 2) {
				//vsos_debug_out("XML-DEBUG:bind-interface %s monitor-addr %s\r\n",
				//	p_cfg->if_name, p_cfg->addr_name);
				if(!strncmp(p_cfg->if_name, ifname, IFNAMSIZ)) {
					//	vsos_debug_out("XML-DEBUG:BINGO!\r\n");
					strncpy(linkbind_name, p_cfg->addr_name, XML_MAX_NAME_LEN);
					//	break;
				}
			} else
				break;
		}
	}
	return 0;
}



int xml_interface_node_vlan_count(char *interface_name)
{
	int sum=0;
	FILE *fp;
	char buf[1024 + 1];
	int ver;

	if(if_belong_to_br(interface_name))
		sum++;

	fp = fopen (_PATH_PROC_NET_DEV, "r");
	if (fp == NULL)
		return sum;
	/* Drop header lines. */
	fgets (buf, 1024, fp);
	fgets (buf, 1024, fp);
	ver =procnetdev_version(buf);

	while (fgets (buf, 1024, fp) != NULL) {
		char *s, *p, name[IFNAMSIZ];
		buf[1024] = '\0';
		s = get_name(name, buf);
		if((p=strchr(name, '.'))) {
			if(strncmp(name, interface_name, (p-name))==0) {
				sum++;
			}
		}
	}

	fclose(fp);

	return sum;
}


/************tb new*****************/
int xml_bos_dev_ioctl(void *name, unsigned int cmd, void *pData)
{
	struct ifreq ifr;
	int fd;
	int error;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		return 3;
	}

	bzero(&ifr, sizeof(ifr));
	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if(pData != NULL) {
		ifr.ifr_data = pData;
	}
	error = ioctl(fd, cmd, &ifr);
	if (error) {
		close(fd);
		return 6;
	}

	close(fd);
	
	bos_dev_ifconfig_down(name, cmd);
	return 0;
}
int xml_interface_duplex_set(char *name, u8 duplex)
{
	struct ethtool_cmd ecmd;
	ecmd.cmd = ETHTOOL_GSET;
	xml_bos_dev_ioctl(name, SIOCETHTOOL, &ecmd);

	if(ecmd.autoneg == 1) { // 接口处于自协商， 不能设置duplex
		return 0;
	}

	if((ecmd.speed==1000)&&(duplex==0)) {
		return 0;
	}

	ecmd.cmd = ETHTOOL_SSET;
	ecmd.duplex = duplex;
	xml_bos_dev_ioctl(name, SIOCETHTOOL, &ecmd);

	return 0;
}
int xml_interface_negotiate_set(char *name, u8 autoneg)
{
	struct ethtool_cmd ecmd;
	u32 speed = 0;


	ecmd.cmd = ETHTOOL_GSET;
	xml_bos_dev_ioctl(name, SIOCETHTOOL, &ecmd);

	if(ecmd.port == PORT_FIBRE) { // 光口不让设置非自协商
		return -1;
	}

	ecmd.cmd = ETHTOOL_SSET;
	if((autoneg==0)&&(ecmd.autoneg==1)) { // 由自协商切换到非自协???		ecmd.autoneg = 0;
		ecmd.autoneg = 0;
		ethtool_cmd_speed_set(&ecmd, 1000);
		ecmd.duplex = 1;
		xml_bos_dev_ioctl(name, SIOCETHTOOL, &ecmd);
		return 0;
	}

	if((autoneg==1)&&(ecmd.autoneg==0)) { // 由非自协商切换到自协???		ecmd.autoneg = 1;
		ecmd.autoneg = 1;
		xml_bos_dev_ioctl(name, SIOCETHTOOL, &ecmd);
		return 0;
	}

	if((autoneg==0)&&(ecmd.autoneg==0)) {
		return -1;
	}
	if((autoneg==1)&&(ecmd.autoneg==1)) {
		return -1;
	}
	return 0;
}
int xml_interface_speed_set(char *name, u32 speed)
{
	struct ethtool_cmd ecmd;

	ecmd.cmd = ETHTOOL_GSET;
	xml_bos_dev_ioctl(name, SIOCETHTOOL, &ecmd);

	if(ecmd.autoneg == 1) { // 接口处于自协商， 不能设置speed
		return -1;
	}

	ecmd.cmd = ETHTOOL_SSET;
	ethtool_cmd_speed_set(&ecmd, speed);
	xml_bos_dev_ioctl(name, SIOCETHTOOL, &ecmd);

	return 0;
}

int xml_tb_interface_ipv4_set(struct tb_ip_info *ip_node, char *if_name)
{
	if(ip_node) {
		while(ip_node) {
			int ret = 0;
			struct {
				struct oam_data_st oh;
				struct zebra_ip_addr_st data;
			}  buf;

			if(strchr(ip_node->tb_ip, ':')) {
				ip_node = ip_node->next;
				continue;
			}


			memset(&buf, 0, sizeof buf);
			buf.oh.cmd_code = ZEBRA_ADD_ADDRESS;
			buf.oh.mod_id = VTYSH_INDEX_ZEBRA;
			strncpy(buf.data.ifname , if_name, XML_MAX_NAME_LEN);
			strncpy(buf.data.addr_str , ip_node->tb_ip, XML_IP_LEN );
			buf.data.addr_type = ip_node->is_floating_ip==0?0:2;
			buf.data.ha_unitID = ip_node->is_floating_ip==0?0:ip_node->unit_id;

			write(client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof buf);

			while (1) {
				int nbytes;

				nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof(buf)-1);

				if (nbytes <= 0 && errno != EINTR) {
					return 0;
				}

				if (nbytes > 0) {
					ret = buf.oh.cmd_code;
					if (ret != 0)
						return  buf.oh.cmd_code;
					break;
				}
			}
			ip_node = ip_node->next;
		}
	}

	return 0;
}
int xml_tb_interface_ipv6_set(struct tb_ip_info *ip_node, char *if_name)
{
	int ret = 0;
	struct prefix_ipv6 cp;

	if(ip_node) {
		while(ip_node) {
			int ret = 0;
			struct {
				struct oam_data_st oh;
				struct zebra_ip_addr_st data;
			}  buf;

			if(!strchr(ip_node->tb_ip, ':')) {
				ip_node = ip_node->next;
				continue;
			}

			ret = vsos_ipv6_check_addr(ip_node->tb_ip, &cp);
			if (ret > 0 /*&& ret != 2 && ret != 5*/) {
				if(ret>=1 && ret<=6)
					ret = 34;
				return ret;
			}

			memset(&buf, 0, sizeof buf);
			buf.oh.cmd_code = ZEBRA_IPV6_ADD_ADDRESS;
			buf.oh.mod_id = VTYSH_INDEX_ZEBRA;
			strncpy(buf.data.ifname , if_name, XML_MAX_NAME_LEN);
			strncpy(buf.data.addr_str , ip_node->tb_ip, XML_MAX_IP6_LEN);

			buf.data.addr_type = ip_node->is_floating_ip==0?0:2;
			buf.data.ha_unitID = ip_node->is_floating_ip==0?0:ip_node->unit_id;

	//		buf.data.addr_type = 0;

			write(client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof buf);

			while (1) {
				int nbytes;

				nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof(buf)-1);

				if (nbytes <= 0 && errno != EINTR) {
					return 0;
				}

				if (nbytes > 0) {
					ret = buf.oh.cmd_code;
					if (ret != 0)
						return  buf.oh.cmd_code;
					break;
				}
			}
			ip_node = ip_node->next;
		}
	}

	return 0;
}

int xml_eth_stat_flag_set(char *name, int adminflag, int value)
{
	struct device_flow_ipc_hdr *req;
	int rc = 0, retval = 0;
	int outlen;
	int i;

	req = get_prealloc_buf();
	memset(req, 0, sizeof(struct device_flow_ipc_hdr));
	strncpy(req->ifname, name, IFNAME_SIZE);
	req->cmd = ETH_CMD_ADMINFLAGS_SET;
	req->flag = adminflag;
	req->value = value;

	outlen = sizeof(*req);
	rc = ipc_send(APP_SYSMON, IPCMSG_APP_STAT_FLOW, req, outlen);
	if (rc < 0 || retval < 0) {
		return retval < 0 ? retval : rc;
	}

	return 0;
}

unsigned long xml_interface_node_mod(void *ptr)
{

	assert (NULL != ptr);
	struct interface_node *if_node = ptr;
	struct interface_info ife;
	unsigned short if_adminflags;

	char tmp_name[64];

	int ret;

#if 0
	int address_type;
#if 0 //#ifdef CONFIG_VSYS
	unsigned int vsysid = 0,ifvsysid;
	enum vsys_ui_type ui_type;

#endif
#endif

	if(interface_find_proc(if_node->name) <= 0)
		return 10;

#ifdef HAVE_IPV6
	if(if_has_ipv6(if_node->real_name) && (if_node->mtu<1280/*IPV6_MIN_MTU*/))
		return 49;
#endif

	memset(tmp_name, 0, 64);
	if_get_alias_by_name(if_node->real_name, tmp_name);

	if(!strncmp(if_node->real_name, if_node->alias_name, IFNAMSIZ) && tmp_name[0]!='\0') {
		char tmp_c[64] = {0};
		if_rename(if_node->real_name, tmp_c);
	} else {
		if(if_node->alias_name[0]!='\0' && strncmp(if_node->alias_name, tmp_name, 64)) {

			if(if_check_aliasname_by_rules(if_node->alias_name))
				return 842;

			ret = if_check_aliasname(if_node->alias_name);
			if(ret == 1)
				return 84;
			else if(ret == 2)
				return 85;

			if(zonename_search(if_node->alias_name)) {
				return 153;
			}

			if_rename(if_node->real_name, if_node->alias_name);
		}
	}

	/********TB new*********/
	ret = check_vm_mode();
	if(ret && !strncmp(if_node->name, "mgt", 3)) {
		goto NEXT;
	}

	strncpy(ife.name,if_node->name,IFNAMSIZ);
	fetch_interface(&ife);

	if(if_node->negotiate==1 && ife.autoneg==AUTONEG_ENABLE) {
		xml_interface_negotiate_set(if_node->name, 0);
	} else if(if_node->negotiate==0 && ife.autoneg!=AUTONEG_ENABLE) {
		xml_interface_negotiate_set(if_node->name, 1);
	}

	xml_interface_speed_set(if_node->name, (u32)if_node->speed);
	xml_interface_duplex_set(if_node->name, (u8)if_node->half);

	{
		struct shutdown_st sbuf;
		if(if_node->shut && !ife.shutdown) {
			sbuf.cmd = 1;
			strcpy(sbuf.ifname, if_node->name);
			kernel_request(CTRL_SHUTDOW, (char *)&sbuf, sizeof(sbuf), sizeof(sbuf), 0);
			xml_bos_dev_ioctl(if_node->name, (BOS_SIOETHIFSHUTDOWN), NULL);
		} else if(!if_node->shut && ife.shutdown) {
			sbuf.cmd = 2;
			strcpy(sbuf.ifname, if_node->name);
			kernel_request(CTRL_SHUTDOW, (char *)&sbuf, sizeof(sbuf), sizeof(sbuf), 0);
			xml_bos_dev_ioctl(if_node->name, (BOS_SIOETHIFNOSHUTDOWN), NULL);
		}
	}
NEXT:
	/*mtu*/
	if(if_node->mtu) {
		if_adminflags = if_adminflags_get(if_node->name);
		if(!(if_adminflags & IFF_USEDBY_BR) && !(if_adminflags & IFF_USEDBY_VLAN) && !(if_adminflags & IFF_USEDBY_TRUNK)) {
			if_mtu_set(if_node->name,if_node->mtu);
		}
	}

	/*handle local access*/
	struct local_access_u abuf;
	memset(&abuf, 0, sizeof(struct local_access_u));
	strncpy(abuf.ifname, if_node->name,IFNAMSIZ);
	abuf.access_type = 0xffff;
	abuf.cmd = LOCAL_ACCESS_CMD_UNSET;
	kernel_request(CTRL_ALLOWACCESS, (char *)&abuf, sizeof(abuf), sizeof(abuf), 0);
	dataplane_request(UIPC_DEV_ALLOWACCESS,(char *)&abuf, sizeof(abuf), sizeof(abuf), 0);	

	abuf.cmd = LOCAL_ACCESS_CMD_SET;
	abuf.access_type = 0;
	if(if_node->http)
		abuf.access_type |= IFF_ACCESS_HTTP;
	if(if_node->https)
		abuf.access_type |= IFF_ACCESS_HTTPS;
	if(if_node->telnet)
		abuf.access_type |= IFF_ACCESS_TELNET;
	if(if_node->ssh)
		abuf.access_type |= IFF_ACCESS_SSH;
	if(if_node->sslvpn)
		abuf.access_type |= IFF_ACCESS_SSLVPN;
	if(if_node->ping)
		abuf.access_type |= IFF_ACCESS_PING;
	if(if_node->bgp)
		abuf.access_type |= IFF_ACCESS_BGP;
	if(if_node->ospf)
		abuf.access_type |= IFF_ACCESS_OSPF;
	if(if_node->rip)
		abuf.access_type |= IFF_ACCESS_RIP;
	if(if_node->dns)
		abuf.access_type |= IFF_ACCESS_DNS;
	if(if_node->tctrl)
		abuf.access_type |= IFF_ACCESS_TCTRL;
	if(if_node->webauth)
		abuf.access_type |= IFF_ACCESS_WEBAUTH;
	if(if_node->linkage)
		abuf.access_type |= IFF_ACCESS_LINKAGE;
	kernel_request(CTRL_ALLOWACCESS, (char *)&abuf, sizeof(abuf), sizeof(abuf), 0);
	dataplane_request(UIPC_DEV_ALLOWACCESS,(char *)&abuf, sizeof(abuf), sizeof(abuf), 0);	
	if ((0 == strncmp(if_node->name, "eth", 3)) ||
			(0 == strncmp(if_node->name, "ge", 2)) ||
			(0 == strncmp(if_node->name, "xge", 3))) {
		if (if_node->external) {
			dataplane_request(UIPC_DEV_SET_EXTERNAL, if_node->name, IFNAMSIZ, IFNAMSIZ, 0);
			xml_eth_stat_flag_set(if_node->name, ETH_CMD_ADMINFLAGS_EXTERNAL,1);
		} else {
			dataplane_request(UIPC_DEV_SET_INTERNAL, if_node->name, IFNAMSIZ, IFNAMSIZ, 0);
			xml_eth_stat_flag_set(if_node->name, ETH_CMD_ADMINFLAGS_EXTERNAL,0);
		}
	}

	/*tb interface ip*/
	//if(strncmp(if_node->name, "mgt", 3)) {
	if(if_node->tb_interface_ip && if_node->address_type == 1) {
		if_adminflags = if_adminflags_get(if_node->name);
		if((if_adminflags & IFF_USEDBY_BR) || (if_adminflags & IFF_USEDBY_VLAN) || (if_adminflags & IFF_USEDBY_TRUNK))
			return 843;

		if_address_config_clear(if_node->name);
		ret = xml_tb_interface_ipv4_set(if_node->tb_interface_ip, if_node->name);
		if(ret)
			return ret;

		if_ipv6_address_config_clear(if_node->name);
		ret = xml_tb_interface_ipv6_set(if_node->tb_interface_ip, if_node->name);
		if(ret)
			return ret;

		if_address_type_set(if_node->name,
			client_xml[VTYSH_INDEX_ZEBRA].fd, if_node->address_type);
	} else {
		if_address_config_clear(if_node->name);
		if_ipv6_address_config_clear(if_node->name);
#if 1
		if(if_node->address_type == 3) { /*pppoe ip config*/
			int ret = 0;
			struct {
				struct oam_data_st oh;
				struct pppoe_link_st data;
			}  buf;

			if_address_type_set(if_node->name,client_xml[VTYSH_INDEX_ZEBRA].fd, if_node->address_type);


			if_node = (struct interface_node *)ptr;
			memset(&buf, 0, sizeof buf);
			buf.oh.cmd_code = PPPOE_ENABLE;
			buf.oh.mod_id = 0;
			buf.data.enable = 1;
			strncpy(buf.data.ifname , if_node->name, IFNAMSIZ);

			if (if_node->pppoe_distance == -1)
				buf.data.distance = 1;
			else
				buf.data.distance = if_node->pppoe_distance;
			buf.data.flags |= pppoe_distance_enable;

			if (if_node->pppoe_weight == -1)
				buf.data.weight = 1;
			else
				buf.data.weight = if_node->pppoe_weight;
			buf.data.flags |= pppoe_weight_enable;


			strncpy(buf.data.username, if_node->pppoe_user, strlen(if_node->pppoe_user));
			buf.data.flags |= username_enable;

			strncpy(buf.data.pswd, if_node->pppoe_passwd, strlen(if_node->pppoe_passwd));
			buf.data.flags |= pswd_enable;

			if(strlen(if_node->pppoe_specify_ip) > 0) {
				sprintf(buf.data.addr_str, "%s/32", if_node->pppoe_specify_ip);
			} else
				strncpy(buf.data.addr_str, "0.0.0.0", 7);
			buf.data.flags |= local_addr_enable;

			buf.data.gateway = if_node->pppoe_default_gate;
			buf.data.flags |= pppoe_gateway_enable;

			buf.data.dns = if_node->pppoe_dns;
			buf.data.flags |= pppoe_dns_enable;

			buf.data.flags |= pppoe_dialer_enable;

			write(client_xml[VTYSH_INDEX_PPPOE].fd, &buf, sizeof buf);

			while (1) {
				int nbytes;

				nbytes = read (client_xml[VTYSH_INDEX_PPPOE].fd, &buf, sizeof(buf)-1);

				if (nbytes <= 0 && errno != EINTR) {
					return 0;
				}

				if (nbytes > 0) {
					ret = buf.oh.cmd_code;
					if (ret != 0)
						return  buf.oh.cmd_code;
					break;
				}
			}
		} else if (if_node->address_type == 2) {
			int ret = 0;
			int if_dhcpcfg_change=0;
			struct if_dhcp_config dhcp_ipcfg;

			if_address_type_set(if_node->name,client_xml[VTYSH_INDEX_ZEBRA].fd, if_node->address_type);
			if_node = (struct interface_node *)ptr;
			memset(&dhcp_ipcfg, 0 , sizeof(dhcp_ipcfg));
			xml_interface_get_ipcfg(if_node->name, if_node->address_type, (void *)&dhcp_ipcfg);

			if (!(dhcp_ipcfg.flags & DHCPC_CFLAG_ENABLE)) {
				if_dhcpcfg_change = 1;
			} else if (dhcp_ipcfg.metric != if_node->dhcp_distance) {
				if_dhcpcfg_change = 1;
			} else if ((if_node->dhcp_default_gate && !(dhcp_ipcfg.flags & DHCPC_CFLAG_GW_RESET)) ||
					   (!if_node->dhcp_default_gate && (dhcp_ipcfg.flags & DHCPC_CFLAG_GW_RESET))) {
				if_dhcpcfg_change = 1;
			} else if ((if_node->dhcp_dns && !(dhcp_ipcfg.flags & DHCPC_CFLAG_DNS_RESET)) ||
					   (!if_node->dhcp_dns && (dhcp_ipcfg.flags & DHCPC_CFLAG_DNS_RESET))) {
				if_dhcpcfg_change = 1;
			}

			if (if_dhcpcfg_change) {
				/* firstly, stop */
				struct {
					struct oam_data_st oh;
					char rbuf[VTY_READ_BUFSIZ];
				} buf1;

				memset(&buf1, 0, sizeof buf1);
				buf1.oh.cmd_code = DHCP_IF_CLIENT_DISABLE;
				buf1.oh.mod_id = VTYSH_INDEX_DHCP;
				strncpy(buf1.rbuf, if_node->name, IFNAMSIZ);
				write(client_xml[VTYSH_INDEX_DHCP].fd, &buf1, sizeof (struct oam_data_st) + IFNAMSIZ);

				while (1) {
					int nbytes;

					nbytes = read (client_xml[VTYSH_INDEX_DHCP].fd, &buf1, sizeof(buf1));

					if (nbytes <= 0 && errno != EINTR) {
						break;
					}

					if (nbytes > 0) {
						break;
					}
				}

				/* secondly, start */
				struct {
					struct oam_data_st oh;
					struct dhcp_config_st data;
				}  buf;

				memset(&buf, 0, sizeof buf);
				buf.oh.cmd_code = DHCP_IF_CLIENT_ENABLE;
				buf.oh.mod_id = 0;
				sprintf(buf.data.dhcpargv[0], "%d", if_node->dhcp_distance);
				if (if_node->dhcp_default_gate)
					strcpy(buf.data.dhcpargv[1], "reset");
				else
					strcpy(buf.data.dhcpargv[1], "default");
				if (if_node->dhcp_dns)
					strcpy(buf.data.dhcpargv[2], "reset");
				else
					strcpy(buf.data.dhcpargv[2], "default");
				strncpy(buf.data.dhcpargv[3], if_node->name, IFNAMSIZ);
				write(client_xml[VTYSH_INDEX_DHCP].fd, &buf, sizeof buf);

				while (1) {
					int nbytes;

					nbytes = read (client_xml[VTYSH_INDEX_DHCP].fd, &buf, sizeof(buf)-1);

					if (nbytes <= 0 && errno != EINTR) {
						return 0;
					}

					if (nbytes > 0) {
						ret = buf.oh.cmd_code;
						if (ret != 0)
							return  buf.oh.cmd_code;
						break;
					}
				}
			}
		}
#endif

	}
	//}

#if 0
	if(if_node->sec_ip && !strlen(if_node->ipaddr))
		return 213;
#if 0 //#ifdef CONFIG_VSYS
	/*handle vsys change*/
	if( xml_vsys_gui_get_current_uitype(&ui_type) < 0 ) {
		vsos_debug_out("xml_interface_node_showall: get vsys_ui_type failed\r\n");
		return 535;
	}
	if( (ui_type == VSYS_UI_SYSTEM_CONFIG)
			&&strlen(if_node->vsys_name) != 0) {
		if(vsys_get_id_by_vsysname(if_node->vsys_name,&vsysid) < 0 )
			return 520;
		ifvsysid = (unsigned int)if_vsys_get(if_node->name);
		if( ifvsysid != vsysid) {
			int ret = 0;
			if( ifvsysid )
				ret =xml_vsys_del_if(ifvsysid, if_node->name);
			if( ret )
				return ret;

			if( vsysid != 0 )
				ret = xml_vsys_add_if(vsysid,if_node->name);
			if( ret )
				return ret;
		}
	}
#endif
	/*handle ip address*/
	address_type = if_address_type_get(if_node->name, client_xml[VTYSH_INDEX_ZEBRA].fd);
	if(address_type != if_node->address_type) {
		if_address_config_clear(if_node->name);
		if_address_type_set(if_node->name,client_xml[VTYSH_INDEX_ZEBRA].fd, if_node->address_type);
	}
	if_adminflags = if_adminflags_get(if_node->name);

	if(if_node->address_type == 1 && strlen(if_node->ipaddr)) { /*static ip config*/
		int ret = 0;
		struct {
			struct oam_data_st oh;
			struct zebra_ip_addr_st data;
		}  buf;

		/* ?????ӿ??Ƿ???????bridge or zone */
		if(if_node->ipaddr[0] && (if_adminflags & (IFF_USEDBY_BR)))
			return 140;
		if(if_node->ipaddr[0] && (if_adminflags & (IFF_USEDBY_TRUNK)))
			return 440;
		ret = ip_address_check(if_node->ipaddr, if_node->name);
		if(ret != 0)
			return ret;
		else {
			int ret = 0;
			struct {
				struct oam_data_st oh;
				struct zebra_ip_addr_st data;
			}  buf;
			memset(&buf, 0, sizeof buf);
			buf.oh.cmd_code = ZEBRA_DEL_ADDRESS;
			buf.oh.mod_id = 0;
			strncpy(buf.data.ifname , if_node->name, XML_MAX_NAME_LEN);
			buf.data.addr_type = 1;
			write(client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof buf);

			while (1) {
				int nbytes;

				nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof(buf)-1);

				if (nbytes <= 0 && errno != EINTR) {
					return 0;
				}

				if (nbytes > 0) {
					ret = buf.oh.cmd_code;
					if (ret != 0)
						return  buf.oh.cmd_code;
					break;
				}
			}
		}
		if_node = (struct interface_node *)ptr;
		memset(&buf, 0, sizeof buf);
		buf.oh.cmd_code = ZEBRA_ADD_ADDRESS;
		buf.oh.mod_id = 0;
		strncpy(buf.data.ifname , if_node->name, IFNAMSIZ);
		strncpy(buf.data.addr_str , if_node->ipaddr, XML_IP_LEN );
		write(client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof buf);

		while (1) {
			int nbytes;

			nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof(buf)-1);

			if (nbytes <= 0 && errno != EINTR) {
				return 0;
			}

			if (nbytes > 0) {
				ret = buf.oh.cmd_code;
				if (ret != 0)
					return  buf.oh.cmd_code;
				break;
			}
		}
	} else if(if_node->address_type == 2) { /*dhcp ip config*/
		int ret = 0;
		/* ?????ӿ??Ƿ???????bridge or zone */
		if(if_adminflags & (IFF_USEDBY_BR))
			return 140;
		if(if_adminflags & (IFF_USEDBY_TRUNK))
			return 440;
		if_node = (struct interface_node *)ptr;

		int if_dhcpcfg_change=0;

		struct if_dhcp_config dhcp_ipcfg;
		memset(&dhcp_ipcfg, 0 , sizeof(dhcp_ipcfg));
		xml_interface_get_ipcfg(if_node->name, 2, (void *)&dhcp_ipcfg);

		if (!dhcp_ipcfg.flags & DHCPC_CFLAG_ENABLE) {
			if_dhcpcfg_change = 1;
		} else if (dhcp_ipcfg.metric != if_node->dhcp_distance) {
			if_dhcpcfg_change = 1;
		} else if ((if_node->dhcp_default_gate && !(dhcp_ipcfg.flags & DHCPC_CFLAG_GW_RESET)) ||
				   (!if_node->dhcp_default_gate && (dhcp_ipcfg.flags & DHCPC_CFLAG_GW_RESET))) {
			if_dhcpcfg_change = 1;
		} else if ((if_node->dhcp_dns && !(dhcp_ipcfg.flags & DHCPC_CFLAG_DNS_RESET)) ||
				   (!if_node->dhcp_dns && (dhcp_ipcfg.flags & DHCPC_CFLAG_DNS_RESET))) {
			if_dhcpcfg_change = 1;
		}

		if (if_dhcpcfg_change) {
			/* firstly, stop */
			struct {
				struct oam_data_st oh;
				char rbuf[VTY_READ_BUFSIZ];
			} buf1;

			memset(&buf1, 0, sizeof buf1);
			buf1.oh.cmd_code = DHCP_IF_CLIENT_DISABLE;
			buf1.oh.mod_id = VTYSH_INDEX_DHCP;
			strncpy(buf1.rbuf, if_node->name, IFNAMSIZ);
			write(client_xml[VTYSH_INDEX_DHCP].fd, &buf1, sizeof (struct oam_data_st) + IFNAMSIZ);

			while (1) {
				int nbytes;

				nbytes = read (client_xml[VTYSH_INDEX_DHCP].fd, &buf1, sizeof(buf1));

				if (nbytes <= 0 && errno != EINTR) {
					break;
				}

				if (nbytes > 0) {
					break;
				}
			}

			/* secondly, start */
			struct {
				struct oam_data_st oh;
				struct dhcp_config_st data;
			}  buf;

			memset(&buf, 0, sizeof buf);
			buf.oh.cmd_code = DHCP_IF_CLIENT_ENABLE;
			buf.oh.mod_id = 0;
			sprintf(buf.data.dhcpargv[0], "%d", if_node->dhcp_distance);
			if (if_node->dhcp_default_gate)
				strcpy(buf.data.dhcpargv[1], "reset");
			else
				strcpy(buf.data.dhcpargv[1], "default");
			if (if_node->dhcp_dns)
				strcpy(buf.data.dhcpargv[2], "reset");
			else
				strcpy(buf.data.dhcpargv[2], "default");
			strncpy(buf.data.dhcpargv[3], if_node->name, IFNAMSIZ);
			write(client_xml[VTYSH_INDEX_DHCP].fd, &buf, sizeof buf);

			while (1) {
				int nbytes;

				nbytes = read (client_xml[VTYSH_INDEX_DHCP].fd, &buf, sizeof(buf)-1);

				if (nbytes <= 0 && errno != EINTR) {
					return 0;
				}

				if (nbytes > 0) {
					ret = buf.oh.cmd_code;
					if (ret != 0)
						return  buf.oh.cmd_code;
					break;
				}
			}
		}
	}
	if(if_node->address_type == 3) { /*pppoe ip config*/
		int ret = 0;
		struct {
			struct oam_data_st oh;
			struct pppoe_link_st data;
		}  buf;

		/* ?????ӿ??Ƿ???????bridge or zone */
		if(if_adminflags & (IFF_USEDBY_BR))
			return 140;
		if(if_adminflags & (IFF_USEDBY_TRUNK))
			return 440;
		if_node = (struct interface_node *)ptr;
		memset(&buf, 0, sizeof buf);
		buf.oh.cmd_code = PPPOE_ENABLE;
		buf.oh.mod_id = 0;
		buf.data.enable = 1;
		strncpy(buf.data.ifname , if_node->name, IFNAMSIZ);

		buf.data.distance = if_node->pppoe_distance;
		buf.data.flags |= pppoe_distance_enable;

		buf.data.weight = if_node->pppoe_weight;
		buf.data.flags |= pppoe_weight_enable;


		strncpy(buf.data.username, if_node->pppoe_user, strlen(if_node->pppoe_user));
		buf.data.flags |= username_enable;

		strncpy(buf.data.pswd, if_node->pppoe_passwd, strlen(if_node->pppoe_passwd));
		buf.data.flags |= pswd_enable;

		if(strlen(if_node->pppoe_specify_ip) > 0) {
			sprintf(buf.data.addr_str, "%s/32", if_node->pppoe_specify_ip);
		} else
			strncpy(buf.data.addr_str, "0.0.0.0", 7);
		buf.data.flags |= local_addr_enable;

		buf.data.gateway = if_node->pppoe_default_gate;
		buf.data.flags |= pppoe_gateway_enable;

		buf.data.dns = if_node->pppoe_dns;
		buf.data.flags |= pppoe_dns_enable;

		buf.data.flags |= pppoe_dialer_enable;

		write(client_xml[VTYSH_INDEX_PPPOE].fd, &buf, sizeof buf);

		while (1) {
			int nbytes;

			nbytes = read (client_xml[VTYSH_INDEX_PPPOE].fd, &buf, sizeof(buf)-1);

			if (nbytes <= 0 && errno != EINTR) {
				return 0;
			}

			if (nbytes > 0) {
				ret = buf.oh.cmd_code;
				if (ret != 0)
					return  buf.oh.cmd_code;
				break;
			}
		}
	} else if(if_node->address_type == 1 && !strlen(if_node->ipaddr)) { /*delete ip addr*/
		if_address_config_clear(if_node->name);
		if_address_type_set(if_node->name,client_xml[VTYSH_INDEX_ZEBRA].fd, if_node->address_type);
	}
	/* ip ddns */
	/* ?????ӿ??Ƿ???????bridge or zone */
	if((if_adminflags & (IFF_USEDBY_BR)) && (if_node->ddns_on))
		return 140;
	if((if_adminflags & (IFF_USEDBY_TRUNK)) && (if_node->ddns_on))
		return 440;

	struct {
		struct oam_data_st oh;
		struct ddns_cfg dc;
	} dbuf;

	memset(&dbuf, 0, sizeof dbuf);
	dbuf.oh.mod_id = VTYSH_INDEX_DDNS;

	if (if_node->ddns_on == 1) {
		dbuf.oh.cmd_code = DDNS_SET_CFG;
		dbuf.dc.cmd = DDNS_SET_ALL;
		dbuf.dc.on =  if_node->ddns_on;
	} else {
		dbuf.oh.cmd_code = DDNS_NOSET_CFG;
		dbuf.dc.cmd = DDNS_DISABLE;
	}

	if (if_node->ddns_type > 0)
		dbuf.dc.type =  if_node->ddns_type;

	if (if_node->ddns_interval > 0)
		dbuf.dc.interval =  if_node->ddns_interval;

	if (if_node->ddns_myip[0] != 0) {
		dbuf.dc.iptype = 1;
		dbuf.dc.myip =  ip_addr(if_node->ddns_myip);
	}

	if (if_node->ddns_user[0] != 0)
		strncpy(dbuf.dc.username, if_node->ddns_user, XML_MAX_NAME_LEN);

	if (if_node->ddns_pass[0] != 0)
		strncpy(dbuf.dc.password, if_node->ddns_pass, XML_MAX_NAME_LEN);

	if (if_node->ddns_domain[0] != 0)
		strncpy(dbuf.dc.domain, if_node->ddns_domain, XML_MAX_NAME_LEN);

	strncpy(dbuf.dc.myif, if_node->name, IFNAMSIZ);

	write(client_xml[VTYSH_INDEX_DDNS].fd, &dbuf, sizeof (dbuf));

	while (1) {
		int nbytes;

		nbytes = read (client_xml[VTYSH_INDEX_DDNS].fd, &dbuf, sizeof(dbuf));

		if (nbytes <= 0 && errno != EINTR) {
			break;
		}

		if (nbytes > 0) {
			int ddns_ret;
			ddns_ret = dbuf.oh.cmd_code;
			if (ddns_ret != 0)
				return  ddns_ret;

			break;
		}
	}

	if(strlen(if_node->haipaddr)) {
		struct {
			struct oam_data_st oh;
			struct ha_link_st data;
		}  buf;

		buf.oh.cmd_code = HA_ADDRESS_ADD;
		buf.oh.mod_id = VTYSH_INDEX_HA;
		strncpy(buf.data.ifname , if_node->name, IFNAMSIZ-1);
		strcpy(buf.data.addr_str , if_node->haipaddr );
		write(client_xml[VTYSH_INDEX_HA].fd, &buf, sizeof buf);


		while (1) {
			u32 nbytes;

			nbytes = read (client_xml[VTYSH_INDEX_HA].fd, &buf, sizeof(buf)-1);

			if (nbytes <= 0 && errno != EINTR) {
				break;
			}

			if (nbytes > 0) {
				break;
			}
		}
	} else {
		struct {
			struct oam_data_st oh;
			struct ha_link_st data;
		}  buf;

		buf.oh.cmd_code = HA_ADDRESS_DEL;
		buf.oh.mod_id = VTYSH_INDEX_HA;
		strncpy(buf.data.ifname , if_node->name, IFNAMSIZ-1);
		write(client_xml[VTYSH_INDEX_HA].fd, &buf, sizeof buf);


		while (1) {
			u32 nbytes;

			nbytes = read (client_xml[VTYSH_INDEX_HA].fd, &buf, sizeof(buf)-1);

			if (nbytes <= 0 && errno != EINTR) {
				break;
			}

			if (nbytes > 0) {
				break;
			}
		}
	}

	/*handle shutdown*/
	struct shutdown_st sbuf;
	if (0 == if_node->shut) {
		sbuf.cmd = 2;
		strncpy(sbuf.ifname, if_node->name, IFNAMSIZ-1);
		kernel_request(CTRL_SHUTDOW, (char *)&sbuf, sizeof(sbuf), sizeof(sbuf), 0);
	} else if(1 == if_node->shut) {
		sbuf.cmd = 1;
		strncpy(sbuf.ifname, if_node->name, IFNAMSIZ-1);
		kernel_request(CTRL_SHUTDOW, (char *)&sbuf, sizeof(sbuf), sizeof(sbuf), 0);
	}
	/* handle phy interface commands */
	if(if_node->type == 0) {
		if(if_node->negotiate == 1) {
			struct eth_if_config eth_if_config;

			eth_if_config.eth_if_config_type = ETHIF_CFG_AUTO;
			strncpy(eth_if_config.eth_if_name, if_node->name, IFNAMSIZ-1);
			eth_if_config.eth_if_config_value[ETHIF_CFG_AUTO_OFFSET] = NO_AUTO;
			//xml_eth_if_config_set(if_node->name, &eth_if_config);

			eth_if_config.eth_if_config_type = ETHIF_CFG_SPEED;
			strncpy(eth_if_config.eth_if_name, if_node->name, IFNAMSIZ-1);
			if(if_node->speed == 10)
				eth_if_config.eth_if_config_value[ETHIF_CFG_SPEED_OFFSET] = SPEED10;
			else if(if_node->speed == 100)
				eth_if_config.eth_if_config_value[ETHIF_CFG_SPEED_OFFSET] = SPEED100;
			else
				eth_if_config.eth_if_config_value[ETHIF_CFG_SPEED_OFFSET] = SPEED1000;

			//xml_eth_if_config_set(if_node->name, &eth_if_config);

			eth_if_config.eth_if_config_type = ETHIF_CFG_DUPLEX;
			strncpy(eth_if_config.eth_if_name, if_node->name, IFNAMSIZ-1);
			if(if_node->half  == 0)
				eth_if_config.eth_if_config_value[ETHIF_CFG_DUPLEX_OFFSET] = DUPLEX;
			else
				eth_if_config.eth_if_config_value[ETHIF_CFG_DUPLEX_OFFSET] = HAFLEX;

			//xml_eth_if_config_set(if_node->name, &eth_if_config);
		} else {
			struct eth_if_config eth_if_config;

			eth_if_config.eth_if_config_type = ETHIF_CFG_AUTO;
			strncpy(eth_if_config.eth_if_name, if_node->name, IFNAMSIZ-1);
			eth_if_config.eth_if_config_value[ETHIF_CFG_AUTO_OFFSET] = AUTO;
			//xml_eth_if_config_set(if_node->name, &eth_if_config);
		}
	}

	/*handle local access*/
	struct allowaccess_st abuf;
	memset(&abuf, 0, sizeof(struct allowaccess_st));
	abuf.cmd = 2;
	strncpy(abuf.ifname, if_node->name,IFNAMSIZ-1);
	abuf.access_type = 0xffff;
	kernel_request(CTRL_ALLOWACCESS, (char *)&abuf, sizeof(abuf), sizeof(abuf), 0);
	abuf.cmd = 1;
	abuf.access_type = 0;
	if(if_node->http)
		abuf.access_type |= IFF_ACCESS_HTTP;
	if(if_node->https)
		abuf.access_type |= IFF_ACCESS_HTTPS;
	if(if_node->telnet)
		abuf.access_type |= IFF_ACCESS_TELNET;
	if(if_node->ssh)
		abuf.access_type |= IFF_ACCESS_SSH;
	if(if_node->ping)
		abuf.access_type |= IFF_ACCESS_PING;
	if(if_node->l2tp)
		abuf.access_type |= IFF_ACCESS_L2TP;
	if(if_node->sslvpn)
		abuf.access_type |= IFF_ACCESS_SSLVPN;
	if(if_node->webauth)
		abuf.access_type |= IFF_ACCESS_WEBAUTH;
	if(if_node->cen_monitor)
		abuf.access_type |= IFF_ACCESS_CENMONI;
	kernel_request(CTRL_ALLOWACCESS, (char *)&abuf, sizeof(abuf), sizeof(abuf), 0);

	/* handle link-bind */
	if(if_node->bind_if_enable !=2) {
		if(strlen(if_node->if_name) != 0) { /* ???ñ??󶨽ӿڵ???ʵ???? */
			char if_real_name[IFNAMSIZ +1];
			memset(if_real_name,0,IFNAMSIZ+1);
			if_get_name_by_alias(if_node->if_name,if_real_name);
			if(strlen(if_real_name)!=0) {
				strncpy(if_node->if_name, if_real_name, IFNAMSIZ);
			}
		}
		if((if_node->bind_if_enable==1) && (if_node->bind_type==0)
				&& (strncmp(if_node->real_name, if_node->if_name, IFNAMSIZ)==0)) { /* ?ӿڲ??ܺ??Լ????? */
			return 463;
		}
		if(if_adminflags_get(if_node->real_name) & IFF_LINK_BIND) { /* ?Ѿ??󶨹???Ӧ????????ԭ?????? */
			struct link_bind_cfg cfg;
			//vsos_debug_out("XML-DEBUG: CHECK-BINDED\r\n");
			memset(&cfg,0,sizeof(struct link_bind_cfg));
			strncpy (cfg.if_name, if_node->real_name, IFNAMSIZ - 1);
			link_bind_config(LINK_BIND_NO_MONIADDR, &cfg);

			kernel_request_ext(CTRL_GET_BIND_IF, NULL, 0, 0, NLM_F_DUMP, if_node->real_name, bind_L2_show);
			if(linkbind_if==1) {
				ctrl_bind_interface(0, if_node->real_name, linkbind_name);
				if_adminflags_unset(linkbind_name, IFF_LINK_BIND);
			}

			if_adminflags_unset(if_node->real_name, IFF_LINK_BIND);
		}
		if(if_node->bind_if_enable == 1) { /* д?????? */
			if(0==if_node->bind_type) {
				struct bind_interface_st sbuf;
				memset(&sbuf, 0, sizeof(struct bind_interface_st));
				sbuf.cmd = 1;
				strncpy(sbuf.ifname1,if_node->real_name, IFNAMSIZ);
				strncpy(sbuf.ifname2,if_node->if_name, IFNAMSIZ);
				kernel_request(CTRL_SET_BIND_IF, (char *)&sbuf, sizeof(sbuf), sizeof(sbuf), 0);
				if_adminflags_set(if_node->real_name, IFF_LINK_BIND);
				if_adminflags_set(if_node->if_name, IFF_LINK_BIND);
			} else if(1==if_node->bind_type) {
				struct link_bind_cfg cfg_2;
				memset(&cfg_2,0,sizeof(struct link_bind_cfg));
				strncpy(cfg_2.if_name, if_node->real_name, IFNAMSIZ);
				strncpy(cfg_2.addr_name, if_node->monitor_addr, XML_MAX_NAME_LEN);
				link_bind_config (LINK_BIND_MONIADDR, &cfg_2);
				if_adminflags_set(if_node->real_name, IFF_LINK_BIND);
			}
		}
	}

	/*mtu*/
	if(if_node->mtu)
		if_mtu_set(if_node->name,if_node->mtu);

	/*interface bandwidth*/
	//if( if_node->if_bandwidth )
	if_bandwidth_set(if_node->name, if_node->if_bandwidth);

	/*desc*/
	if(strlen(if_node->desc))
		if_desc_set(if_node->name,if_node->desc);
	else
		if_desc_unset(if_node->name);

	/*second_ip*/
	{
		int ret = 0;
		struct {
			struct oam_data_st oh;
			struct zebra_ip_addr_st data;
		}  buf;
		memset(&buf, 0, sizeof buf);
		buf.oh.cmd_code = ZEBRA_DEL_ADDRESS;
		buf.oh.mod_id = 0;
		strncpy(buf.data.ifname , if_node->name, XML_MAX_NAME_LEN);
		buf.data.addr_type = 1;
		write(client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof buf);

		while (1) {
			int nbytes;

			nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof(buf)-1);

			if (nbytes <= 0 && errno != EINTR) {
				return 0;
			}

			if (nbytes > 0) {
				ret = buf.oh.cmd_code;
				if (ret != 0)
					return  buf.oh.cmd_code;
				break;
			}
		}
	}

	if(if_node->sec_ip) {
		int ret = 0;
		struct {
			struct oam_data_st oh;
			struct zebra_ip_addr_st data;
		}  buf;
		struct second_ip *sec_ip;
		/* ?????ӿ??Ƿ???????bridge or zone */
		if(if_adminflags & (IFF_USEDBY_BR))
			return 140;
		if(if_adminflags & (IFF_USEDBY_TRUNK))
			return 440;
		sec_ip = if_node->sec_ip;
		while(sec_ip) {
			memset(&buf, 0, sizeof buf);
			buf.oh.cmd_code = ZEBRA_ADD_ADDRESS;
			buf.oh.mod_id = 0;
			strncpy(buf.data.ifname , if_node->name, XML_MAX_NAME_LEN);
			strncpy(buf.data.addr_str , sec_ip->second_ip, XML_IP_LEN );
			buf.data.addr_type = 1;
			write(client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof buf);

			while (1) {
				int nbytes;

				nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof(buf)-1);

				if (nbytes <= 0 && errno != EINTR) {
					return 0;
				}

				if (nbytes > 0) {
					ret = buf.oh.cmd_code;
					if (ret != 0)
						return  buf.oh.cmd_code;
					break;
				}
			}
			sec_ip = sec_ip->next;
		}
	}
#endif /*OLD_PLAT*/

	return 0;
}

unsigned long xml_interface_node_del(void *ptr)
{
	assert (NULL != ptr);
	struct interface_node *if_node = ptr;
#if 0 //#ifdef CONFIG_VSYS
	unsigned int vsysid = 0;
	enum vsys_ui_type ui_type;

	if( xml_vsys_gui_get_current_uitype(&ui_type) < 0 ) {
		vsos_debug_out("xml_interface_node_showall: get vsys_ui_type failed\r\n");
		return 535;
	}
#endif
	/*handle vlan del*/
	if(if_node->type == 1
#ifdef CONFIG_TRUNK
			|| if_node->type==2
#endif
	  ) {
		int ret;
		if(interface_find_proc(if_node->name) == 1) {
			ret = if_used_check(if_node->name);
			if(ret) {
				return ret;
			}
			if_shutdown(if_node->name);
			if_address_config_clear(if_node->name);
#if 0 //#ifdef CONFIG_VSYS
			if(ui_type == VSYS_UI_SYSTEM_CONFIG) {
				vsysid = (unsigned int)if_vsys_get(if_node->name);
				if( vsysid != 0 )
					ret = xml_vsys_del_if(vsysid,if_node->name);
				if( ret )
					return ret;
			}
#endif
			ret=vlan_device_del_ioctl(if_node->name);
			if(ret)
				return ret;
		}
	}
	isp_route_node_update(if_node->name);
	return 0;
}

int  pppoe_show_state(char *ifname, int fd, struct pppoe_xml *pppoe)
{
	struct {
		struct oam_data_st oh;
		struct pppoe_xml data;
	}  buf;

	memset(&buf, 0, sizeof buf);
	buf.oh.cmd_code = PPPOE_XML_SHOW;
	buf.oh.mod_id = 0;
	strncpy(buf.data.ifname , ifname, IFNAMSIZ-1);
	write(fd, &buf, sizeof buf);

	while(1) {
		int nbytes;

		nbytes = read (fd, &buf, sizeof(buf));

		if (nbytes <= 0 && errno != EINTR) {
			return -1;
		}

		if (nbytes > 0) {
			break;
		}
	}
	memcpy(pppoe, &buf.data, sizeof(struct pppoe_xml));

	return 0;
}

int bos_dev_ifconfig_down(void *name, unsigned int cmd)
{	
	int ret;
	char *ptr1 = NULL;
	char *ptr2 = NULL;
	char pData[128];
	
	if (cmd != BOS_SIOETHIFSHUTDOWN && cmd != BOS_SIOETHIFNOSHUTDOWN)
		return 0;

	memset(pData, 0, 128*sizeof(char));
	__bos_dev_ioctl(name, BOS_SIOETHIFGETVER, pData);
	ptr1 = strstr(pData, "virtio");
	ptr2 = strstr(pData, "vmxnet3");

	ret = check_vm_mode();
	if (ret && (ptr1 != NULL || ptr2 != NULL)) {/*in vm mode && dev is virtio or vmxnet3*/
		if (cmd == BOS_SIOETHIFSHUTDOWN)
			zebra_flags_set(name, client_xml[VTYSH_INDEX_ZEBRA].fd, 1, 0);
		else if (cmd == BOS_SIOETHIFNOSHUTDOWN)
			zebra_flags_set(name, client_xml[VTYSH_INDEX_ZEBRA].fd, 1, 1);
	}

	return 0;
}

int bos_dev_ioctl(void *name, unsigned int cmd, void *pData)
{
	struct ifreq ifr;
	int fd;
	int error;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		return CMD_ERR_AMBIGUOUS;
	}

	bzero(&ifr, sizeof(ifr));
	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if(pData != NULL) {
		ifr.ifr_data = pData;
	}
	error = ioctl(fd, cmd, &ifr);
	if (error) {
		close(fd);
		return CMD_ERR_NOTHING_TODO;
	}

	close(fd);
	bos_dev_ifconfig_down(name, cmd);

	return 0;
}


/***********tb new************/
int xml_tb_interface_ipv4_fill(xmlNode *cur, char *if_name)
{
	xmlNode *cur1;
	int ret = 0;
	struct {
		struct oam_data_st oh;
		struct zebra_ip_addr_st data;
	}  buf;

	struct prefix prefix;
	char tmpbuf[32];

	memset(&buf, 0, sizeof buf);
	buf.oh.cmd_code = ZEBRA_GET_ADDRESS_ALL;
	buf.oh.mod_id = VTYSH_INDEX_ZEBRA;
	strncpy(buf.data.ifname , if_name, XML_MAX_NAME_LEN);

	write(client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof buf);

	while (1) {
		int nbytes;

		nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof(buf));

		if (nbytes <= 0 && errno != EINTR) {
			break;
		}
		if(buf.oh.cmd_code == 2) {
			cur1 = vsos_xmlNewChild(cur, NULL, BAD_CAST"group", BAD_CAST NULL);

			memset(&prefix, 0, sizeof(struct prefix));
			memset(tmpbuf, 0, 32);
			memcpy(&prefix, buf.data.addr_str, sizeof(struct prefix));
			prefix2str(&prefix, tmpbuf,32);
			vsos_xmlNewChild(cur1, NULL, BAD_CAST"tb_ip", BAD_CAST tmpbuf);

			memset(tmpbuf, 0, 32);
			sprintf(tmpbuf, "%d", buf.data.addr_type==2?1:0);
			vsos_xmlNewChild(cur1, NULL, BAD_CAST"is_floating_ip", BAD_CAST tmpbuf);
			memset(tmpbuf, 0, 32);
			sprintf(tmpbuf, "%d", buf.data.ha_unitID);
			vsos_xmlNewChild(cur1, NULL, BAD_CAST"unit_id", BAD_CAST tmpbuf);

			vsos_xmlNewChild(cur1, NULL, BAD_CAST"tb_ip_version", BAD_CAST "4");

			continue;
		}
		if (nbytes > 0) {
			break;
		}
	}

	return 0;
}
int xml_tb_interface_ipv6_fill(xmlNode *cur, char *if_name)
{
	int ret = 0;
	xmlNode *cur1;
	struct {
		struct oam_data_st oh;
		struct zebra_ip_addr_st data;
	}  buf;

	struct prefix prefix;
	char tmpbuf[32];

	memset(&buf, 0, sizeof buf);
	buf.oh.cmd_code = ZEBRA_GET_ADDRESS_ALL_IPV6;
	buf.oh.mod_id = VTYSH_INDEX_ZEBRA;
	strncpy(buf.data.ifname , if_name, XML_MAX_NAME_LEN);

	write(client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof buf);

	while (1) {
		int nbytes;

		nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof(buf));

		if (nbytes <= 0 && errno != EINTR) {
			break;
		}
		if(buf.oh.cmd_code == 2) {
			cur1= vsos_xmlNewChild(cur, NULL, BAD_CAST"group", BAD_CAST NULL);

			struct prefix prefix;
			char rtbuf[64];
			char ipbuf[64];

			memcpy(&prefix, buf.data.addr_str, sizeof(struct prefix));
			memset(&rtbuf, 0, sizeof(rtbuf));
			memset(&ipbuf, 0, sizeof(ipbuf));
			snprintf (rtbuf, 63, "%s/%d", inet_ntop (AF_INET6, &prefix.u.prefix6, ipbuf, INET6_ADDRSTRLEN), prefix.prefixlen);

			vsos_xmlNewChild(cur1, NULL, BAD_CAST"tb_ip", BAD_CAST rtbuf);

			memset(tmpbuf, 0, 32);
			sprintf(tmpbuf, "%d", buf.data.addr_type==2?1:0);
			vsos_xmlNewChild(cur1, NULL, BAD_CAST"is_floating_ip", BAD_CAST tmpbuf);
			memset(tmpbuf, 0, 32);
			sprintf(tmpbuf, "%d", buf.data.ha_unitID);
			vsos_xmlNewChild(cur1, NULL, BAD_CAST"unit_id", BAD_CAST tmpbuf);

			vsos_xmlNewChild(cur1, NULL, BAD_CAST"tb_ip_version", BAD_CAST "6");

			continue;
		}
		if (nbytes > 0) {
			break;
		}
	}

	return 0;
}

int xml_interface_ip_show
(xmlNode *cur, int address_type, struct interface_info *ptr)
{
	xmlNode *cur1;
	u_char prefixlen;
	char ip_add_str[INET6_ADDRSTRLEN];
	char mask_str[INET6_ADDRSTRLEN];
	char ip_add_mask[sizeof("255.255.255.255/255")];
	char tmpbuf[32];
	struct in_addr mask;
	int ret = 0;

	if (ptr->has_ip) {
		if(ptr->addr.sa_family==AF_INET) {
			inet_ntop (ptr->addr.sa_family,
					   ptr->addr.sa_data+2, ip_add_str, sizeof (ip_add_str));

			inet_ntop (ptr->netmask.sa_family,
					   ptr->netmask.sa_data+2, mask_str, sizeof (mask_str));
		}

		ret = inet_aton (mask_str, &mask);
		if (! ret) {
			return 0;
		} else {
			prefixlen = ip_masklen (mask);
			sprintf(ip_add_mask, "%s/%d", ip_add_str, prefixlen);
		}
	} else {
		return 0;
	}

	cur1 = vsos_xmlNewChild(cur, NULL, BAD_CAST"group", BAD_CAST NULL);
	vsos_xmlNewChild(cur1, NULL, BAD_CAST"tb_ip", BAD_CAST ip_add_mask);
	memset(tmpbuf, 0, 32);
	sprintf(tmpbuf, "%d", address_type==2?1:0);
	vsos_xmlNewChild(cur1, NULL, BAD_CAST"is_floating_ip", BAD_CAST tmpbuf);
	memset(tmpbuf, 0, 32);
	sprintf(tmpbuf, "%d", 0);
	vsos_xmlNewChild(cur1, NULL, BAD_CAST"unit_id", BAD_CAST tmpbuf);
	vsos_xmlNewChild(cur1, NULL, BAD_CAST"tb_ip_version", BAD_CAST "4");

	return 0;
}

int xml_interface_node_info
	(xmlNode *xml_node, struct interface_info ife, int ifvsysid)	
{	
	xmlNode *cur;
	char tmp_name[64];
	struct ethtool_cmd ecmd;
	int address_type;
	char buf[1024 + 1];
	int ip_set_ability = 1;
	int mtu_set_ability = 1;
	xmlNode *cur_tb_ip;
	struct local_access_u abuf;
	int vsysid = GUI_CUR_VSYSID;
	
	cur = vsos_xmlNewChild( xml_node, NULL, BAD_CAST"group", BAD_CAST NULL );

	memset(tmp_name, 0, 64);
	if_get_alias_by_name(ife.name, tmp_name);
	vsos_xmlNewChild( cur, NULL, BAD_CAST"name", BAD_CAST ife.name);
	vsos_xmlNewChild( cur, NULL, BAD_CAST"real_name", BAD_CAST ife.name);
	vsos_xmlNewChild( cur, NULL, BAD_CAST"alias_name", BAD_CAST tmp_name/*ife.name*/);

	ecmd.cmd = ETHTOOL_GSET;
	bos_dev_ioctl(ife.name, SIOCETHTOOL, &ecmd);
	if(ecmd.port == PORT_FIBRE) { //???ڲ??????÷???Э??
		vsos_xmlNewChild( cur, NULL, BAD_CAST"hw_type", BAD_CAST "2");
	} else {
		vsos_xmlNewChild( cur, NULL, BAD_CAST"hw_type", BAD_CAST "1");
	}

	address_type = if_address_type_get(ife.name, client_xml[VTYSH_INDEX_ZEBRA].fd);
	if (address_type == 0)
		address_type = 1; //static
	{
		buf[0] = 0;
		snprintf(buf, 9, "%d", address_type);
		vsos_xmlNewChild( cur, NULL, BAD_CAST"address_type", BAD_CAST buf);
	}

	if(address_type == 3) {
		struct pppoe_xml pppoe;
		pppoe_show_state(ife.name, client_xml[VTYSH_INDEX_PPPOE].fd, &pppoe);
		vsos_xmlNewChild( cur, NULL, BAD_CAST"pppoe_user", BAD_CAST pppoe.username);
		vsos_xmlNewChild( cur, NULL, BAD_CAST"pppoe_passwd", BAD_CAST pppoe.pswd);
		vsos_xmlNewChild( cur, NULL, BAD_CAST"pppoe_specify_ip", BAD_CAST pppoe.pppoe_specify_ip);

		buf[0] = 0;
		snprintf(buf, 9, "%d", pppoe.pppoe_default_gate);
		vsos_xmlNewChild( cur, NULL, BAD_CAST"pppoe_default_gate", BAD_CAST buf);

		buf[0] = 0;
		snprintf(buf, 9, "%d", pppoe.pppoe_dns);
		vsos_xmlNewChild( cur, NULL, BAD_CAST"pppoe_dns", BAD_CAST buf);

		buf[0] = 0;
		snprintf(buf, 9, "%d", pppoe.pppoe_distance);
		vsos_xmlNewChild( cur, NULL, BAD_CAST"pppoe_distance", BAD_CAST buf);

		buf[0] = 0;
		snprintf(buf, 9, "%d", pppoe.pppoe_weight);
		vsos_xmlNewChild( cur, NULL, BAD_CAST"pppoe_weight", BAD_CAST buf);

		mtu_set_ability = 0;
	}

	if(address_type == 2) { /*dhcp ip config*/
		struct {
			struct oam_data_st oh;
			char sbuf[VTY_READ_BUFSIZ];
		}  cmd;

		memset(&cmd, 0, sizeof cmd);
		cmd.oh.cmd_code = DHCP_IF_CONFIG_DUMP;
		cmd.oh.mod_id = VTYSH_INDEX_DHCP;
		strncpy(cmd.sbuf, ife.name, IFNAMSIZ);
		write(client_xml[VTYSH_INDEX_DHCP].fd, &cmd, sizeof (struct oam_data_st) + IFNAMSIZ);

		while (1) {
			int nbytes;

			nbytes = read (client_xml[VTYSH_INDEX_DHCP].fd, &cmd, sizeof(struct oam_data_st)+sizeof(struct if_dhcp_config));

			if (nbytes <= 0 && errno != EINTR) {
				break;
			}

			if (nbytes > 0) {
				if(cmd.oh.cmd_code == DHCP_IF_CONFIG_DUMP) {
					if(nbytes >= sizeof(struct oam_data_st)) {
						cmd.sbuf[nbytes - sizeof(struct oam_data_st)] = 0;
						struct if_dhcp_config *ifd = (struct if_dhcp_config *)cmd.sbuf;

						if (ifd->flags & DHCPC_CFLAG_ENABLE) {
							buf[0] = 0;
							snprintf(buf, 9, "%d", ifd->metric);
							vsos_xmlNewChild( cur, NULL, BAD_CAST"dhcp_distance", BAD_CAST buf);
							if (ifd->flags & DHCPC_CFLAG_GW_RESET)
								vsos_xmlNewChild( cur, NULL, BAD_CAST"dhcp_default_gate", BAD_CAST "1");
							else
								vsos_xmlNewChild( cur, NULL, BAD_CAST"dhcp_default_gate", BAD_CAST "0");
							if (ifd->flags & DHCPC_CFLAG_DNS_RESET)
								vsos_xmlNewChild( cur, NULL, BAD_CAST"dhcp_dns", BAD_CAST "1");
							else
								vsos_xmlNewChild( cur, NULL, BAD_CAST"dhcp_dns", BAD_CAST "0");
						}
					}
					continue;
				}
				break;
			}
		}
	}


	memset(buf, 0, 10);
	snprintf(buf, 9, "%d", ife.shutdown);
	vsos_xmlNewChild( cur, NULL, BAD_CAST"shut", BAD_CAST buf);

	/***************TB new******************/
	{
		char buf_temp[50];
		memset(buf_temp, 0, 50);
		get_str_from_macaddr((unsigned char *)ife.hwaddr,buf_temp);
		vsos_xmlNewChild( cur, NULL, BAD_CAST"hw_addr", BAD_CAST buf_temp);
	}

	if(ife.type == ARPHRD_ETHER) {
		/*autoneg ability*/
		char *ptr1 = NULL;
		char *ptr2 = NULL;
		char pData[128];
		
		memset(pData, 0, 128*sizeof(char));
		__bos_dev_ioctl(ife.name, BOS_SIOETHIFGETVER, pData);
		ptr1 = strstr(pData, "virtio");
		ptr2 = strstr(pData, "vmxnet3");
		
		if (ptr1 != NULL || ptr2 != NULL) {/*in vm mode && dev is virtio or vmxnet3*/
			vsos_xmlNewChild( cur, NULL, BAD_CAST"autoneg_ability", BAD_CAST "0");
		} else {
			if (ife.media_type == PORT_FIBRE)
				vsos_xmlNewChild( cur, NULL, BAD_CAST"autoneg_ability", BAD_CAST "0");
			else
				vsos_xmlNewChild( cur, NULL, BAD_CAST"autoneg_ability", BAD_CAST "1");
		}
		
		if(ife.media_type != PORT_FIBRE) {
			if(ife.autoneg==AUTONEG_ENABLE) {
				vsos_xmlNewChild( cur, NULL, BAD_CAST"negotiate", BAD_CAST "0");
			} else {
				vsos_xmlNewChild( cur, NULL, BAD_CAST"negotiate", BAD_CAST "1");
			}
		} else {
			vsos_xmlNewChild( cur, NULL, BAD_CAST"negotiate", BAD_CAST "0");
		}

		if(((ife.flags&IFF_RUNNING) && (ife.autoneg == AUTONEG_ENABLE)) || (ife.autoneg == AUTONEG_DISABLE)) {
			if(ife.speed == SPEED10000)
				vsos_xmlNewChild( cur, NULL, BAD_CAST"speed", BAD_CAST "10000");
			else if(ife.speed == SPEED1000)
				vsos_xmlNewChild( cur, NULL, BAD_CAST"speed", BAD_CAST "1000");
			else if(ife.speed == SPEED100)
				vsos_xmlNewChild( cur, NULL, BAD_CAST"speed", BAD_CAST "100");
			else if(ife.speed == SPEED10)
				vsos_xmlNewChild( cur, NULL, BAD_CAST"speed", BAD_CAST "10");

			if(ife.duplex) {
				vsos_xmlNewChild( cur, NULL, BAD_CAST"half", BAD_CAST "1");
			} else {
				vsos_xmlNewChild( cur, NULL, BAD_CAST"half", BAD_CAST "0");
			}
		} else {
			vsos_xmlNewChild( cur, NULL, BAD_CAST"speed", BAD_CAST "-1");
			vsos_xmlNewChild( cur, NULL, BAD_CAST"half", BAD_CAST "-1");
		}
	}

	if(ife.flags&IFF_RUNNING) {
		vsos_xmlNewChild( cur, NULL, BAD_CAST"status", BAD_CAST "1");
	} else {
		vsos_xmlNewChild( cur, NULL, BAD_CAST"status", BAD_CAST "0");
	}

	{
		char countbuf[16]= {0};
		int vlancount = xml_interface_node_vlan_count(ife.name);
		snprintf(countbuf, 16, "%d", vlancount);
		vsos_xmlNewChild( cur, NULL, BAD_CAST"vlan_count", BAD_CAST countbuf);
		if (vlancount) {
			ip_set_ability = 0;
			mtu_set_ability = 0;
		}
	}

	{
		char trunknamebuf[64]= {0};
		if(tr_get_if_config(ife.name, trunknamebuf)) {
			vsos_xmlNewChild( cur, NULL, BAD_CAST"of_trunk", BAD_CAST trunknamebuf);
			ip_set_ability = 0;
			mtu_set_ability = 0;
		}
	}
	
	{
		char ability_buf[16]= {0};
		snprintf(ability_buf, 16, "%d", ip_set_ability);
		vsos_xmlNewChild( cur, NULL, BAD_CAST"ip_set_ability", BAD_CAST ability_buf);
	}
	{
		char ability_buf[16]= {0};
		snprintf(ability_buf, 16, "%d", mtu_set_ability);
		vsos_xmlNewChild( cur, NULL, BAD_CAST"mtu_set_ability", BAD_CAST ability_buf);
	}
	
	cur_tb_ip = vsos_xmlNewChild( cur, NULL, BAD_CAST"tb_interface_ip", BAD_CAST NULL);
	if(address_type == 3 || address_type == 2) {
		xml_interface_ip_show(cur_tb_ip, address_type, &ife);
	} else {
		xml_tb_interface_ipv4_fill(cur_tb_ip, ife.name);
		xml_tb_interface_ipv6_fill(cur_tb_ip, ife.name);
	}

	{
		char accessbuf[10] = {0};
		memset(&abuf, 0, sizeof(abuf));
		abuf.cmd = LOCAL_ACCESS_CMD_GET;
		strncpy(abuf.ifname, ife.name, IFNAMSIZ);
		kernel_request(CTRL_ALLOWACCESS, (char *)&abuf, sizeof(abuf), sizeof(abuf), 0);
		dataplane_request(UIPC_DEV_ALLOWACCESS,(char *)&abuf, sizeof(abuf), sizeof(abuf), 0);	

		memset(accessbuf, 0, 10);
		if(abuf.access_type & IFF_ACCESS_HTTP)
			sprintf(accessbuf, "%d", 1);
		else
			sprintf(accessbuf, "%d", 0);
		vsos_xmlNewChild(cur, NULL, BAD_CAST"http", BAD_CAST accessbuf);

		memset(accessbuf, 0, 10);
		if(abuf.access_type & IFF_ACCESS_HTTPS)
			sprintf(accessbuf, "%d", 1);
		else
			sprintf(accessbuf, "%d", 0);
		vsos_xmlNewChild(cur, NULL, BAD_CAST"https", BAD_CAST accessbuf);

		memset(accessbuf, 0, 10);
		if(abuf.access_type & IFF_ACCESS_PING)
			sprintf(accessbuf, "%d", 1);
		else
			sprintf(accessbuf, "%d", 0);
		vsos_xmlNewChild(cur, NULL, BAD_CAST"ping", BAD_CAST accessbuf);

		memset(accessbuf, 0, 10);
		if(abuf.access_type & IFF_ACCESS_TELNET)
			sprintf(accessbuf, "%d", 1);
		else
			sprintf(accessbuf, "%d", 0);
		vsos_xmlNewChild(cur, NULL, BAD_CAST"telnet", BAD_CAST accessbuf);

		memset(accessbuf, 0, 10);
		if(abuf.access_type & IFF_ACCESS_SSH)
			sprintf(accessbuf, "%d", 1);
		else
			sprintf(accessbuf, "%d", 0);
		vsos_xmlNewChild(cur, NULL, BAD_CAST"ssh", BAD_CAST accessbuf);

		memset(accessbuf, 0, 10);
		if(abuf.access_type & IFF_ACCESS_SSLVPN)
			sprintf(accessbuf, "%d", 1);
		else
			sprintf(accessbuf, "%d", 0);
		vsos_xmlNewChild(cur, NULL, BAD_CAST"sslvpn", BAD_CAST accessbuf);

		memset(accessbuf, 0, 10);
		if(abuf.access_type & IFF_ACCESS_BGP)
			sprintf(accessbuf, "%d", 1);
		else
			sprintf(accessbuf, "%d", 0);
		vsos_xmlNewChild(cur, NULL, BAD_CAST"bgp", BAD_CAST accessbuf);

		memset(accessbuf, 0, 10);
		if(abuf.access_type & IFF_ACCESS_OSPF)
			sprintf(accessbuf, "%d", 1);
		else
			sprintf(accessbuf, "%d", 0);
		vsos_xmlNewChild(cur, NULL, BAD_CAST"ospf", BAD_CAST accessbuf);

		memset(accessbuf, 0, 10);
		if(abuf.access_type & IFF_ACCESS_RIP)
			sprintf(accessbuf, "%d", 1);
		else
			sprintf(accessbuf, "%d", 0);
		vsos_xmlNewChild(cur, NULL, BAD_CAST"rip", BAD_CAST accessbuf);

		memset(accessbuf, 0, 10);
		if(abuf.access_type & IFF_ACCESS_DNS)
			sprintf(accessbuf, "%d", 1);
		else
			sprintf(accessbuf, "%d", 0);
		vsos_xmlNewChild(cur, NULL, BAD_CAST"dns", BAD_CAST accessbuf);

		memset(accessbuf, 0, 10);
		if(abuf.access_type & IFF_ACCESS_TCTRL)
			sprintf(accessbuf, "%d", 1);
		else
			sprintf(accessbuf, "%d", 0);
		vsos_xmlNewChild(cur, NULL, BAD_CAST"tctrl", BAD_CAST accessbuf);

		memset(accessbuf, 0, 10);
		if(abuf.access_type & IFF_ACCESS_WEBAUTH)
			sprintf(accessbuf, "%d", 1);
		else
			sprintf(accessbuf, "%d", 0);
		vsos_xmlNewChild(cur, NULL, BAD_CAST"webauth", BAD_CAST accessbuf);

		memset(accessbuf, 0, 10);
		if(abuf.access_type & IFF_ACCESS_LINKAGE)
			sprintf(accessbuf, "%d", 1);
		else
			sprintf(accessbuf, "%d", 0);
		vsos_xmlNewChild(cur, NULL, BAD_CAST"linkage", BAD_CAST accessbuf);
	}

	/*mtu*/
	if(ife.mtu > 0) {
		memset(buf, 0, 10);
		snprintf(buf, 9, "%d", ife.mtu);
		vsos_xmlNewChild( cur, NULL, BAD_CAST"mtu", BAD_CAST buf);
	}

	/* external & internal */
	struct ifdev_info dev_info;
	memset(&dev_info, 0, sizeof(dev_info));
	strncpy(dev_info.ifname, ife.name, IFNAMSIZ);
	dataplane_request(UIPC_DEV_GET_MODE, (char *)&dev_info,
					  sizeof(dev_info), sizeof(dev_info), 0);
	if (dev_info.dev_type == IFDEV_ETH_TYPE) {
		memset(buf, 0, sizeof(buf));
		if (dev_info.adminflags & IFF_EXTERNAL)
			snprintf(buf, sizeof(buf), "%d", 1);
		else
			snprintf(buf, sizeof(buf), "%d", 0);
		vsos_xmlNewChild( cur, NULL, BAD_CAST"external", BAD_CAST buf);
	}

	/*vsys id*/
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "%d", ifvsysid);
	vsos_xmlNewChild(cur, NULL, BAD_CAST "ifvsysid", BAD_CAST buf);

	if (ifvsysid == vsysid)
		vsos_xmlNewChild( cur, NULL, BAD_CAST"vsysid_same", BAD_CAST "1");
	else
		vsos_xmlNewChild( cur, NULL, BAD_CAST"vsysid_same", BAD_CAST "0");
			
	/***************TB new******************/

return 0;
}


char *xml_interface_node_showall(void *ptr)
{
	xmlNode *cur,*cur_2=NULL, *cur_3 = NULL;
	xmlNode *child;
	char *ifname;
	char * send_buf;
	FILE *fp;
	char buf[1024 + 1];
	int ret, if_bandwidth;
	struct interface_info ife;
	struct interface_node *if_node = NULL;
	struct local_access_u abuf;
	xmlDoc *global_doc ;
	int vsysid = GUI_CUR_VSYSID;
	int ifvsysid;
	char tmp_name[64];
	int total = 0;
	int count = 0;
	
	const struct gui_xml_attr *sattr = gui_get_attr(ptr);
	int page_count = sattr->count;

#if 0 //#ifdef CONFIG_VSYS
	unsigned int vsysid,ifvsysid;
	enum vsys_ui_type ui_type;

	if( xml_vsys_gui_get_current_uitype(&ui_type) < 0 ) {
		vsos_debug_out("xml_interface_node_showall: get vsys_ui_type failed\r\n");
		return NULL;
	}
	if( ui_type == VSYS_UI_SYSTEM_CONFIG )
		vsysid = ~0;
	else {
		if(xml_vsys_gui_get_current_vsysid(&vsysid) < 0 ) {
			vsos_debug_out("xml_interface_node_showall: get vsysid failed\r\n");
			return NULL;
		}
	}
#endif

	if_node = (struct interface_node *)ptr;
	ifname = if_node->name;

	/* Open /proc/net/dev. */
	ret = 0;
	int ver =0;
	fp = fopen (_PATH_PROC_NET_DEV, "r");
	if (fp == NULL)
		return NULL;

	/* Drop header lines. */
	fgets (buf, 1024, fp);
	fgets (buf, 1024, fp);
	ver =procnetdev_version(buf);

	child = xml_start_more(&global_doc, "interface_sub");

	while (fgets (buf, 1024, fp) != NULL) {
		buf[1024] = '\0';
		char *s, name[IFNAMSIZ];
		s = get_name(name, buf);
		if(strlen(ifname)==0 ||!strcmp(name,ifname)) {
			char buf[50];
			struct prefix addr;
			int iftype;
			strncpy(ife.name,name,IFNAMSIZ);
//			get_dev_fields(ver, s, &ife);
			/*
						if(fetch_interface(&ife)<0)
							continue;
			*/
			if(!strncmp(ife.name, "loop", 4))
				continue;
			if(!strncmp(ife.name, "eth", 3))
				continue;
			if(!strncmp(ife.name, "lo", 2))
				continue;
			if(strchr(ife.name, '.'))
				continue;

			/*11 is for qos*/
			if (11 != if_node->type) {
				if(!strncmp(ife.name, "vlan", 4))
					continue;
				if(!strncmp(ife.name, "tvi", 3))
					continue;
			}

			/* 10 is policy(phy & vpn) */
			if (10 != if_node->type) {
				if(!strncmp(ife.name, "gre", 3))
					continue;
				if(!strncmp(ife.name, "tunl", 4))
					continue;
				if(!strncmp(ife.name, "sit", 3))
					continue;
			}

			if(fetch_interface(&ife)<0)
				continue;

             /* Determine whether the current vsys */
            ifvsysid = dp_netns_get_vrfid_by_ifname(name);
            if (ifvsysid != vsysid && vsysid != 0) {
                continue;
            }

			/*sattr->count <= 0 Delegates do not need paging*/
			if ((page_count > 0 && count >= (sattr->page -1)*page_count) || sattr->count <= 0){
				xml_interface_node_info(child, ife, ifvsysid);
				page_count--;
			}
			count++;
			total++;

#if 0 /*Arrangement code by nilijun*/
			cur = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );

#if 0
			if(ife.type != ARPHRD_ETHER)
				continue;
			if(!strncmp(ife.name, "bvi", 3))
				continue;

#ifdef CONFIG_TRUNK
			if((!strncmp(ife.name, "tvi", 3)) && (!strchr(ife.name, '.')))
				continue;
#endif

			if(!strcmp(ife.name, "npec"))
				continue;
			if(!strncmp(ife.name, "swi", 3))
				continue;
			if(!strncmp("nsa", name, 3))
				continue;
			if(!strncmp(ife.name, "swi", 3))
				continue;
			if(!strncmp(ife.name, "loop", 4))
				continue;
#if 0 //#ifdef CONFIG_VSYS
			if((vsysid != ~0)
					&& if_vsys_check(ife.name,vsysid) == 1 ) {
				//vsos_debug_out("xml_interface_node_showall: VSYSID=%d, ifname=%s\r\n",
				//	vsysid,ife.name);
				continue;
			}
#endif
			if(ife.ifHwIntf == 1)
				iftype = 0;
			else
				iftype = 1;

#ifdef CONFIG_TRUNK
			if((!strncmp(ife.name, "tvi", 3)) && (strchr(ife.name, '.') != NULL))
				iftype = 2;
#endif

			cur = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );
			if( iftype==0 )
				vsos_xmlNewChild( cur, NULL, BAD_CAST"type", BAD_CAST "0");
			else
#ifdef CONFIG_TRUNK
				if(iftype == 1)
#endif
				{
					char * dotptr=strchr(ife.name,'.');
					char master_if[IFNAMSIZ]= {0};
					int vlan_id = 0;
					if(dotptr) {
						vlan_id = atoi(dotptr+1);
						strncpy(master_if,ife.name,dotptr-ife.name);
						vsos_xmlNewChild( cur, NULL, BAD_CAST"type", BAD_CAST "1");
						snprintf(buf, 9, "%d",vlan_id);
						vsos_xmlNewChild( cur, NULL, BAD_CAST"vlan_id", BAD_CAST buf);
						vsos_xmlNewChild( cur, NULL, BAD_CAST"vlan_phy_if", BAD_CAST master_if);
					} else
						vsos_xmlNewChild( cur, NULL, BAD_CAST"type", BAD_CAST "0");
				}

#ifdef CONFIG_TRUNK
			if(iftype == 2) {
				char * dotptr=strchr(ife.name,'.');
				char master_if[IFNAMSIZ]= {0};
				int vlan_id = 0;
				if(dotptr) {
					vlan_id = atoi(dotptr+1);
					strncpy(master_if,ife.name,dotptr-ife.name);
					vsos_xmlNewChild( cur, NULL, BAD_CAST"type", BAD_CAST "2");
					snprintf(buf, 9, "%d",vlan_id);
					vsos_xmlNewChild( cur, NULL, BAD_CAST"vlan_id", BAD_CAST buf);
					vsos_xmlNewChild( cur, NULL, BAD_CAST"vlan_phy_if", BAD_CAST master_if);
				} else
					vsos_xmlNewChild( cur, NULL, BAD_CAST"type", BAD_CAST "2");
			}
#endif
#endif/*OLD_FLAG*/

			memset(tmp_name, 0, 64);
			if_get_alias_by_name(name, tmp_name);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"name", BAD_CAST ife.name);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"real_name", BAD_CAST ife.name);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"alias_name", BAD_CAST tmp_name/*ife.name*/);

			ecmd.cmd = ETHTOOL_GSET;
			bos_dev_ioctl(ife.name, SIOCETHTOOL, &ecmd);
			if(ecmd.port == PORT_FIBRE) { //???ڲ??????÷???Э??
				vsos_xmlNewChild( cur, NULL, BAD_CAST"hw_type", BAD_CAST "2");
			} else {
				vsos_xmlNewChild( cur, NULL, BAD_CAST"hw_type", BAD_CAST "1");
			}

			int address_type = if_address_type_get(ife.name, client_xml[VTYSH_INDEX_ZEBRA].fd);
			if (address_type == 0)
				address_type = 1; //static
			{
				buf[0] = 0;
				snprintf(buf, 9, "%d", address_type);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"address_type", BAD_CAST buf);
			}

			if(address_type == 3) {
				struct pppoe_xml pppoe;
				pppoe_show_state(ife.name, client_xml[VTYSH_INDEX_PPPOE].fd, &pppoe);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"pppoe_user", BAD_CAST pppoe.username);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"pppoe_passwd", BAD_CAST pppoe.pswd);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"pppoe_specify_ip", BAD_CAST pppoe.pppoe_specify_ip);

				buf[0] = 0;
				snprintf(buf, 9, "%d", pppoe.pppoe_default_gate);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"pppoe_default_gate", BAD_CAST buf);

				buf[0] = 0;
				snprintf(buf, 9, "%d", pppoe.pppoe_dns);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"pppoe_dns", BAD_CAST buf);

				buf[0] = 0;
				snprintf(buf, 9, "%d", pppoe.pppoe_distance);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"pppoe_distance", BAD_CAST buf);

				buf[0] = 0;
				snprintf(buf, 9, "%d", pppoe.pppoe_weight);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"pppoe_weight", BAD_CAST buf);

				mtu_set_ability = 0;
			}

			if(address_type == 2) { /*dhcp ip config*/
				struct {
					struct oam_data_st oh;
					char sbuf[VTY_READ_BUFSIZ];
				}  cmd;

				memset(&cmd, 0, sizeof cmd);
				cmd.oh.cmd_code = DHCP_IF_CONFIG_DUMP;
				cmd.oh.mod_id = VTYSH_INDEX_DHCP;
				strncpy(cmd.sbuf, ife.name, IFNAMSIZ);
				write(client_xml[VTYSH_INDEX_DHCP].fd, &cmd, sizeof (struct oam_data_st) + IFNAMSIZ);

				while (1) {
					int nbytes;

					nbytes = read (client_xml[VTYSH_INDEX_DHCP].fd, &cmd, sizeof(struct oam_data_st)+sizeof(struct if_dhcp_config));

					if (nbytes <= 0 && errno != EINTR) {
						break;
					}

					if (nbytes > 0) {
						if(cmd.oh.cmd_code == DHCP_IF_CONFIG_DUMP) {
							if(nbytes >= sizeof(struct oam_data_st)) {
								cmd.sbuf[nbytes - sizeof(struct oam_data_st)] = 0;
								struct if_dhcp_config *ifd = (struct if_dhcp_config *)cmd.sbuf;

								if (ifd->flags & DHCPC_CFLAG_ENABLE) {
									buf[0] = 0;
									snprintf(buf, 9, "%d", ifd->metric);
									vsos_xmlNewChild( cur, NULL, BAD_CAST"dhcp_distance", BAD_CAST buf);
									if (ifd->flags & DHCPC_CFLAG_GW_RESET)
										vsos_xmlNewChild( cur, NULL, BAD_CAST"dhcp_default_gate", BAD_CAST "1");
									else
										vsos_xmlNewChild( cur, NULL, BAD_CAST"dhcp_default_gate", BAD_CAST "0");
									if (ifd->flags & DHCPC_CFLAG_DNS_RESET)
										vsos_xmlNewChild( cur, NULL, BAD_CAST"dhcp_dns", BAD_CAST "1");
									else
										vsos_xmlNewChild( cur, NULL, BAD_CAST"dhcp_dns", BAD_CAST "0");
								}
							}
							continue;
						}
						break;
					}
				}
			}

#ifdef OLD_FLAG
			if (get_mgmt_port(ife.name) == 1) {
				vsos_xmlNewChild( cur, NULL, BAD_CAST"mgmt_flag", BAD_CAST "1");
			} else {
				vsos_xmlNewChild( cur, NULL, BAD_CAST"mgmt_flag", BAD_CAST "0");
			}
			int address_type = if_address_type_get(ife.name, client_xml[VTYSH_INDEX_ZEBRA].fd);
			if(address_type) {
				buf[0] = 0;
				snprintf(buf, 9, "%d", address_type);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"address_type", BAD_CAST buf);
			}
			if(address_type == 2) { /*dhcp ip config*/
				struct {
					struct oam_data_st oh;
					char sbuf[VTY_READ_BUFSIZ];
				}  cmd;

				memset(&cmd, 0, sizeof cmd);
				cmd.oh.cmd_code = DHCP_IF_CONFIG_DUMP;
				cmd.oh.mod_id = VTYSH_INDEX_DHCP;
				strncpy(cmd.sbuf, if_node->name, IFNAMSIZ);
				write(client_xml[VTYSH_INDEX_DHCP].fd, &cmd, sizeof (struct oam_data_st) + IFNAMSIZ);

				while (1) {
					int nbytes;

					nbytes = read (client_xml[VTYSH_INDEX_DHCP].fd, &cmd, sizeof(struct oam_data_st)+sizeof(struct if_dhcp_config));

					if (nbytes <= 0 && errno != EINTR) {
						break;
					}

					if (nbytes > 0) {
						if(cmd.oh.cmd_code == DHCP_IF_CONFIG_DUMP) {
							if(nbytes >= sizeof(struct oam_data_st)) {
								cmd.sbuf[nbytes - sizeof(struct oam_data_st)] = 0;
								struct if_dhcp_config *ifd = (struct if_dhcp_config *)cmd.sbuf;

								if (ifd->flags & DHCPC_CFLAG_ENABLE) {
									buf[0] = 0;
									snprintf(buf, 9, "%d", ifd->metric);
									vsos_xmlNewChild( cur, NULL, BAD_CAST"dhcp_distance", BAD_CAST buf);
									if (ifd->flags & DHCPC_CFLAG_GW_RESET)
										vsos_xmlNewChild( cur, NULL, BAD_CAST"dhcp_default_gate", BAD_CAST "1");
									else
										vsos_xmlNewChild( cur, NULL, BAD_CAST"dhcp_default_gate", BAD_CAST "0");
									if (ifd->flags & DHCPC_CFLAG_DNS_RESET)
										vsos_xmlNewChild( cur, NULL, BAD_CAST"dhcp_dns", BAD_CAST "1");
									else
										vsos_xmlNewChild( cur, NULL, BAD_CAST"dhcp_dns", BAD_CAST "0");
								}
							}
							continue;
						}
						break;
					}
				}
			}

			if(ife.has_ip && ((struct sockaddr_in *)(&ife.addr))->sin_addr.s_addr) {
				addr.family = AF_INET;
				addr.u.prefix4.s_addr = ((struct sockaddr_in *)(&ife.addr))->sin_addr.s_addr;
				addr.prefixlen = ip_masklen(((struct sockaddr_in *)(&ife.netmask))->sin_addr);
				prefix2str((struct prefix *)&addr, buf, 50);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"ipaddr", BAD_CAST buf);
			}
			if(address_type == 3) {
				struct pppoe_xml pppoe;
				pppoe_show_state(ife.name, client_xml[VTYSH_INDEX_PPPOE].fd, &pppoe);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"pppoe_user", BAD_CAST pppoe.username);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"pppoe_passwd", BAD_CAST pppoe.pswd);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"pppoe_specify_ip", BAD_CAST pppoe.pppoe_specify_ip);

				buf[0] = 0;
				snprintf(buf, 9, "%d", pppoe.pppoe_default_gate);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"pppoe_default_gate", BAD_CAST buf);

				buf[0] = 0;
				snprintf(buf, 9, "%d", pppoe.pppoe_dns);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"pppoe_dns", BAD_CAST buf);

				buf[0] = 0;
				snprintf(buf, 9, "%d", pppoe.pppoe_distance);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"pppoe_distance", BAD_CAST buf);

				buf[0] = 0;
				snprintf(buf, 9, "%d", pppoe.pppoe_weight);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"pppoe_weight", BAD_CAST buf);
			}

			/* ip ddns */
			struct {
				struct oam_data_st oh;
				char buf[VTY_READ_BUFSIZ];
			}  cmd;

			memset(&cmd, 0, sizeof cmd);
			cmd.oh.cmd_code = DDNS_SHOW;
			cmd.oh.mod_id = VTYSH_INDEX_DDNS;
			strncpy(cmd.buf, if_node->name, IFNAMSIZ);
			write(client_xml[VTYSH_INDEX_DDNS].fd, &cmd, sizeof (struct oam_data_st) + IFNAMSIZ);

			while (1) {
				int nbytes;

				nbytes = read (client_xml[VTYSH_INDEX_DDNS].fd, &cmd, sizeof(struct oam_data_st)+sizeof(struct ddns_cfg));

				if (nbytes <= 0 && errno != EINTR) {
					break;
				}

				if (nbytes > 0) {
					if(cmd.oh.cmd_code == DDNS_SHOW) {
						if(nbytes >= sizeof(struct oam_data_st)) {
							cmd.buf[nbytes - sizeof(struct oam_data_st)] = 0;
							struct ddns_cfg *dc = (struct ddns_cfg *)cmd.buf;

							char idbuf[50];
							memset(idbuf, 0, sizeof(idbuf));
							sprintf(idbuf, "%d", dc->type);
							vsos_xmlNewChild( cur, NULL, BAD_CAST"ddns_type", BAD_CAST idbuf);

							memset(idbuf, 0, sizeof(idbuf));
							sprintf(idbuf, "%d", dc->interval);
							vsos_xmlNewChild( cur, NULL, BAD_CAST"ddns_interval", BAD_CAST idbuf);

							if ((dc->iptype) && (dc->myip)) {
								memset(idbuf, 0, sizeof(idbuf));
								vsos_xmlNewChild( cur, NULL, BAD_CAST"ddns_myip", BAD_CAST ip_ntoa(idbuf, dc->myip));
							} else
								vsos_xmlNewChild( cur, NULL, BAD_CAST"ddns_myip", BAD_CAST NULL);


							vsos_xmlNewChild( cur, NULL, BAD_CAST"ddns_domain", BAD_CAST dc->domain);
							vsos_xmlNewChild( cur, NULL, BAD_CAST"ddns_user", BAD_CAST dc->username);
							vsos_xmlNewChild( cur, NULL, BAD_CAST"ddns_pass", BAD_CAST dc->password);

							memset(idbuf, 0, sizeof(idbuf));
							sprintf(idbuf, "%d", dc->on);
							vsos_xmlNewChild( cur, NULL, BAD_CAST"ddns_on", BAD_CAST idbuf);

							memset(idbuf, 0, sizeof(idbuf));
							sprintf(idbuf, "%d", dc->status);
							vsos_xmlNewChild( cur, NULL, BAD_CAST"ddns_status", BAD_CAST idbuf);

							if (dc->status) {
								memset(idbuf, 0, sizeof(idbuf));
								sprintf(idbuf, "online %d seconds", (int)(vsos_get_uptime() -dc->stime));
								vsos_xmlNewChild( cur, NULL, BAD_CAST"ddns_onlinetime", BAD_CAST idbuf);
							} else
								vsos_xmlNewChild( cur, NULL, BAD_CAST"ddns_onlinetime", BAD_CAST "offline");

						}

						continue;
					}

					if(cmd.oh.cmd_code != 0) {
						break;
					}
					break;
				}
			}
			/* end of ddns */
#endif/*OLD_FLAG*/

			memset(buf, 0, 10);
			snprintf(buf, 9, "%d", ife.shutdown);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"shut", BAD_CAST buf);

#if 0
			if((ife.type == ARPHRD_ETHER)&&(ife.ifHwIntf == 1)) {
				//vsos_debug_out("%s- auto:%d, speed:%d, duplex:%d\r\n", ife.name, ife.autoneg, ife.speed, ife.duplex);

				if(ife.autoneg == AUTO)
					vsos_xmlNewChild( cur, NULL, BAD_CAST"negotiate", BAD_CAST "0");
				else
					vsos_xmlNewChild( cur, NULL, BAD_CAST"negotiate", BAD_CAST "1");

				if(((ife.flags&IFF_PHYUP) && (ife.autoneg == AUTO)) || (ife.autoneg == NO_AUTO)) {
					if(ife.speed == SPEED1000)
						vsos_xmlNewChild( cur, NULL, BAD_CAST"speed", BAD_CAST "1000");
					else if(ife.speed == SPEED100)
						vsos_xmlNewChild( cur, NULL, BAD_CAST"speed", BAD_CAST "100");
					else if(ife.speed == SPEED10)
						vsos_xmlNewChild( cur, NULL, BAD_CAST"speed", BAD_CAST "10");
					if(ife.duplex == DUPLEX)
						vsos_xmlNewChild( cur, NULL, BAD_CAST"half", BAD_CAST "0");
					else
						vsos_xmlNewChild( cur, NULL, BAD_CAST"half", BAD_CAST "1");
				} else if((!(ife.flags&IFF_PHYUP))&&(ife.autoneg == AUTO)) {
					if(ife.max_speed == SPEED10)
						vsos_xmlNewChild( cur, NULL, BAD_CAST"speed", BAD_CAST "10");
					else if(ife.max_speed == SPEED100)
						vsos_xmlNewChild( cur, NULL, BAD_CAST"speed", BAD_CAST "100");
					else
						vsos_xmlNewChild( cur, NULL, BAD_CAST"speed", BAD_CAST "1000");

					vsos_xmlNewChild( cur, NULL, BAD_CAST"half", BAD_CAST "0");
				}

				if(ife.max_speed == SPEED1000)
					vsos_xmlNewChild( cur, NULL, BAD_CAST"max_speed", BAD_CAST "1000");
				else if(ife.max_speed == SPEED100)
					vsos_xmlNewChild( cur, NULL, BAD_CAST"max_speed", BAD_CAST "100");
				else
					vsos_xmlNewChild( cur, NULL, BAD_CAST"max_speed", BAD_CAST "10");

			}
#endif/*OLD_FLAG*/


			/***************TB new******************/
			{
				char buf_temp[50];
				memset(buf_temp, 0, 50);
				get_str_from_macaddr((unsigned char *)ife.hwaddr,buf_temp);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"hw_addr", BAD_CAST buf_temp);
			}

			if(ife.type == ARPHRD_ETHER) {
				if(ife.media_type != PORT_FIBRE) {
					if(ife.autoneg==AUTONEG_ENABLE) {
						vsos_xmlNewChild( cur, NULL, BAD_CAST"negotiate", BAD_CAST "0");
					} else {
						vsos_xmlNewChild( cur, NULL, BAD_CAST"negotiate", BAD_CAST "1");
					}
				} else {
					vsos_xmlNewChild( cur, NULL, BAD_CAST"negotiate", BAD_CAST "0");
				}

				if(((ife.flags&IFF_RUNNING) && (ife.autoneg == AUTONEG_ENABLE)) || (ife.autoneg == AUTONEG_DISABLE)) {
					if(ife.speed == SPEED10000)
						vsos_xmlNewChild( cur, NULL, BAD_CAST"speed", BAD_CAST "10000");
					else if(ife.speed == SPEED1000)
						vsos_xmlNewChild( cur, NULL, BAD_CAST"speed", BAD_CAST "1000");
					else if(ife.speed == SPEED100)
						vsos_xmlNewChild( cur, NULL, BAD_CAST"speed", BAD_CAST "100");
					else if(ife.speed == SPEED10)
						vsos_xmlNewChild( cur, NULL, BAD_CAST"speed", BAD_CAST "10");

					if(ife.duplex) {
						vsos_xmlNewChild( cur, NULL, BAD_CAST"half", BAD_CAST "1");
					} else {
						vsos_xmlNewChild( cur, NULL, BAD_CAST"half", BAD_CAST "0");
					}
				} else {
					vsos_xmlNewChild( cur, NULL, BAD_CAST"speed", BAD_CAST "-1");
					vsos_xmlNewChild( cur, NULL, BAD_CAST"half", BAD_CAST "-1");
				}
			}

			if(ife.flags&IFF_RUNNING) {
				vsos_xmlNewChild( cur, NULL, BAD_CAST"status", BAD_CAST "1");
			} else {
				vsos_xmlNewChild( cur, NULL, BAD_CAST"status", BAD_CAST "0");
			}

			{
				char countbuf[16]= {0};
				int vlancount = xml_interface_node_vlan_count(name);
				snprintf(countbuf, 16, "%d", vlancount);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"vlan_count", BAD_CAST countbuf);
				if (vlancount) {
					ip_set_ability = 0;
					mtu_set_ability = 0;
				}
			}

			{
				char trunknamebuf[64]= {0};
				if(tr_get_if_config(name, trunknamebuf)) {
					vsos_xmlNewChild( cur, NULL, BAD_CAST"of_trunk", BAD_CAST trunknamebuf);
					ip_set_ability = 0;
					mtu_set_ability = 0;
				}
			}
			
			{
				char ability_buf[16]= {0};
				snprintf(ability_buf, 16, "%d", ip_set_ability);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"ip_set_ability", BAD_CAST ability_buf);
			}
			{
				char ability_buf[16]= {0};
				snprintf(ability_buf, 16, "%d", mtu_set_ability);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"mtu_set_ability", BAD_CAST ability_buf);
			}
			
			cur_tb_ip = vsos_xmlNewChild( cur, NULL, BAD_CAST"tb_interface_ip", BAD_CAST NULL);
			if(address_type == 3 || address_type == 2) {
				xml_interface_ip_show(cur_tb_ip, address_type, &ife);
			} else {
				xml_tb_interface_ipv4_fill(cur_tb_ip, name);
				xml_tb_interface_ipv6_fill(cur_tb_ip, name);
			}

			{
				char accessbuf[10] = {0};
				memset(&abuf, 0, sizeof(abuf));
				abuf.cmd = LOCAL_ACCESS_CMD_GET;
				strncpy(abuf.ifname, name, IFNAMSIZ);
				kernel_request(CTRL_ALLOWACCESS, (char *)&abuf, sizeof(abuf), sizeof(abuf), 0);
				dataplane_request(UIPC_DEV_ALLOWACCESS,(char *)&abuf, sizeof(abuf), sizeof(abuf), 0);	

				memset(accessbuf, 0, 10);
				if(abuf.access_type & IFF_ACCESS_HTTP)
					sprintf(accessbuf, "%d", 1);
				else
					sprintf(accessbuf, "%d", 0);
				vsos_xmlNewChild(cur, NULL, BAD_CAST"http", BAD_CAST accessbuf);

				memset(accessbuf, 0, 10);
				if(abuf.access_type & IFF_ACCESS_HTTPS)
					sprintf(accessbuf, "%d", 1);
				else
					sprintf(accessbuf, "%d", 0);
				vsos_xmlNewChild(cur, NULL, BAD_CAST"https", BAD_CAST accessbuf);

				memset(accessbuf, 0, 10);
				if(abuf.access_type & IFF_ACCESS_PING)
					sprintf(accessbuf, "%d", 1);
				else
					sprintf(accessbuf, "%d", 0);
				vsos_xmlNewChild(cur, NULL, BAD_CAST"ping", BAD_CAST accessbuf);

				memset(accessbuf, 0, 10);
				if(abuf.access_type & IFF_ACCESS_TELNET)
					sprintf(accessbuf, "%d", 1);
				else
					sprintf(accessbuf, "%d", 0);
				vsos_xmlNewChild(cur, NULL, BAD_CAST"telnet", BAD_CAST accessbuf);

				memset(accessbuf, 0, 10);
				if(abuf.access_type & IFF_ACCESS_SSH)
					sprintf(accessbuf, "%d", 1);
				else
					sprintf(accessbuf, "%d", 0);
				vsos_xmlNewChild(cur, NULL, BAD_CAST"ssh", BAD_CAST accessbuf);

				memset(accessbuf, 0, 10);
				if(abuf.access_type & IFF_ACCESS_SSLVPN)
					sprintf(accessbuf, "%d", 1);
				else
					sprintf(accessbuf, "%d", 0);
				vsos_xmlNewChild(cur, NULL, BAD_CAST"sslvpn", BAD_CAST accessbuf);

				memset(accessbuf, 0, 10);
				if(abuf.access_type & IFF_ACCESS_BGP)
					sprintf(accessbuf, "%d", 1);
				else
					sprintf(accessbuf, "%d", 0);
				vsos_xmlNewChild(cur, NULL, BAD_CAST"bgp", BAD_CAST accessbuf);

				memset(accessbuf, 0, 10);
				if(abuf.access_type & IFF_ACCESS_OSPF)
					sprintf(accessbuf, "%d", 1);
				else
					sprintf(accessbuf, "%d", 0);
				vsos_xmlNewChild(cur, NULL, BAD_CAST"ospf", BAD_CAST accessbuf);

				memset(accessbuf, 0, 10);
				if(abuf.access_type & IFF_ACCESS_RIP)
					sprintf(accessbuf, "%d", 1);
				else
					sprintf(accessbuf, "%d", 0);
				vsos_xmlNewChild(cur, NULL, BAD_CAST"rip", BAD_CAST accessbuf);

				memset(accessbuf, 0, 10);
				if(abuf.access_type & IFF_ACCESS_DNS)
					sprintf(accessbuf, "%d", 1);
				else
					sprintf(accessbuf, "%d", 0);
				vsos_xmlNewChild(cur, NULL, BAD_CAST"dns", BAD_CAST accessbuf);

				memset(accessbuf, 0, 10);
				if(abuf.access_type & IFF_ACCESS_TCTRL)
					sprintf(accessbuf, "%d", 1);
				else
					sprintf(accessbuf, "%d", 0);
				vsos_xmlNewChild(cur, NULL, BAD_CAST"tctrl", BAD_CAST accessbuf);

				memset(accessbuf, 0, 10);
				if(abuf.access_type & IFF_ACCESS_WEBAUTH)
					sprintf(accessbuf, "%d", 1);
				else
					sprintf(accessbuf, "%d", 0);
				vsos_xmlNewChild(cur, NULL, BAD_CAST"webauth", BAD_CAST accessbuf);

				memset(accessbuf, 0, 10);
				if(abuf.access_type & IFF_ACCESS_LINKAGE)
					sprintf(accessbuf, "%d", 1);
				else
					sprintf(accessbuf, "%d", 0);
				vsos_xmlNewChild(cur, NULL, BAD_CAST"linkage", BAD_CAST accessbuf);
			}

			/*mtu*/
			if(ife.mtu > 0) {
				memset(buf, 0, 10);
				snprintf(buf, 9, "%d", ife.mtu);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"mtu", BAD_CAST buf);
			}

			/* external & internal */
			struct ifdev_info dev_info;
			memset(&dev_info, 0, sizeof(dev_info));
			strncpy(dev_info.ifname, ife.name, IFNAMSIZ);
			dataplane_request(UIPC_DEV_GET_MODE, (char *)&dev_info,
							  sizeof(dev_info), sizeof(dev_info), 0);
			if (dev_info.dev_type == IFDEV_ETH_TYPE) {
				memset(buf, 0, sizeof(buf));
				if (dev_info.adminflags & IFF_EXTERNAL)
					snprintf(buf, sizeof(buf), "%d", 1);
				else
					snprintf(buf, sizeof(buf), "%d", 0);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"external", BAD_CAST buf);
			}

			/***************TB new******************/


#if 0
			/* get local access*/
			memset(&abuf, 0, sizeof(struct allowaccess_st));
			abuf.cmd = 3;
			strcpy(abuf.ifname, ife.name);
			kernel_request(CTRL_ALLOWACCESS, (char *)&abuf, sizeof(abuf), sizeof(abuf), 0);
			memset(buf, 0, 10);
			if((abuf.access_type & IFF_ACCESS_HTTP) == IFF_ACCESS_HTTP)
				snprintf(buf, 9, "%d", 1);
			else
				snprintf(buf, 9, "%d", 0);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"http", BAD_CAST buf);
			memset(buf, 0, 10);
			if((abuf.access_type & IFF_ACCESS_HTTPS) == IFF_ACCESS_HTTPS)
				snprintf(buf, 9, "%d", 1);
			else
				snprintf(buf, 9, "%d", 0);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"https", BAD_CAST buf);
			memset(buf, 0, 10);
			if((abuf.access_type & IFF_ACCESS_PING) == IFF_ACCESS_PING)
				snprintf(buf, 9, "%d", 1);
			else
				snprintf(buf, 9, "%d", 0);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"ping", BAD_CAST buf);
			memset(buf, 0, 10);
			if((abuf.access_type & IFF_ACCESS_TELNET) == IFF_ACCESS_TELNET)
				snprintf(buf, 9, "%d", 1);
			else
				snprintf(buf, 9, "%d", 0);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"telnet", BAD_CAST buf);
			memset(buf, 0, 10);
			if((abuf.access_type & IFF_ACCESS_SSH) == IFF_ACCESS_SSH)
				snprintf(buf, 9, "%d", 1);
			else
				snprintf(buf, 9, "%d", 0);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"ssh", BAD_CAST buf);
			memset(buf, 0, 10);
			if((abuf.access_type & IFF_ACCESS_L2TP) == IFF_ACCESS_L2TP)
				snprintf(buf, 9, "%d", 1);
			else
				snprintf(buf, 9, "%d", 0);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"l2tp", BAD_CAST buf);
			memset(buf, 0, 10);
			if((abuf.access_type & IFF_ACCESS_SSLVPN) == IFF_ACCESS_SSLVPN)
				snprintf(buf, 9, "%d", 1);
			else
				snprintf(buf, 9, "%d", 0);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"sslvpn", BAD_CAST buf);
			memset(buf, 0, 10);
			if((abuf.access_type & IFF_ACCESS_WEBAUTH) == IFF_ACCESS_WEBAUTH)
				snprintf(buf, 9, "%d", 1);
			else
				snprintf(buf, 9, "%d", 0);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"webauth", BAD_CAST buf);
			memset(buf, 0, 10);
			if((abuf.access_type & IFF_ACCESS_CENMONI) == IFF_ACCESS_CENMONI)
				snprintf(buf, 9, "%d", 1);
			else
				snprintf(buf, 9, "%d", 0);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"cen_monitor", BAD_CAST buf);
			if(if_used_check(name))
				vsos_xmlNewChild( cur, NULL, BAD_CAST"count", BAD_CAST "1");
			else
				vsos_xmlNewChild( cur, NULL, BAD_CAST"count", BAD_CAST "0");

			/* get link-bind */
			if(if_belong_to_ha(ife.name) /* ??????HA ?ڣ???֧?ְ??? */
					|| (get_mgmt_port(ife.name))) { /* ?????ǹ????ڣ???֧?ְ??? */
				memset(buf, 0, 10);
				snprintf(buf, 9, "%d", 2);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"bind_if_enable", BAD_CAST buf);
			} else {
				memset(linkbind_name, 0, XML_MAX_NAME_LEN);
				if(if_adminflags_get(ife.name) & IFF_LINK_BIND) {
					kernel_request_ext(CTRL_GET_BIND_IF, NULL, 0, 0, NLM_F_DUMP, ife.name, bind_L2_show);
					if(strlen(linkbind_name) != 0) {
						char alias[IFNAMSIZ+1];
						//vsos_debug_out("XML-DEBUG: L2-BIND: bind-interface %s %s\r\n",
						//	ife.name, linkbind_name);
						memset(buf, 0, 10);
						snprintf(buf, 9, "%d", linkbind_if);
						vsos_xmlNewChild( cur, NULL, BAD_CAST"bind_if_enable", BAD_CAST buf);

						memset(alias,0,IFNAMSIZ+1);
						if_get_alias_by_name(linkbind_name, alias);
						if(strlen(alias)!=0)
							vsos_xmlNewChild( cur, NULL, BAD_CAST"if_name", BAD_CAST alias);
						else
							vsos_xmlNewChild( cur, NULL, BAD_CAST"if_name", BAD_CAST linkbind_name);

						memset(buf, 0, 10);
						snprintf(buf, 9, "%d", 0);
						vsos_xmlNewChild( cur, NULL, BAD_CAST"bind_type", BAD_CAST buf);
					} else {
						//vsos_debug_out("XML-DEBUG: L3-BIND\r\n");
						memset(buf, 0, 10);
						snprintf(buf, 9, "%d", 1);
						vsos_xmlNewChild( cur, NULL, BAD_CAST"bind_if_enable", BAD_CAST buf);

						bind_L3_show(ife.name);
						if(strlen(linkbind_name) != 0) {
							vsos_xmlNewChild( cur, NULL, BAD_CAST"monitor_addr", BAD_CAST linkbind_name);

							memset(buf, 0, 10);
							snprintf(buf, 9, "%d", 1);
							vsos_xmlNewChild( cur, NULL, BAD_CAST"bind_type", BAD_CAST buf);
						}
					}
				} else {
					//vsos_debug_out("%s is not link-bind\r\n", ife.name);
					memset(buf, 0, 10);
					snprintf(buf, 9, "%d", 0);
					vsos_xmlNewChild( cur, NULL, BAD_CAST"bind_if_enable", BAD_CAST buf);
				}
			}

			/*mtu*/
			if(ife.mtu > 0) {
				memset(buf, 0, 10);
				snprintf(buf, 9, "%d", ife.mtu);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"mtu", BAD_CAST buf);
			}

			/*interface bandwidth*/
			if_bandwidth = if_bandwidth_get(ife.name);
			if(if_bandwidth > 0 ) {
				memset(buf,0,10);
				snprintf(buf,9, "%d",if_bandwidth);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"if_bandwidth", BAD_CAST buf);
			}

			/*ha ip address*/
			{
				struct {
					struct oam_data_st oh;
					struct ha_link_st data;
				}  buf;

				buf.oh.cmd_code = HA_IFCFG_GET;
				buf.oh.mod_id = VTYSH_INDEX_HA;
				strcpy(buf.data.ifname , name);
				write(client_xml[VTYSH_INDEX_HA].fd, &buf, sizeof buf);


				while (1) {
					u32 nbytes;

					nbytes = read (client_xml[VTYSH_INDEX_HA].fd, &buf, VTY_READ_BUFSIZ);

					if (nbytes <= 0 && errno != EINTR) {
						break;
					}

					if (nbytes > 0) {
						if(buf.oh.cmd_code != 0) {
							break;
						} else {
							if(strlen(buf.data.addr_str))
								vsos_xmlNewChild( cur, NULL, BAD_CAST"haipaddr", BAD_CAST buf.data.addr_str);
						}
						break;
					}
				}
			}
#if 0 //#ifdef CONFIG_VSYS
			/* vsys_name */
			if( ui_type == VSYS_UI_SYSTEM_CONFIG ) {
				char vsys_name[32];
				int vsys_swi = 0;
				vsys_switch_get(&vsys_swi);
				if( vsys_swi ) {
					ifvsysid = (unsigned int)if_vsys_get(ife.name);
					if( vsys_get_name_by_vsysid(vsys_name,ifvsysid) == 0 )
						vsos_xmlNewChild( cur, NULL, BAD_CAST"vsys_name", BAD_CAST vsys_name);
				}
			}
#endif
			/*description*/
			char description[VSOS_MAX_DESC_LEN];
			if_desc_get(name, description);
			if(strlen(description))
				vsos_xmlNewChild( cur, NULL, BAD_CAST"desc", BAD_CAST description);

			/*sec ip*/
			{
				struct {
					struct oam_data_st oh;
					struct zebra_ip_addr_st data;
				}  buf;
				int firstip = 1;
				memset(&buf, 0, sizeof buf);
				buf.oh.cmd_code = ZEBRA_GET_ADDRESS;
				buf.oh.mod_id = 0;
				strcpy(buf.data.ifname, name);
				buf.data.addr_type = 1;
				write(client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof buf);


				while (1) {
					int nbytes;

					nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof(buf));

					if (nbytes <= 0 && errno != EINTR) {
						break;
					}
					if(buf.oh.cmd_code == 2) {
						struct prefix prefix;
						char tmpbuf[50];
						if(firstip == 1) {
							cur_2 = vsos_xmlNewChild( cur, NULL, BAD_CAST"sec_ip", BAD_CAST NULL);
							firstip = 0;
						}
						memcpy(&prefix, buf.data.addr_str, sizeof(struct prefix));
						prefix2str(&prefix, tmpbuf,50);
						cur_3 = vsos_xmlNewChild( cur_2, NULL, BAD_CAST"group", BAD_CAST NULL );
						vsos_xmlNewChild( cur_3, NULL, BAD_CAST"second_ip", BAD_CAST tmpbuf);
						continue;
					}
					if (nbytes > 0) {
						break;
					}
				}
			}
#endif/*OLD_PLAT*/
#endif /*Arrangement code by nilijun*/
		}
	}

	fclose(fp);/*?*/
	
	if (sattr->count)
		xml_add_page(child, sattr, total);

	send_buf = xml_end(global_doc, ZEBRA_SAVE_TMP);
	return send_buf;
}

char *xml_interface_node_showindex(void *ptr)
{
	xmlNode *cur;
	xmlNode *child;
	char *ifname;
	char * send_buf;
	FILE *fp;
	char buf[1024 + 1];
	int ret;
	struct interface_info ife;
	struct interface_node *if_node = NULL;
	xmlDoc *global_doc ;
	char tmp_name[64];

#if 0 //#ifdef CONFIG_VSYS
	unsigned int vsysid;
	enum vsys_ui_type ui_type;

	if( xml_vsys_gui_get_current_uitype(&ui_type) < 0 ) {
		vsos_debug_out("xml_interface_node_showindex: get vsys_ui_type failed\r\n");
		return NULL;
	}

	if( ui_type == VSYS_UI_SYSTEM_CONFIG )
		vsysid = ~0;
	else {
		if(xml_vsys_gui_get_current_vsysid(&vsysid) < 0 ) {
			vsos_debug_out("xml_interface_node_showindex: get vsysid failed\r\n");
			return NULL;
		}
	}
#endif

	if_node = (struct interface_node *)ptr;
	ifname = if_node->name;
	/* Open /proc/net/dev. */
	ret = 0;
	int ver =0;
	fp = fopen (_PATH_PROC_NET_DEV, "r");
	if (fp == NULL)
		return NULL;

	/* Drop header lines. */
	fgets (buf, 1024, fp);
	fgets (buf, 1024, fp);
	ver =procnetdev_version(buf);

	child = xml_start_more(&global_doc, "interface_sub");

	while (fgets (buf, 1024, fp) != NULL) {
		buf[1024] = '\0';
		char *s, name[IFNAMSIZ];
		s = get_name(name, buf);
		if(strlen(ifname)==0 ||!strcmp(name,ifname)) {
			int iftype;
			strncpy(ife.name,name,IFNAMSIZ);
//			get_dev_fields(ver, s, &ife);
			if(fetch_interface(&ife)<0)
				continue;
			if(!strncmp(ife.name, "loop", 4))
				continue;
			if(!strncmp(ife.name, "eth", 3))
				continue;
			if(!strncmp(ife.name, "vlan", 4))
				continue;
			if(!strncmp(ife.name, "lo", 2))
				continue;
			if(strchr(ife.name, '.'))
				continue;
			if(!strncmp(ife.name, "tvi", 3))
				continue;

			/* 10 is policy(phy & vpn) */
			if (10 != if_node->type) {
				if(!strncmp(ife.name, "gre", 3))
					continue;
				if(!strncmp(ife.name, "tunl", 4))
					continue;
				if(!strncmp(ife.name, "sit", 3))
					continue;
			}

#if 0
			if(ife.type != ARPHRD_ETHER)
				continue;
			if(!strncmp(ife.name, "bvi", 3))
				continue;
			/*
			#ifdef CONFIG_TRUNK
						if((!strncmp(ife.name, "tvi", 3)) && (!strchr(ife.name, '.')))
							continue;
			#endif
			*/
			if(!strcmp(ife.name, "npec"))
				continue;
			if(!strncmp(ife.name, "swi", 3))
				continue;
			if(!strncmp("nsa", name, 3))
				continue;
			if(!strncmp(ife.name, "swi",3))
				continue;
			if(!strncmp(ife.name, "loop", 4))
				continue;
			if (if_node->mgmt_flag != 1) {
				if (get_mgmt_port(ife.name) == 1)
					continue;
			}
#if 0 //#ifdef CONFIG_VSYS
			if((vsysid != ~0)
					&&  if_vsys_check(ife.name,vsysid) == 1 )
				continue;
			if( vsysid == ~0 ) {
				if( if_node->get_type == 12 ) {
					/*create new vsys, only display vsys == 0*/
					unsigned int tmp_vsysid = if_vsys_get(ife.name);
					if( tmp_vsysid )
						continue;
				} else if( if_node->get_type == 13 ) {
					/*edit vsys, only display vsys == 0 || vsys == cur_vsys*/
					unsigned int tmp_vsysid = if_vsys_get(ife.name);
					unsigned int tmp_editvsysid = 0;

					vsys_get_id_by_vsysname(if_node->vsys_name,&tmp_editvsysid);
					if( tmp_vsysid && tmp_editvsysid != tmp_vsysid )
						continue;
				}
			}
#endif
#endif/*OLD_PLAT*/

			if(!if_show_index_valid(client_xml+VTYSH_INDEX_APP, ife.name, if_node->get_type))
				continue;

#if 0
			if(ife.ifHwIntf == 1)
				iftype = 0;
			else
				iftype = 1;

#ifdef CONFIG_TRUNK
			if(if_node->get_type==4 && (!strncmp(ife.name, "tvi", 3)) && (!strchr(ife.name, '.')))
				iftype=0;
#endif
#endif/*OLD_PLAT*/

			cur = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );

#if 0
			if( iftype==0 )
				vsos_xmlNewChild( cur, NULL, BAD_CAST"type", BAD_CAST "0");
			else
				vsos_xmlNewChild( cur, NULL, BAD_CAST"type", BAD_CAST "1");
#endif

			vsos_xmlNewChild( cur, NULL, BAD_CAST"name", BAD_CAST ife.name);
			memset(tmp_name, 0, 64);
			if_get_alias_by_name(name, tmp_name);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"name", BAD_CAST ife.name);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"real_name", BAD_CAST ife.name);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"alias_name", BAD_CAST tmp_name/*ife.name*/);
		}
	}

	fclose(fp);/*?*/
	send_buf = xml_end(global_doc, ZEBRA_SAVE_TMP);
	return send_buf;
}

char *xml_ptp_if_showindex(void *ptr)
{
	xmlNode *cur;
	xmlNode *child;
	char * send_buf;
	FILE *fp;
	char buf[1024 + 1];
	int ret;
	struct interface_info ife;
	xmlDoc *global_doc ;
	char tmp_name[64];

	/* Open /proc/net/dev. */
	ret = 0;
	int ver =0;
	fp = fopen (_PATH_PROC_NET_DEV, "r");
	if (fp == NULL)
		return NULL;

	/* Drop header lines. */
	fgets (buf, 1024, fp);
	fgets (buf, 1024, fp);
	ver =procnetdev_version(buf);

	child = xml_start_more(&global_doc, "ptp_if");

	while (fgets (buf, 1024, fp) != NULL) {
		buf[1024] = '\0';
		char name[IFNAMSIZ];
		get_name(name, buf);

		strncpy(ife.name,name,IFNAMSIZ);
		if(fetch_interface(&ife)<0)
			continue;

		if(!strncmp(ife.name, "gre", 3) || 
			!strncmp(ife.name, "tunl", 4)) {
			cur = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );

			vsos_xmlNewChild( cur, NULL, BAD_CAST"name", BAD_CAST ife.name);
			memset(tmp_name, 0, 64);
			if_get_alias_by_name(name, tmp_name);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"name", BAD_CAST ife.name);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"real_name", BAD_CAST ife.name);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"alias_name", BAD_CAST tmp_name/*ife.name*/);
		}
	}

	fclose(fp);/*?*/
	send_buf = xml_end(global_doc, ZEBRA_SAVE_TMP);
	return send_buf;
}

char *xml_p2p_if_showindex(void *ptr)
{
	xmlNode *cur;
	xmlNode *child;
	char * send_buf;
	FILE *fp;
	char buf[1024 + 1];
	int ret;
	char ifname[IFNAMSIZ];
	char tmp_name[64];
	char vrf_name[XML_MAX_NAME_LEN];
	xmlDoc *global_doc ;
	struct p2p_if *req = (struct p2p_if *)ptr;
	int ifvsysid, vsysid;
	
	vsysid = GUI_CUR_VSYSID;
	/* Open /proc/net/dev. */
	ret = 0;
	int ver =0;
	fp = fopen (_PATH_PROC_NET_DEV, "r");
	if (fp == NULL)
		return NULL;

	/* Drop header lines. */
	fgets (buf, 1024, fp);
	fgets (buf, 1024, fp);
	ver =procnetdev_version(buf);

	child = xml_start_more(&global_doc, "p2p_if");

	while (fgets (buf, 1024, fp) != NULL) {
		buf[1024] = '\0';
		char name[IFNAMSIZ];
		get_name(name, buf);

		strncpy(ifname,name,IFNAMSIZ);
		if (if_is_p2p(ifname, client_xml[VTYSH_INDEX_ZEBRA].fd)) {
			
			ifvsysid = dp_netns_get_vrfid_by_ifname(ifname);
			if (ifvsysid != vsysid)
				continue;
			
			cur = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );

			memset(tmp_name, 0, 64);
			if_get_alias_by_name(name, tmp_name);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"name", BAD_CAST ifname);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"real_name", BAD_CAST ifname);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"alias_name", BAD_CAST tmp_name/*ife.name*/);
		}
	}

	fclose(fp);/*?*/
	send_buf = xml_end(global_doc, ZEBRA_SAVE_TMP);
	return send_buf;
}

char *xml_inf_select_showindex(void *ptr)
{
	xmlNode *cur;
	xmlNode *child;
	char *ifname;
	char * send_buf;
	FILE *fp;
	char buf[1024 + 1];
	struct interface_info ife;
	struct inf_select *if_node = NULL;
	xmlDoc *global_doc ;
	char tmp_buf[64];
    int ifvsysid, vsysid;
    vsysid = GUI_CUR_VSYSID;

	if_node = (struct inf_select *)ptr;
	ifname = if_node->name;

	/* Open /proc/net/dev. */
	int ver =0;
	fp = fopen (_PATH_PROC_NET_DEV, "r");
	if (fp == NULL)
		return NULL;

	/* Drop header lines. */
	fgets (buf, 1024, fp);
	fgets (buf, 1024, fp);
	ver =procnetdev_version(buf);

	child = xml_start_more(&global_doc, "inf_select");

	while (fgets (buf, 1024, fp) != NULL) {
		buf[1024] = '\0';
		char *s, name[IFNAMSIZ], vrf_name[XML_MAX_NAME_LEN];
		s = get_name(name, buf);
		if(strlen(ifname)==0 ||!strcmp(name,ifname)) {
			strncpy(ife.name,name,IFNAMSIZ);

			if(fetch_interface(&ife)<0)
				continue;

			if(strncmp(ife.name, "ge", 2) && strncmp(ife.name, "vlan", 4)
					&& strncmp(ife.name, "tvi", 3) && strncmp(ife.name, "gre", 3)
					&& strncmp(ife.name, "xge", 3) && strncmp(ife.name, "tunl", 4)) {
				continue;
			}

			if(!if_show_index_select(client_xml+VTYSH_INDEX_APP, ife.name, if_node->get_type))
				continue;

			ifvsysid = dp_netns_get_vrfid_by_ifname(ife.name);
            if(ifvsysid != vsysid && vsysid != 0)
				continue;

			cur = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );
			vsos_xmlNewChild( cur, NULL, BAD_CAST"get_type", BAD_CAST "0");
			vsos_xmlNewChild( cur, NULL, BAD_CAST"name", BAD_CAST ife.name);

			memset(tmp_buf, 0, 64);
			if_get_alias_by_name(name, tmp_buf);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"real_name", BAD_CAST ife.name);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"alias_name", BAD_CAST tmp_buf/*ife.name*/);

			
			/*vsys id*/
			memset(tmp_buf, 0, sizeof(tmp_buf));
			snprintf(tmp_buf, sizeof(tmp_buf), "%d", ifvsysid);
			vsos_xmlNewChild(cur, NULL, BAD_CAST "ifvsysid", BAD_CAST tmp_buf);
			
			if (ifvsysid == vsysid)
				vsos_xmlNewChild( cur, NULL, BAD_CAST"vsysid_same", BAD_CAST "1");
			else
				vsos_xmlNewChild( cur, NULL, BAD_CAST"vsysid_same", BAD_CAST "0");	
		}
	}

	fclose(fp);/*?*/
	send_buf = xml_end(global_doc, ZEBRA_SAVE_TMP);
	return send_buf;
}

struct interface_infomation	interface_infomation_data;

PARSE_XML_INTERFACE(interface_infomation, name, struct interface_infomation, XML_MAX_NAME_LEN)
PARSE_XML_STRING(interface_infomation, real_name, struct interface_infomation, IFNAMSIZ)
PARSE_XML_INT(interface_infomation, status, struct interface_infomation)
PARSE_XML_INT(interface_infomation, speed, struct interface_infomation)
PARSE_XML_IPMASK(interface_infomation, ip_address, struct interface_infomation)
PARSE_XML_INT(interface_infomation, duplex, struct interface_infomation)
PARSE_XML_INT(interface_infomation, rx_pkt, struct interface_infomation)
PARSE_XML_INT(interface_infomation, send_pkt, struct interface_infomation)
PARSE_XML_INT(interface_infomation, rx_byte, struct interface_infomation)
PARSE_XML_INT(interface_infomation, send_byte, struct interface_infomation)
PARSE_XML_STRING(interface_infomation, hw_addr, struct interface_infomation, XML_MAX_NAME_LEN)
PARSE_XML_INT(interface_infomation, bw_usage, struct interface_infomation)

struct element_node_parameter interface_infomation_element_parameter[] = {
	HASH_NODE_PARAM_ADD(interface_infomation, name, TYPE_XML_INTERFACE),
	HASH_NODE_PARAM_ADD(interface_infomation, real_name, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(interface_infomation, status, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_infomation, speed, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_infomation, ip_address, TYPE_XML_IPMASK),
	HASH_NODE_PARAM_ADD(interface_infomation, duplex, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_infomation, rx_pkt, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_infomation, send_pkt, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_infomation, rx_byte, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_infomation, send_byte, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_infomation, hw_addr, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(interface_infomation, bw_usage, TYPE_XML_INT),
	{"", NULL, NULL, NULL, TYPE_XML_INT}
};

static int get_if_ipv6_address(char *if_name, struct prefix *prefix)
{
	int find_flag = 0;

	struct {
		struct oam_data_st oh;
		struct zebra_ip_addr_st data;
	} buf;

	memset(&buf, 0, sizeof buf);
	buf.oh.cmd_code = ZEBRA_GET_ADDRESS_ALL_IPV6;
	strncpy(buf.data.ifname , if_name, XML_MAX_NAME_LEN);
	buf.oh.mod_id = VTYSH_INDEX_ZEBRA;

	write(client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof buf);

	while (1) {
		int nbytes;

		nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof(buf));

		if (nbytes <= 0 && errno != EINTR) {
			break;
		}
		if (nbytes > 0) {
			if(buf.oh.cmd_code == 2) {
				memset(prefix, 0, sizeof(struct prefix));
				memcpy(prefix, buf.data.addr_str, sizeof(struct prefix));

				find_flag = 1;
				continue;
			}
			break;
		}
	}
	return find_flag;
}

char *xml_interface_infomation_showall(void *ptr)
{
	xmlNode *cur;
	xmlNode *child;
	char *ifname = NULL;
	char * send_buf;
	FILE *fp;
	char buf[1024 + 1];
	int ret;
	struct interface_info ife;
	struct interface_infomation *if_node = NULL;
	xmlDoc *global_doc ;


	if_node = (struct interface_infomation *)ptr;
	if(if_node)
		ifname = if_node->name;
	/* Open /proc/net/dev. */
	ret = 0;
	int ver =0;
	fp = fopen (_PATH_PROC_NET_DEV, "r");
	if (fp == NULL)
		return NULL;

	/* Drop header lines. */
	fgets (buf, 1024, fp);
	fgets (buf, 1024, fp);
	ver =procnetdev_version(buf);

	child = xml_start_more(&global_doc, "interface_infomation");

	while (fgets (buf, 1024, fp) != NULL) {
		buf[1024] = '\0';
		char *s, name[IFNAMSIZ];
		s = get_name(name, buf);
		if(strlen(ifname)==0 ||!strcmp(name,ifname)) {
			char buf[50];
			struct prefix addr;
			memset(&ife,0,sizeof (struct interface_info));
			strncpy(ife.name,name,IFNAMSIZ);

			if(if_type_get(name) != IF_ETHERNET)
				continue;
			if(!strcmp(ife.name, "npec"))
				continue;
			if(!strncmp(ife.name, "swi", 3))
				continue;
			if(!strncmp("nsa", name, 3))
				continue;
			if(!strncmp(ife.name, "swi",3))
				continue;
			if(!strncmp(ife.name, "loop", 4))
				continue;
			if(!strncmp(ife.name, "lo", 2))
				continue;
			if(strchr(ife.name, '.'))
				continue;
			if(!strncmp(ife.name, "tvi", 3))
				continue;

			//get_dev_fields(ver, s, &ife);
			if(fetch_interface(&ife)<0)
				continue;

			cur = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );
			vsos_xmlNewChild( cur, NULL, BAD_CAST"name", BAD_CAST ife.name);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"real_name", BAD_CAST ife.name);
			if(ife.flags& IFF_RUNNING)
				vsos_xmlNewChild( cur, NULL, BAD_CAST"status", BAD_CAST "1");
			else
				vsos_xmlNewChild( cur, NULL, BAD_CAST"status", BAD_CAST "0");

			if(ife.has_ip && ((struct sockaddr_in *)(&ife.addr))->sin_addr.s_addr) {
				addr.family = AF_INET;
				addr.u.prefix4.s_addr = ((struct sockaddr_in *)(&ife.addr))->sin_addr.s_addr;
				addr.prefixlen = ip_masklen(((struct sockaddr_in *)(&ife.netmask))->sin_addr);
				prefix2str((struct prefix *)&addr, buf, 50);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"ip_address", BAD_CAST buf);
			}

			{
				char buf_temp[50];
				memset(buf_temp, 0, 50);
				get_str_from_macaddr((unsigned char *)ife.hwaddr,buf_temp);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"hw_addr", BAD_CAST buf_temp);
			}

			if(get_if_ipv6_address(ife.name, &addr) && (addr.prefixlen)) {
				prefix2str((struct prefix *)&addr, buf, 50);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"ip6_address", BAD_CAST buf);
			}

			if(!(ife.flags& IFF_RUNNING))
				continue;

			if(ife.type == ARPHRD_ETHER) {
				if(ife.media_type != PORT_FIBRE) {
					if(ife.autoneg==AUTONEG_ENABLE) {
						vsos_xmlNewChild( cur, NULL, BAD_CAST"negotiate", BAD_CAST "0");
					} else {
						vsos_xmlNewChild( cur, NULL, BAD_CAST"negotiate", BAD_CAST "1");
					}
				} else {
					vsos_xmlNewChild( cur, NULL, BAD_CAST"negotiate", BAD_CAST "0");
				}

				if(((ife.flags&IFF_RUNNING) && (ife.autoneg == AUTONEG_ENABLE)) || (ife.autoneg == AUTONEG_DISABLE)) {
					if(ife.speed == SPEED10000)
						vsos_xmlNewChild( cur, NULL, BAD_CAST"speed", BAD_CAST "10000");
					else if(ife.speed == SPEED1000)
						vsos_xmlNewChild( cur, NULL, BAD_CAST"speed", BAD_CAST "1000");
					else if(ife.speed == SPEED100)
						vsos_xmlNewChild( cur, NULL, BAD_CAST"speed", BAD_CAST "100");
					else if(ife.speed == SPEED10)
						vsos_xmlNewChild( cur, NULL, BAD_CAST"speed", BAD_CAST "10");

					if(ife.duplex) {
						vsos_xmlNewChild( cur, NULL, BAD_CAST"half", BAD_CAST "1");
					} else {
						vsos_xmlNewChild( cur, NULL, BAD_CAST"half", BAD_CAST "0");
					}
				} else {
					vsos_xmlNewChild( cur, NULL, BAD_CAST"speed", BAD_CAST "-1");
					vsos_xmlNewChild( cur, NULL, BAD_CAST"half", BAD_CAST "-1");
				}
			}
#if 1
			if(ife.statistics_valid == 0) {
				memset(buf, 0, 50);
				snprintf(buf, 49, "%ld", ife.rx_packets_traffic);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"rx_pkt", BAD_CAST buf);

				memset(buf, 0, 50);
				snprintf(buf, 49, "%Lu", ife.stats.rx_packets);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"rx_pkt_total", BAD_CAST buf);

				memset(buf, 0, 50);
				snprintf(buf, 49, "%ld", ife.tx_packets_traffic);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"send_pkt", BAD_CAST buf);

				memset(buf, 0, 50);
				snprintf(buf, 49, "%Lu", ife.stats.tx_packets);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"send_pkt_total", BAD_CAST buf);

				memset(buf, 0, 50);
				snprintf(buf, 49, "%ld.%03ld", (ife.rx_bytes_traffic<<3)/1000, (ife.rx_bytes_traffic<<3)%1000);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"rx_byte", BAD_CAST buf);

				memset(buf, 0, 50);
				snprintf(buf, 49, "%Lu", ife.stats.rx_bytes);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"rx_byte_total", BAD_CAST buf);

				memset(buf, 0, 50);
				snprintf(buf, 49, "%ld.%03ld", (ife.tx_bytes_traffic<<3)/1000, (ife.tx_bytes_traffic<<3)%1000);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"send_byte", BAD_CAST buf);

				memset(buf, 0, 50);
				snprintf(buf, 49, "%Lu", ife.stats.tx_bytes);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"send_byte_total", BAD_CAST buf);

				int bw_usage = 0;
				if(ife.duplex) {
					if(ife.speed == SPEED1000)
						bw_usage = (ife.rx_bytes_traffic+ife.tx_bytes_traffic)*8/(2*1000000000);
					else if(ife.speed == SPEED100)
						bw_usage = (ife.rx_bytes_traffic+ife.tx_bytes_traffic)*8/(2*100000000);
					else if(ife.speed == SPEED10)
						bw_usage = (ife.rx_bytes_traffic+ife.tx_bytes_traffic)*8/(2*10000000);
				} else {
					if(ife.speed == SPEED1000)
						bw_usage = (ife.rx_bytes_traffic+ife.tx_bytes_traffic)*8/(1000000000);
					else if(ife.speed == SPEED100)
						bw_usage = (ife.rx_bytes_traffic+ife.tx_bytes_traffic)*8/(100000000);
					else if(ife.speed == SPEED10)
						bw_usage = (ife.rx_bytes_traffic+ife.tx_bytes_traffic)*8/(10000000);
				}
				memset(buf, 0, 50);
				snprintf(buf, 49, "%d", bw_usage);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"bw_usage", BAD_CAST buf);
			}
#endif
		}
	}

	fclose(fp);/*?*/
	send_buf = xml_end(global_doc, ZEBRA_SAVE_TMP);
	return send_buf;
}

struct module_node_parameter interface_infomation_module_parameter = {
	"interface_infomation",
	NULL,
	NULL,
	NULL,
	xml_interface_infomation_showall,
	NULL,
	xml_interface_infomation_showall,
	NULL,
	0,
	sizeof(struct interface_infomation),
};

struct loop_interface	loop_interface_data;

PARSE_XML_INTERFACE(loop_interface, name, struct loop_interface, XML_MAX_NAME_LEN)
PARSE_XML_STRING(loop_interface, desc, struct loop_interface, 4*XML_MAX_NAME_LEN)
PARSE_XML_IPMASK(loop_interface, ipaddr, struct loop_interface)
PARSE_XML_INT(loop_interface, count, struct loop_interface)

struct element_node_parameter loop_interface_element_parameter[] = {
	HASH_NODE_PARAM_ADD(loop_interface, name, TYPE_XML_INTERFACE),
	HASH_NODE_PARAM_ADD(loop_interface, desc, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(loop_interface, ipaddr, TYPE_XML_IPMASK),
	HASH_NODE_PARAM_ADD(loop_interface, count, TYPE_XML_INT),
	{"", NULL, NULL, NULL, TYPE_XML_INT}
};

unsigned long xml_loop_interface_add(void *ptr)
{
	assert (NULL != ptr);
	struct loop_interface *if_node = ptr;
	int ret;
	char realname[100];
	//add by hongdong
	if(if_check_aliasname_by_rules(if_node->name))
		return 434;
	ret = if_check_aliasname(if_node->name);
	if(ret == 1)
		return 84;
	else if(ret == 2)
		return 85;
	else if(ret == 3)
		return 130;
	ret = guish_add_loopback();
	if(ret == 0)
		return 4;
	sprintf(realname,"lo%d",ret);
	if_rename(realname, if_node->name);
	strncpy(if_node->name, realname, XML_MAX_NAME_LEN);
	/*handle ip address*/
	if_address_config_clear(if_node->name);
	if(strlen(if_node->ipaddr)) { /*static ip config*/
		int ret = 0;
		struct {
			struct oam_data_st oh;
			struct zebra_ip_addr_st data;
		}  buf;
		memset(&buf, 0, sizeof buf);
		buf.oh.cmd_code = ZEBRA_ADD_ADDRESS;
		buf.oh.mod_id = 0;
		strncpy(buf.data.ifname , if_node->name, XML_MAX_NAME_LEN);
		strncpy(buf.data.addr_str , if_node->ipaddr, XML_IP_LEN );
		write(client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof buf);

		while (1) {
			int nbytes;

			nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof(buf)-1);

			if (nbytes <= 0 && errno != EINTR) {
				return 0;
			}

			if (nbytes > 0) {
				ret = buf.oh.cmd_code;
				if (ret != 0) {
					xml_loop_interface_del(ptr);
					return  buf.oh.cmd_code;
				}
				break;
			}
		}
	}

	/*desc*/
	if(strlen(if_node->desc))
		if_desc_set(if_node->name,if_node->desc);
	else
		if_desc_unset(if_node->name);

	return 0;
}

unsigned long xml_loop_interface_mod(void *ptr)
{
#if 0
	assert (NULL != ptr);
	struct loop_interface *if_node = ptr;
	struct prefix_ipv4 p;
	str2prefix_ipv4(if_node->ipaddr, &p);
//	int ret = ip_address_overlap_check(p.prefix.s_addr, p.prefixlen);
	int ret = ip_address_check(if_node->ipaddr, if_node->name);
	if(ret)
		return ret;
	/*handle ip address*/
	if_address_config_clear(if_node->name);
	if(strlen(if_node->ipaddr)) { /*static ip config*/
		int ret = 0;
		struct {
			struct oam_data_st oh;
			struct zebra_ip_addr_st data;
		}  buf;
		memset(&buf, 0, sizeof buf);
		buf.oh.cmd_code = ZEBRA_ADD_ADDRESS;
		buf.oh.mod_id = 0;
		strncpy(buf.data.ifname , if_node->name, XML_MAX_NAME_LEN);
		strncpy(buf.data.addr_str , if_node->ipaddr, XML_IP_LEN );
		write(client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof buf);

		while (1) {
			int nbytes;

			nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof(buf)-1);

			if (nbytes <= 0 && errno != EINTR) {
				return 0;
			}

			if (nbytes > 0) {
				ret = buf.oh.cmd_code;
				if (ret != 0)
					return  buf.oh.cmd_code;
				break;
			}
		}
	}

	/*desc*/
	if(strlen(if_node->desc))
		if_desc_set(if_node->name,if_node->desc);
	else
		if_desc_unset(if_node->name);

#endif
	return 0;
}

unsigned long xml_loop_interface_del(void *ptr)
{
	assert (NULL != ptr);
	struct loop_interface *if_node = ptr;
	if_shutdown(if_node->name);
	if_address_config_clear(if_node->name);
	del_loop_interface( if_node->name);

	return 0;
}


char *xml_loop_interface_showall(void *ptr)
{
	xmlNode *child;
	xmlNode *cur;
	char * send_buf;
	FILE *fp;
	char buf[1024 + 1];
	xmlDoc *global_doc ;
	struct interface_info ife;
	int ret;
	struct prefix addr;

	struct loop_interface *if_node = NULL;

	if_node = (struct loop_interface *)ptr;

	/* Open /proc/net/dev. */
	ret = 0;
	int ver =0;
	fp = fopen (_PATH_PROC_NET_DEV, "r");
	if (fp == NULL)
		return NULL;

	/* Drop header lines. */
	fgets (buf, 1024, fp);
	fgets (buf, 1024, fp);
	ver =procnetdev_version(buf);

	child = xml_start_more(&global_doc, "loop_interface");

	while (fgets (buf, 1024, fp) != NULL) {
		buf[1024] = '\0';
		char *s, name[IFNAMSIZ];
		s = get_name(name, buf);
		if(strncmp(name, "lo", 2))
			continue;
		if(strlen(name) == 2)
			continue;
		if(strncmp(name, "loop", 4) == 0)
			continue;

		if(strlen(if_node->name)==0 ||!strcmp(name,if_node->name)) {
			strncpy(ife.name,name,IFNAMSIZ);
			get_dev_fields(ver, s, &ife);
			if(fetch_interface(&ife)<0)
				continue;
			cur = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );
			vsos_xmlNewChild( cur, NULL, BAD_CAST"name", BAD_CAST ife.name);
			/*description*/
			char description[VSOS_MAX_DESC_LEN];
			if_desc_get(name, description);
			if(strlen(description))
				vsos_xmlNewChild( cur, NULL, BAD_CAST"desc", BAD_CAST description);
			if(ife.has_ip && ((struct sockaddr_in *)(&ife.addr))->sin_addr.s_addr) {
				addr.family = AF_INET;
				addr.u.prefix4.s_addr = ((struct sockaddr_in *)(&ife.addr))->sin_addr.s_addr;
				addr.prefixlen = ip_masklen(((struct sockaddr_in *)(&ife.netmask))->sin_addr);
				prefix2str((struct prefix *)&addr, buf, 50);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"ipaddr", BAD_CAST buf);
			}
		}
	}

	fclose(fp);/*?*/
	send_buf = xml_end(global_doc, ZEBRA_SAVE_TMP);
	return send_buf;
}

int loop_interface_logcontent(unsigned char log, void *ptr, char *Operate)
{
    struct loop_interface* data = (struct loop_interface*)ptr;
    switch(log)
    {
        case WEB_OPERATION_MOD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Mod loop_interface.");

        case WEB_OPERATION_ADD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Add loop_interface.");

        case WEB_OPERATION_DEL:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Del loop_interface.");

        default:
            return 0;
    }
}

struct module_node_parameter loopback_interface_module_parameter = {
	"loop_interface",
	xml_loop_interface_add,
	xml_loop_interface_del,
	xml_loop_interface_mod,
	xml_loop_interface_showall,
	NULL,
	xml_loop_interface_showall,
	NULL,
	1,
	sizeof(struct loop_interface),
	.func_logcontent = loop_interface_logcontent,
};

//interface topo
struct interface_layout interface_layout_data;

PARSE_XML_STRING(interface_layout, layout_type, struct interface_layout, XML_MAX_NAME_LEN)
PARSE_XML_INT(interface_layout, layout_height, struct interface_layout)
PARSE_XML_INT(interface_layout, layout_width, struct interface_layout)
PARSE_XML_INT(interface_layout, row_total, struct interface_layout)
PARSE_XML_INT(interface_layout, col_total, struct interface_layout)
PARSE_XML_STRUCT(interface_layout, if_grid_detail, struct interface_layout, struct grid_detail)
PARSE_XML_STRING(grid_detail, real_name, struct grid_detail, XML_MAX_NAME_LEN)
PARSE_XML_INT(grid_detail, media_type, struct grid_detail)
PARSE_XML_INT(grid_detail, row, struct grid_detail)
PARSE_XML_INT(grid_detail, col, struct grid_detail)

struct element_node_parameter interface_layout_element_parameter[] = {

	HASH_NODE_PARAM_ADD(interface_layout, layout_type, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(interface_layout, layout_height, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_layout, layout_width, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_layout, row_total, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_layout, col_total, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_layout, if_grid_detail, TYPE_XML_STRUCT),
	{"", NULL, NULL, NULL, TYPE_XML_INT}
};

struct element_node_parameter grid_detail_element_parameter[] = {
	HASH_NODE_PARAM_ADD(grid_detail, real_name, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(grid_detail, media_type, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(grid_detail, row, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(grid_detail, col, TYPE_XML_INT),
};

extern char *interface_layout_show(void *ptr);

struct module_node_parameter interface_layout_module_parameter = {
	"interface_layout",
	NULL,
	NULL,
	NULL,
	interface_layout_show,
	NULL,
	NULL,
	NULL,
	1,
	sizeof(struct interface_layout),
};

#define LAYOUT_GRID "grid"
#define LAYOUT_HEIGHT 80
#define LAYOUT_WIDTH
#define LAYOUT_A_COL 4
#define LAYOUT_T_COL 6
#define LAYOUT_C_COL 8
#define LAYOUT_E_COL 7
#define MEDIA_ELECTRIC 1
#define MEDIA_LIGHT 2
#define MEDIA_L_OR_E 3
#define MEDIA_XGE 4

extern   int vsos_get_slot_type(int slot_id);

/**add by zxj start**/
#define LAYOUT_HEIGHT_CALC(row_total) (LAYOUT_HEIGHT * (row_total))
#define LAYOUT_WIDTH_CALC(col_total) (20 * (col_total) + 150)

#define INT_DEC_DIGIT_WIDTH 32
char *my_itoa(int value)
{
	static char int_dec_str[INT_DEC_DIGIT_WIDTH + 1];

	bzero(int_dec_str, INT_DEC_DIGIT_WIDTH + 1);
	snprintf(int_dec_str, INT_DEC_DIGIT_WIDTH, "%d", value);

	return int_dec_str;
}

static char *if_layout_show_xml(const if_layout_t *if_layout)
{
	xmlDoc *global_doc;
	xmlNode *root, *parent;
	char buf[16];

	root = xml_start_more(&global_doc, "interface_layout");
	parent = vsos_xmlNewChild(root, NULL, BAD_CAST"group", BAD_CAST NULL);
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf)-1, "%d", if_layout->device_type);
	vsos_xmlNewChild(parent, NULL, BAD_CAST"device_type", BAD_CAST buf);
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf)-1, "%d", if_layout->slot[0]);
	vsos_xmlNewChild(parent, NULL, BAD_CAST"slot0", BAD_CAST buf);
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf)-1, "%d", if_layout->slot[1]);
	vsos_xmlNewChild(parent, NULL, BAD_CAST"slot1", BAD_CAST buf);
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf)-1, "%d", if_layout->slot[2]);
	vsos_xmlNewChild(parent, NULL, BAD_CAST"slot2", BAD_CAST buf);
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf)-1, "%d", if_layout->slot[3]);
	vsos_xmlNewChild(parent, NULL, BAD_CAST"slot3", BAD_CAST buf);

	if((if_layout->device_type == 4) //ep xinhan 800
			|| (if_layout->device_type == 8) // adc900
			|| (if_layout->device_type == 14)) { //7120
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf)-1, "%d", if_layout->slot[4]);
		vsos_xmlNewChild(parent, NULL, BAD_CAST"slot4", BAD_CAST buf);

	} else if(if_layout->device_type == 6 //ep lihua 800_L
			|| if_layout->device_type == 206 //linghua 7200
			|| if_layout->device_type == 26	 //linghua 7200-a2 
			|| if_layout->device_type == 28) {	//xinhan 7135
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf)-1, "%d", if_layout->slot[4]);
		vsos_xmlNewChild(parent, NULL, BAD_CAST"slot4", BAD_CAST buf);
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf)-1, "%d", if_layout->slot[5]);
		vsos_xmlNewChild(parent, NULL, BAD_CAST"slot5", BAD_CAST buf);
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf)-1, "%d", if_layout->slot[6]);
		vsos_xmlNewChild(parent, NULL, BAD_CAST"slot6", BAD_CAST buf);
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf)-1, "%d", if_layout->slot[7]);
		vsos_xmlNewChild(parent, NULL, BAD_CAST"slot7", BAD_CAST buf);
	}

	return xml_end(global_doc, INTERFACE_TOPO_TMP);
}

void list_physics_interface(int device_type, struct interface_layout *data )
{
	FILE *fp;
	char buf[1024 + 1];
	int ret, i = 0;
	struct interface_info ife;

	struct grid_detail *tail = data->if_grid_detail;
	struct grid_detail *tmp = NULL;
#if 0 //#ifdef CONFIG_VSYS
	unsigned int vsysid;
	enum vsys_ui_type ui_type;

	if( xml_vsys_gui_get_current_uitype(&ui_type) < 0 ) {
		vsos_debug_out("list_physics_interface: get vsys_ui_type failed\r\n");
		return;
	}
	if( ui_type == VSYS_UI_SYSTEM_CONFIG )
		vsysid = ~0;
	else {
		if(xml_vsys_gui_get_current_vsysid(&vsysid) < 0 ) {
			vsos_debug_out("list_physics_interface: get vsysid failed\r\n");
			return ;
		}
	}
#endif
	/* Open /proc/net/dev. */
	ret = 0;
	int ver =0;
	fp = fopen (_PATH_PROC_NET_DEV, "r");
	if (fp == NULL)
		return ;

	/* Drop header lines. */
	fgets (buf, 1024, fp);
	fgets (buf, 1024, fp);
	ver =procnetdev_version(buf);

	while (fgets (buf, 1024, fp) != NULL) {
		buf[1024] = '\0';
		char *s, name[IFNAMSIZ];
		s = get_name(name, buf);

		strncpy(ife.name,name,IFNAMSIZ);
		get_dev_fields(ver, s, &ife);
		if(fetch_interface(&ife)<0)
			continue;
		if (ife.type != ARPHRD_ETHER)
			continue;
		if (IF_ETHERNET != if_type_get(ife.name))
			continue;
#if 0 //#ifdef CONFIG_VSYS
		if((vsysid != ~0)
				&& (if_vsys_check(ife.name,vsysid) == 1) )
			continue;
#endif
		if ((0 == strncmp(ife.name, "eth", 3))||(0 == strncmp(ife.name, "ge", 2))||(0 == strncmp(ife.name, "xge", 3))) {
			tmp = XMALLOC(MTYPE_GUISH_ZEBRA_GRID, sizeof(struct grid_detail));
			memset(tmp, 0, sizeof(struct grid_detail));
			sprintf(tmp->real_name, ife.name);
			tmp->media_type = ife.media_type;
			tmp->row = 0;
			tmp->col = i;
			tmp->next = NULL;
			tail->next= tmp;
			tail = tail->next;
			if (get_mgmt_port(ife.name) == 1)
				i++;
			i++;
		}
		data->layout_width = 20*i+150;
		data->col_total =  i;
	}

	fclose(fp);/*?*/
	return ;
}

char *interface_layout_show(void *ptr)
{
	struct interface_layout data;
	memset(&data, 0, sizeof(struct interface_layout));
	struct grid_detail *tail = data.if_grid_detail;
	struct grid_detail *tmp = NULL;
	int i=0, j=0;
	int sub_device_type=0;

#if 0
	if(check_vm_mode()) {
		return NULL;
	}
#endif

	int device_type = vsos_get_device_type();
	{
		int board_port_num = 0;
		if_layout_t *if_layout;
		if_layout = if_layout_search(device_type);

		//vsos_debug_out("device_type is %d, if_layout is %p\n", device_type, if_layout);
		if (!if_layout) {
			return NULL;
		}

		if (if_layout->device_type == 24) {
			board_port_num = vsos_get_board_port();
			if (board_port_num == 4)
				if_layout->device_type = 15;
		}

		if_layout_slot_probe(if_layout);
		return if_layout_show_xml(if_layout);
	}

}

/*interface flow implemation*/
list interface_flow_record;
#define INTERFACE_FLOW_DB "/tmp/interface_flow_graph.db"

#define INTERFACE_FLOW_GRAPH_MIN  1
#define INTERFACE_FLOW_GRAPH_HOUR  2
#define INTERFACE_FLOW_GRAPH_DAY  3

#define INTERFACE_FLOW_GRAPH_30MIN  1
#define INTERFACE_FLOW_GRAPH_3HOUR  2
#define INTERFACE_FLOW_GRAPH_24HOUR  3
#define INTERFACE_FLOW_GRAPH_7DAY  4
#define INTERFACE_FLOW_GRAPH_30DAY  5


struct interface_flow_th_st interface_flow_timer;

extern int add_interface_flow_record_to_list(list record_list, struct interface_flow_point *record);

void del_interface_flow_record(void *data)
{
	XFREE(MTYPE_GUISH, data);
}

int get_interface_flow(int type)
{
	struct timeval ts;

	u32 per_band=0, per_tx=0, per_rx=0;
	FILE *fp;
	char buf[1024 + 1];
	int ret;
	struct interface_info ife;

	/* Open /proc/net/dev. */
	ret = 0;
	int ver =0;
	fp = fopen (_PATH_PROC_NET_DEV, "r");
	if (fp == NULL)
		return -1;

	/* Drop header lines. */
	fgets (buf, 1024, fp);
	fgets (buf, 1024, fp);
	ver =procnetdev_version(buf);

	while (fgets (buf, 1024, fp) != NULL) {
		buf[1024] = '\0';
		char *s, name[IFNAMSIZ];
		s = get_name(name, buf);
		{
			memset(&ife,0,sizeof (struct interface_info));
			strncpy(ife.name,name,IFNAMSIZ);

			if(if_type_get(name) != IF_ETHERNET)
				continue;
			if(!strcmp(ife.name, "npec"))
				continue;
			if(!strncmp(ife.name, "swi", 3))
				continue;
			if(!strncmp(ife.name, "loop", 4))
				continue;
			if(!strncmp("nsa", name, 3))
				continue;
			if(!strncmp(ife.name, "lo", 2))
				continue;
			if(!strncmp(ife.name, "eth", 3))
				continue;
			if(!strncmp(ife.name, "tvi", 3))
				continue;
			if(!strncmp(ife.name, "vlan", 4))
				continue;

			get_dev_fields(ver, s, &ife);
			if(fetch_interface(&ife)<0)
				continue;

			if((ife.type == ARPHRD_ETHER)&&(ife.ifHwIntf == 1)) {

				if(ife.max_speed == SPEED1000)
					per_band = 1000 * 1000 * 2;
				else if(ife.max_speed == SPEED100)
					per_band = 100 *1000 * 2;
				else
					per_band = 10 * 1000 * 2;
				//info->band += per_band;
			}
			if(ife.statistics_valid == 0) {
				per_rx = ife.rx_bytes_traffic;
				per_tx = ife.tx_bytes_traffic;

				vsos_get_current_utctm(&ts);
				struct interface_flow_point *tmp_node;
				tmp_node = XMALLOC(MTYPE_GUISH, sizeof(struct interface_flow_point));
				memset(tmp_node, 0, sizeof(struct interface_flow_point));

				tmp_node->graph_type = type;
				memcpy(tmp_node->ifname, ife.name, IFNAMSIZ);
				tmp_node->rx = (per_rx << 3)/1000;
				tmp_node->tx = (per_tx << 3)/1000;
				memcpy(&(tmp_node->ts), &ts, sizeof(struct timeval));
				vsos_get_current_localtime(tmp_node->time, sizeof(tmp_node->time));
				add_interface_flow_record_to_list(interface_flow_record, tmp_node);
				XFREE(MTYPE_GUISH, tmp_node);
			}
		}
	}

	fclose(fp);/*?*/

	return 0;
}

/*
int get_interface_flow_m(struct thread *th)
{
	struct interface_flow_th_st *timer_st;
	timer_st = THREAD_ARG(th);
	timer_st->m_th = NULL;

	get_interface_flow(INTERFACE_FLOW_GRAPH_MIN);
	timer_st->m_th = thread_add_timer(master, get_interface_flow_m, timer_st, 60);
	return 0;
}

int get_interface_flow_h(struct thread *th)
{
	struct interface_flow_th_st *timer_st;
	timer_st = THREAD_ARG(th);
	timer_st->h_th = NULL;

	get_interface_flow(INTERFACE_FLOW_GRAPH_HOUR);
	timer_st->h_th = thread_add_timer(master, get_interface_flow_h, timer_st, 3600);
	return 0;
}

int get_interface_flow_d(struct thread *th)
{
	struct interface_flow_th_st *timer_st;
	timer_st = THREAD_ARG(th);
	timer_st->d_th = NULL;

	get_interface_flow(INTERFACE_FLOW_GRAPH_DAY);

	timer_st->d_th = thread_add_timer(master, get_interface_flow_d, timer_st, 3600*3);
	return 0;
}
*/

int get_interface_flow_30m(struct thread *th)
{
	struct interface_flow_th_st *timer_st;
	timer_st = THREAD_ARG(th);
	timer_st->th_30s = NULL;

	get_interface_flow(INTERFACE_FLOW_GRAPH_30MIN);
	timer_st->th_30s = thread_add_timer(master, get_interface_flow_30m, timer_st, 30);
	return 0;
}
int get_interface_flow_3h(struct thread *th)
{
	struct interface_flow_th_st *timer_st;
	timer_st = THREAD_ARG(th);
	timer_st->th_3m = NULL;

	get_interface_flow(INTERFACE_FLOW_GRAPH_3HOUR);
	timer_st->th_3m = thread_add_timer(master, get_interface_flow_3h, timer_st, 3*60);
	return 0;
}
int get_interface_flow_24h(struct thread *th)
{
	struct interface_flow_th_st *timer_st;
	timer_st = THREAD_ARG(th);
	timer_st->th_1h = NULL;

	get_interface_flow(INTERFACE_FLOW_GRAPH_24HOUR);
	timer_st->th_1h = thread_add_timer(master, get_interface_flow_24h, timer_st, 3600);
	return 0;
}
int get_interface_flow_7d(struct thread *th)
{
	struct interface_flow_th_st *timer_st;
	timer_st = THREAD_ARG(th);
	timer_st->th_3h = NULL;

	get_interface_flow(INTERFACE_FLOW_GRAPH_7DAY);
	timer_st->th_3h = thread_add_timer(master, get_interface_flow_7d, timer_st, 3*3600);
	return 0;
}
int get_interface_flow_30d(struct thread *th)
{
	struct interface_flow_th_st *timer_st;
	timer_st = THREAD_ARG(th);
	timer_st->th_12h = NULL;

	get_interface_flow(INTERFACE_FLOW_GRAPH_30DAY);
	timer_st->th_12h = thread_add_timer(master, get_interface_flow_30d, timer_st, 12*3600);
	return 0;
}


int save_interface_flow_data(struct thread *th)
{
	struct interface_flow_th_st *timer_st;
	timer_st = THREAD_ARG(th);
	timer_st->save_th = NULL;

	int ret;
	int key_value = 0;
	DB *dbp;
	DBT key, data;
	struct listnode *nn;
	struct listnode *nn_tmp;
	struct interface_flow *ifnode;
	struct interface_flow_point *rnode;


	if ((ret = db_create(&dbp, NULL, 0)) != 0) {
		return 1;
	}
	if ((ret = dbp->open(dbp, NULL, INTERFACE_FLOW_DB, NULL, DB_BTREE, DB_TRUNCATE, 0664)) != 0) {
		return 2;
	}

	LIST_LOOP (interface_flow_record, ifnode, nn) {
		/*
				LIST_LOOP (ifnode->record_1m, rnode, nn_tmp)
				{
					memset(&key, 0, sizeof(DBT));
					memset(&data, 0, sizeof(DBT));
					key.data = &key_value;
					key.size = sizeof(int);
					data.data = rnode;
					data.size = sizeof(struct interface_flow_point);
					dbp->put(dbp, NULL, &key, &data, 0);
				}

				LIST_LOOP (ifnode->record_1h, rnode, nn_tmp)
				{
					memset(&key, 0, sizeof(DBT));
					memset(&data, 0, sizeof(DBT));
					key.data = &key_value;
					key.size = sizeof(int);
					data.data = rnode;
					data.size = sizeof(struct interface_flow_point);
					dbp->put(dbp, NULL, &key, &data, 0);
				}

				LIST_LOOP (ifnode->record_3h, rnode, nn_tmp)
				{
					memset(&key, 0, sizeof(DBT));
					memset(&data, 0, sizeof(DBT));
					key.data = &key_value;
					key.size = sizeof(int);
					data.data = rnode;
					data.size = sizeof(struct interface_flow_point);
					dbp->put(dbp, NULL, &key, &data, 0);
				}
		*/
		LIST_LOOP (ifnode->record_30s, rnode, nn_tmp) {
			memset(&key, 0, sizeof(DBT));
			memset(&data, 0, sizeof(DBT));
			key.data = &key_value;
			key.size = sizeof(int);
			data.data = rnode;
			data.size = sizeof(struct interface_flow_point);
			dbp->put(dbp, NULL, &key, &data, 0);
		}
		LIST_LOOP (ifnode->record_3m, rnode, nn_tmp) {
			memset(&key, 0, sizeof(DBT));
			memset(&data, 0, sizeof(DBT));
			key.data = &key_value;
			key.size = sizeof(int);
			data.data = rnode;
			data.size = sizeof(struct interface_flow_point);
			dbp->put(dbp, NULL, &key, &data, 0);
		}
		LIST_LOOP (ifnode->record_1h, rnode, nn_tmp) {
			memset(&key, 0, sizeof(DBT));
			memset(&data, 0, sizeof(DBT));
			key.data = &key_value;
			key.size = sizeof(int);
			data.data = rnode;
			data.size = sizeof(struct interface_flow_point);
			dbp->put(dbp, NULL, &key, &data, 0);
		}
		LIST_LOOP (ifnode->record_3h, rnode, nn_tmp) {
			memset(&key, 0, sizeof(DBT));
			memset(&data, 0, sizeof(DBT));
			key.data = &key_value;
			key.size = sizeof(int);
			data.data = rnode;
			data.size = sizeof(struct interface_flow_point);
			dbp->put(dbp, NULL, &key, &data, 0);
		}
		LIST_LOOP (ifnode->record_12h, rnode, nn_tmp) {
			memset(&key, 0, sizeof(DBT));
			memset(&data, 0, sizeof(DBT));
			key.data = &key_value;
			key.size = sizeof(int);
			data.data = rnode;
			data.size = sizeof(struct interface_flow_point);
			dbp->put(dbp, NULL, &key, &data, 0);
		}
	}

	ret = dbp->close(dbp, 0);
	if(ret != 0)
		return 3;

	timer_st->save_th = thread_add_timer(master, save_interface_flow_data, timer_st, 3600);
	return 0;
}

int init_interface_flow_timer(list record_list, struct interface_flow_th_st *timer)
{
	struct interface_flow *ifnode;
	struct interface_flow_point *rnode;
	struct timeval ts;
	int offset = 0;

	if(listnode_head(record_list) == NULL) {
		/*
				timer->m_th = thread_add_timer(master, get_interface_flow_m, timer, 1);
				timer->h_th = thread_add_timer(master, get_interface_flow_h, timer, 1);
				timer->d_th = thread_add_timer(master, get_interface_flow_d, timer, 1);
		*/
		timer->th_30s = thread_add_timer(master, get_interface_flow_30m, timer, 1);
		timer->th_3m = thread_add_timer(master, get_interface_flow_3h, timer, 1);
		timer->th_1h = thread_add_timer(master, get_interface_flow_24h, timer, 1);
		timer->th_3h = thread_add_timer(master, get_interface_flow_7d, timer, 1);
		timer->th_12h = thread_add_timer(master, get_interface_flow_30d, timer, 1);
	} else {
		vsos_get_current_utctm(&ts);
		ifnode = listnode_head(record_list);
		/*
				rnode = listnode_tail(ifnode->record_1m);
				if(rnode != NULL)
					offset = 60 - (ts.tv_sec - rnode->ts.tv_sec);
				else
					offset = 1;
				timer->m_th = thread_add_timer(master, get_interface_flow_m, timer, offset);

				rnode = listnode_tail(ifnode->record_1h);
				if(rnode != NULL)
					offset = 3600 - (ts.tv_sec - rnode->ts.tv_sec);
				else
					offset = 1;
				timer->h_th = thread_add_timer(master, get_interface_flow_h, timer, offset);

				rnode = listnode_tail(ifnode->record_3h);
				if(rnode != NULL)
					offset = 3600*3 - (ts.tv_sec - rnode->ts.tv_sec);
				else
					offset = 1;
				timer->d_th = thread_add_timer(master, get_interface_flow_d, timer, offset);
		*/
		rnode = listnode_tail(ifnode->record_30s);
		if(rnode != NULL)
			offset = 30 - (ts.tv_sec - rnode->ts.tv_sec);
		else
			offset = 1;
		timer->th_30s = thread_add_timer(master, get_interface_flow_30m, timer, offset);

		rnode = listnode_tail(ifnode->record_3m);
		if(rnode != NULL)
			offset = 60*3 - (ts.tv_sec - rnode->ts.tv_sec);
		else
			offset = 1;
		timer->th_3m = thread_add_timer(master, get_interface_flow_3h, timer, offset);

		rnode = listnode_tail(ifnode->record_1h);
		if(rnode != NULL)
			offset = 3600 - (ts.tv_sec - rnode->ts.tv_sec);
		else
			offset = 1;
		timer->th_1h = thread_add_timer(master, get_interface_flow_24h, timer, offset);

		rnode = listnode_tail(ifnode->record_3h);
		if(rnode != NULL)
			offset = 3600*3 - (ts.tv_sec - rnode->ts.tv_sec);
		else
			offset = 1;
		timer->th_3h = thread_add_timer(master, get_interface_flow_7d, timer, offset);

		rnode = listnode_tail(ifnode->record_12h);
		if(rnode != NULL)
			offset = 3600*12 - (ts.tv_sec - rnode->ts.tv_sec);
		else
			offset = 1;
		timer->th_12h = thread_add_timer(master, get_interface_flow_30d, timer, offset);
	}

#if 0
	timer->save_th = thread_add_timer(master, save_interface_flow_data, timer, 1);
#endif

	return 0;
}

int interface_flow_convert_time(char *time_str, int time_len, long offset)
{
	if (XML_MAX_TIME_LEN+1 >  time_len)
		return -1;

	time_t now;
	struct tm tm_time;

	time (&now);

	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);
	now += tz.tz_minuteswest*60 + offset;

	memcpy(&tm_time, gmtime(&now), sizeof(tm_time));

	memset(time_str, 0, XML_MAX_TIME_LEN+1);
	snprintf(time_str, XML_MAX_TIME_LEN, "%04d-%02d-%02d %02d:%02d:%02d",
			 tm_time.tm_year+1900,tm_time.tm_mon+1, tm_time.tm_mday,
			 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);

	return 0;
}

int add_interface_flow_zero_record(list record_list)
{
	struct listnode *nn;
	struct interface_flow *ifnode;
	struct interface_flow_point *rnode;
	struct timeval ts;
	struct timeval tmp_ts;
	struct interface_flow_point *tmp_node;
	vsos_get_current_utctm(&ts);

	LIST_LOOP (record_list, ifnode, nn) {
		rnode = listnode_tail(ifnode->record_30s);
		if((rnode == NULL) || (ts.tv_sec - rnode->ts.tv_sec >= 1800))
			tmp_ts.tv_sec = ts.tv_sec - 1800;
		else
			memcpy(&tmp_ts, &(rnode->ts), sizeof(struct timeval));

		while(ts.tv_sec - tmp_ts.tv_sec >= 0) {
			tmp_node = XMALLOC(MTYPE_GUISH, sizeof(struct interface_flow_point));
			memset(tmp_node, 0, sizeof(struct interface_flow_point));
			tmp_node->graph_type = INTERFACE_FLOW_GRAPH_30MIN;
			memcpy(tmp_node->ifname, ifnode->ifname, IFNAMSIZ);
			tmp_node->rx = 0;
			tmp_node->tx = 0;
			tmp_node->ts.tv_sec = tmp_ts.tv_sec;
			tmp_node->ts.tv_usec= tmp_ts.tv_usec;
			interface_flow_convert_time(tmp_node->time, sizeof(tmp_node->time), tmp_ts.tv_sec - ts.tv_sec);
			add_interface_flow_record_to_list(record_list, tmp_node);
			XFREE(MTYPE_GUISH, tmp_node);
			tmp_ts.tv_sec += 30;
		}

		rnode = listnode_tail(ifnode->record_3m);
		if((rnode == NULL) || (ts.tv_sec - rnode->ts.tv_sec >= 3*3600))
			tmp_ts.tv_sec = ts.tv_sec - 3*3600;
		else
			memcpy(&tmp_ts, &(rnode->ts), sizeof(struct timeval));

		while(ts.tv_sec - tmp_ts.tv_sec >= 0) {
			tmp_node = XMALLOC(MTYPE_GUISH, sizeof(struct interface_flow_point));
			memset(tmp_node, 0, sizeof(struct interface_flow_point));
			tmp_node->graph_type = INTERFACE_FLOW_GRAPH_3HOUR;
			memcpy(tmp_node->ifname, ifnode->ifname, IFNAMSIZ);
			tmp_node->rx = 0;
			tmp_node->tx = 0;
			tmp_node->ts.tv_sec = tmp_ts.tv_sec;
			tmp_node->ts.tv_usec= tmp_ts.tv_usec;
			interface_flow_convert_time(tmp_node->time, sizeof(tmp_node->time), tmp_ts.tv_sec - ts.tv_sec);
			add_interface_flow_record_to_list(record_list, tmp_node);
			XFREE(MTYPE_GUISH, tmp_node);
			tmp_ts.tv_sec += 3*60;
		}

		rnode = listnode_tail(ifnode->record_1h);
		if((rnode == NULL) || (ts.tv_sec - rnode->ts.tv_sec >= 24*3600))
			tmp_ts.tv_sec = ts.tv_sec - 24*3600;
		else
			memcpy(&tmp_ts, &(rnode->ts), sizeof(struct timeval));

		while(ts.tv_sec - tmp_ts.tv_sec >= 0) {
			tmp_node = XMALLOC(MTYPE_GUISH, sizeof(struct interface_flow_point));
			memset(tmp_node, 0, sizeof(struct interface_flow_point));
			tmp_node->graph_type = INTERFACE_FLOW_GRAPH_24HOUR;
			memcpy(tmp_node->ifname, ifnode->ifname, IFNAMSIZ);
			tmp_node->rx = 0;
			tmp_node->tx = 0;
			tmp_node->ts.tv_sec = tmp_ts.tv_sec;
			tmp_node->ts.tv_usec= tmp_ts.tv_usec;
			interface_flow_convert_time(tmp_node->time, sizeof(tmp_node->time), tmp_ts.tv_sec - ts.tv_sec);
			add_interface_flow_record_to_list(record_list, tmp_node);
			XFREE(MTYPE_GUISH, tmp_node);
			tmp_ts.tv_sec += 3600;
		}

		rnode = listnode_tail(ifnode->record_3h);
		if((rnode == NULL) || (ts.tv_sec - rnode->ts.tv_sec >= 7*24*3600))
			tmp_ts.tv_sec = ts.tv_sec - 7*24*3600;
		else
			memcpy(&tmp_ts, &(rnode->ts), sizeof(struct timeval));

		while(ts.tv_sec - tmp_ts.tv_sec >= 0) {
			tmp_node = XMALLOC(MTYPE_GUISH, sizeof(struct interface_flow_point));
			memset(tmp_node, 0, sizeof(struct interface_flow_point));
			tmp_node->graph_type = INTERFACE_FLOW_GRAPH_7DAY;
			memcpy(tmp_node->ifname, ifnode->ifname, IFNAMSIZ);
			tmp_node->rx = 0;
			tmp_node->tx = 0;
			tmp_node->ts.tv_sec = tmp_ts.tv_sec;
			tmp_node->ts.tv_usec= tmp_ts.tv_usec;
			interface_flow_convert_time(tmp_node->time, sizeof(tmp_node->time), tmp_ts.tv_sec - ts.tv_sec);
			add_interface_flow_record_to_list(record_list, tmp_node);
			XFREE(MTYPE_GUISH, tmp_node);
			tmp_ts.tv_sec += 3*3600;
		}

		rnode = listnode_tail(ifnode->record_12h);
		if((rnode == NULL) || (ts.tv_sec - rnode->ts.tv_sec >= 30*24*3600))
			tmp_ts.tv_sec = ts.tv_sec - 30*24*3600;
		else
			memcpy(&tmp_ts, &(rnode->ts), sizeof(struct timeval));

		while(ts.tv_sec - tmp_ts.tv_sec >= 0) {
			tmp_node = XMALLOC(MTYPE_GUISH, sizeof(struct interface_flow_point));
			memset(tmp_node, 0, sizeof(struct interface_flow_point));
			tmp_node->graph_type = INTERFACE_FLOW_GRAPH_30DAY;
			memcpy(tmp_node->ifname, ifnode->ifname, IFNAMSIZ);
			tmp_node->rx = 0;
			tmp_node->tx = 0;
			tmp_node->ts.tv_sec = tmp_ts.tv_sec;
			tmp_node->ts.tv_usec= tmp_ts.tv_usec;
			interface_flow_convert_time(tmp_node->time, sizeof(tmp_node->time), tmp_ts.tv_sec - ts.tv_sec);
			add_interface_flow_record_to_list(record_list, tmp_node);
			XFREE(MTYPE_GUISH, tmp_node);
			tmp_ts.tv_sec += 12*3600;
		}
#if 0
		/*1m graph record*/
		rnode = listnode_tail(ifnode->record_1m);
		if((rnode == NULL) || (ts.tv_sec - rnode->ts.tv_sec >= 3600))
			tmp_ts.tv_sec = ts.tv_sec - 3600;
		else
			memcpy(&tmp_ts, &(rnode->ts), sizeof(struct timeval));

		while(ts.tv_sec - tmp_ts.tv_sec >= 0) {
			tmp_node = XMALLOC(MTYPE_GUISH, sizeof(struct interface_flow_point));
			memset(tmp_node, 0, sizeof(struct interface_flow_point));
			tmp_node->graph_type = INTERFACE_FLOW_GRAPH_MIN;
			memcpy(tmp_node->ifname, ifnode->ifname, IFNAMSIZ);
			tmp_node->rx = 0;
			tmp_node->tx = 0;
			tmp_node->ts.tv_sec = tmp_ts.tv_sec;
			tmp_node->ts.tv_usec= tmp_ts.tv_usec;
			interface_flow_convert_time(tmp_node->time, sizeof(tmp_node->time), tmp_ts.tv_sec - ts.tv_sec);
			add_interface_flow_record_to_list(record_list, tmp_node);
			XFREE(MTYPE_GUISH, tmp_node);
			tmp_ts.tv_sec += 60;
		}

		/*1h graph record*/
		rnode = listnode_tail(ifnode->record_1h);
		if((rnode == NULL) || (ts.tv_sec - rnode->ts.tv_sec >= 3600 *24))
			tmp_ts.tv_sec = ts.tv_sec -3600*24;
		else
			memcpy(&tmp_ts, &(rnode->ts), sizeof(struct timeval));
		while(ts.tv_sec - tmp_ts.tv_sec >= 0) {
			tmp_node = XMALLOC(MTYPE_GUISH, sizeof(struct interface_flow_point));
			memset(tmp_node, 0, sizeof(struct interface_flow_point));
			tmp_node->graph_type = INTERFACE_FLOW_GRAPH_HOUR;
			memcpy(tmp_node->ifname, ifnode->ifname, IFNAMSIZ);
			tmp_node->rx = 0;
			tmp_node->tx = 0;
			tmp_node->ts.tv_sec = tmp_ts.tv_sec;
			tmp_node->ts.tv_usec= tmp_ts.tv_usec;
			interface_flow_convert_time(tmp_node->time, sizeof(tmp_node->time), tmp_ts.tv_sec - ts.tv_sec );
			add_interface_flow_record_to_list(record_list, tmp_node);
			XFREE(MTYPE_GUISH, tmp_node);
			tmp_ts.tv_sec += 3600;
		}

		/*3h graph record*/
		rnode = listnode_tail(ifnode->record_3h);
		if((rnode == NULL) || (ts.tv_sec - rnode->ts.tv_sec >= 3600 *24*7))
			tmp_ts.tv_sec = ts.tv_sec -3600*24*7;
		else
			memcpy(&tmp_ts, &(rnode->ts), sizeof(struct timeval));
		while(ts.tv_sec - tmp_ts.tv_sec >= 0) {
			tmp_node = XMALLOC(MTYPE_GUISH, sizeof(struct interface_flow_point));
			memset(tmp_node, 0, sizeof(struct interface_flow_point));
			tmp_node->graph_type = INTERFACE_FLOW_GRAPH_DAY;
			memcpy(tmp_node->ifname, ifnode->ifname, IFNAMSIZ);
			tmp_node->rx = 0;
			tmp_node->tx = 0;
			tmp_node->ts.tv_sec = tmp_ts.tv_sec;
			tmp_node->ts.tv_usec= tmp_ts.tv_usec;
			interface_flow_convert_time(tmp_node->time, sizeof(tmp_node->time), tmp_ts.tv_sec - ts.tv_sec);
			add_interface_flow_record_to_list(record_list, tmp_node);
			XFREE(MTYPE_GUISH, tmp_node);
			tmp_ts.tv_sec += 3600*3;
		}
#endif
	}
	return 0;
}

int add_interface_flow_record_to_list(list record_list, struct interface_flow_point *record)
{
	struct listnode *nn = NULL;
	struct interface_flow *if_node = NULL;
	struct interface_flow_point *record_node = NULL;
	int interface_found = 0;

	LIST_LOOP (record_list, if_node, nn) {
		if(strcmp(if_node->ifname, record->ifname) == 0) {
			interface_found = 1;
			record_node = XMALLOC(MTYPE_GUISH, sizeof(struct interface_flow_point));
			memset(record_node, 0, sizeof(struct interface_flow_point));
			memcpy(record_node, record, sizeof(struct interface_flow_point));

			/*
						if(record->graph_type == INTERFACE_FLOW_GRAPH_MIN)
						{
							if(if_node->list_len_1m >= 60)
							{
								struct interface_flow_point *tmp = (struct interface_flow_point *)listnode_head (if_node->record_1m);
								XFREE(MTYPE_GUISH, tmp);
								listnode_delete_first(if_node->record_1m);
								if_node->list_len_1m --;
							}
							listnode_add(if_node->record_1m, record_node);
							if_node->list_len_1m ++;
						}

						if(record->graph_type == INTERFACE_FLOW_GRAPH_HOUR)
						{
							if(if_node->list_len_1h >= 24)
							{
								struct interface_flow_point *tmp = (struct interface_flow_point *)listnode_head (if_node->record_1h);
								XFREE(MTYPE_GUISH, tmp);
								listnode_delete_first(if_node->record_1h);
								if_node->list_len_1h --;
							}
							listnode_add(if_node->record_1h, record_node);
							if_node->list_len_1h ++;
						}

						if(record->graph_type == INTERFACE_FLOW_GRAPH_DAY)
						{
							if(if_node->list_len_3h >= 56)
							{
								struct interface_flow_point *tmp = (struct interface_flow_point *)listnode_head (if_node->record_3h);
								XFREE(MTYPE_GUISH, tmp);
								listnode_delete_first(if_node->record_3h);
								if_node->list_len_3h --;
							}
							listnode_add(if_node->record_3h, record_node);
							if_node->list_len_3h ++;
						}
			*/
			if(record->graph_type == INTERFACE_FLOW_GRAPH_30MIN) {
				if(if_node->list_len_30s >= 60) {
					struct interface_flow_point *tmp = (struct interface_flow_point *)listnode_head (if_node->record_30s);
					XFREE(MTYPE_GUISH, tmp);
					listnode_delete_first(if_node->record_30s);
					if_node->list_len_30s --;
				}
				listnode_add(if_node->record_30s, record_node);
				if_node->list_len_30s++;
			}
			if(record->graph_type == INTERFACE_FLOW_GRAPH_3HOUR) {
				if(if_node->list_len_3m >= 60) {
					struct interface_flow_point *tmp = (struct interface_flow_point *)listnode_head (if_node->record_3m);
					XFREE(MTYPE_GUISH, tmp);
					listnode_delete_first(if_node->record_3m);
					if_node->list_len_3m --;
				}
				listnode_add(if_node->record_3m, record_node);
				if_node->list_len_3m++;
			}
			if(record->graph_type == INTERFACE_FLOW_GRAPH_24HOUR) {
				if(if_node->list_len_1h >= 24) {
					struct interface_flow_point *tmp = (struct interface_flow_point *)listnode_head (if_node->record_1h);
					XFREE(MTYPE_GUISH, tmp);
					listnode_delete_first(if_node->record_1h);
					if_node->list_len_1h --;
				}
				listnode_add(if_node->record_1h, record_node);
				if_node->list_len_1h++;
			}
			if(record->graph_type == INTERFACE_FLOW_GRAPH_7DAY) {
				if(if_node->list_len_3h >= 56) {
					struct interface_flow_point *tmp = (struct interface_flow_point *)listnode_head (if_node->record_3h);
					XFREE(MTYPE_GUISH, tmp);
					listnode_delete_first(if_node->record_3h);
					if_node->list_len_3h --;
				}
				listnode_add(if_node->record_3h, record_node);
				if_node->list_len_3h++;
			}
			if(record->graph_type == INTERFACE_FLOW_GRAPH_30DAY) {
				if(if_node->list_len_12h >= 60) {
					struct interface_flow_point *tmp = (struct interface_flow_point *)listnode_head (if_node->record_12h);
					XFREE(MTYPE_GUISH, tmp);
					listnode_delete_first(if_node->record_12h);
					if_node->list_len_12h --;
				}
				listnode_add(if_node->record_12h, record_node);
				if_node->list_len_12h++;
			}
		}
	}
	if(interface_found == 0) {
		if_node = XMALLOC(MTYPE_GUISH, sizeof(struct interface_flow));
		memset(if_node, 0, sizeof(struct interface_flow));
		strcpy(if_node->ifname, record->ifname);
		/*
				if_node->record_1m = list_new();
				if_node->record_1m->del = del_interface_flow_record;
				if_node->list_len_1m = 0;
				if_node->record_1h = list_new();
				if_node->record_1h->del = del_interface_flow_record;
				if_node->list_len_1h = 0;
				if_node->record_3h = list_new();
				if_node->record_3h->del = del_interface_flow_record;
				if_node->list_len_3h= 0;
		*/
		if_node->record_30s = list_new();
		if_node->record_30s->del = del_interface_flow_record;
		if_node->list_len_30s = 0;
		if_node->record_3m = list_new();
		if_node->record_3m->del = del_interface_flow_record;
		if_node->list_len_3m = 0;
		if_node->record_1h = list_new();
		if_node->record_1h->del = del_interface_flow_record;
		if_node->list_len_1h= 0;
		if_node->record_3h = list_new();
		if_node->record_3h->del = del_interface_flow_record;
		if_node->list_len_3h= 0;
		if_node->record_12h = list_new();
		if_node->record_12h->del = del_interface_flow_record;
		if_node->list_len_12h= 0;

		listnode_add(record_list, if_node);
		add_interface_flow_record_to_list(record_list, record);
	}

	return 0;
}

int init_interface_node(list record_list)
{
//	u32 per_band=0;
	FILE *fp;
	char buf[1024 + 1];
	int ret;
	struct interface_info ife;

	/* Open /proc/net/dev. */
	ret = 0;
	int ver =0;
	fp = fopen (_PATH_PROC_NET_DEV, "r");
	if (fp == NULL)
		return -1;

	/* Drop header lines. */
	fgets (buf, 1024, fp);
	fgets (buf, 1024, fp);
	ver =procnetdev_version(buf);

	while (fgets (buf, 1024, fp) != NULL) {
		buf[1024] = '\0';
		char *s, name[IFNAMSIZ];
		s = get_name(name, buf);
		{
			memset(&ife,0,sizeof (struct interface_info));
			strncpy(ife.name,name,IFNAMSIZ);

			if(if_type_get(name) != IF_ETHERNET)
				continue;
			if(!strcmp(ife.name, "npec"))
				continue;
			if(!strncmp(ife.name, "swi", 3))
				continue;
			if(!strncmp(ife.name, "loop", 4))
				continue;
			if(!strncmp("nsa", name, 3))
				continue;

			if(!strncmp("lo", name, 2))
				continue;
			if(!strncmp("eth", name, 3))
				continue;
			if(!strncmp("tvi", name, 3))
				continue;
			if(!strncmp("vlan", name, 4))
				continue;

			get_dev_fields(ver, s, &ife);
			if(fetch_interface(&ife)<0)
				continue;

			/*
						if((ife.type == ARPHRD_ETHER)&&(ife.ifHwIntf == 1))
						{

							if(ife.max_speed == SPEED1000)
								per_band = 1000 * 1000 * 2;
							else if(ife.max_speed == SPEED100)
								per_band = 100 *1000 * 2;
							else
								per_band = 10 * 1000 * 2;
							//info->band += per_band;
						}
			*/
			if(ife.statistics_valid == 0) {
				struct interface_flow *tmp_node;
				tmp_node = XMALLOC(MTYPE_GUISH, sizeof(struct interface_flow));
				memset(tmp_node, 0, sizeof(struct interface_flow));
				memcpy(tmp_node->ifname, ife.name, IFNAMSIZ);
				/*
								tmp_node->record_1m = list_new();
								tmp_node->record_1m->del= del_interface_flow_record;
								tmp_node->list_len_1m = 0;
								tmp_node->record_1h = list_new();
								tmp_node->record_1h->del= del_interface_flow_record;
								tmp_node->list_len_1h = 0;
								tmp_node->record_3h = list_new();
								tmp_node->record_3h->del= del_interface_flow_record;
								tmp_node->list_len_3h= 0;
				*/
				tmp_node->record_30s = list_new();
				tmp_node->record_30s->del = del_interface_flow_record;
				tmp_node->list_len_30s = 0;
				tmp_node->record_3m = list_new();
				tmp_node->record_3m->del = del_interface_flow_record;
				tmp_node->list_len_3m = 0;
				tmp_node->record_1h = list_new();
				tmp_node->record_1h->del = del_interface_flow_record;
				tmp_node->list_len_1h = 0;
				tmp_node->record_3h = list_new();
				tmp_node->record_3h->del= del_interface_flow_record;
				tmp_node->list_len_3h = 0;
				tmp_node->record_12h = list_new();
				tmp_node->record_12h->del = del_interface_flow_record;
				tmp_node->list_len_12h = 0;

				listnode_add(record_list, tmp_node);
			}
		}
	}

	fclose(fp);/*?*/

	return 0;
}

int init_interface_flow(void)
{
	int ret;

#if 0
	DB *dbp;
	DBC *cursorp;
	DBT key, data;
#endif

	interface_flow_record = list_new();
	interface_flow_record->del = del_interface_flow_record;
	init_interface_node(interface_flow_record);

#if 0
	if ((ret = db_create(&dbp, NULL, 0)) != 0) {
		return 1;
	}
	if ((ret = dbp->open(dbp, NULL, INTERFACE_FLOW_DB, NULL, DB_BTREE, DB_CREATE, 0664)) != 0) {
		return 2;
	}

	dbp->cursor(dbp, NULL, &cursorp, 0);

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	/* Iterate over the database, retrieving each record in turn. */
	/*DB_NEXT, from first to last*/
	/*DB_PREV, form last to first*/
	while ((ret = cursorp->c_get(cursorp, &key, &data, DB_NEXT)) == 0) {
		add_interface_flow_record_to_list(interface_flow_record, (struct interface_flow_point *)data.data);
		memset(&key, 0, sizeof(DBT));
		memset(&data, 0, sizeof(DBT));
	}

	/* Cursors must be closed */
	if (cursorp != NULL)
		cursorp->c_close(cursorp);

	ret = dbp->close(dbp, 0);
	if(ret != 0)
		return 3;
#endif

	add_interface_flow_zero_record(interface_flow_record);
	init_interface_flow_timer(interface_flow_record, &interface_flow_timer);

	return 0;
}

struct interface_flow_xml interface_flow_xml_data;

//PARSE_XML_INT(interface_flow_xml, graph_type, struct interface_flow_xml)
PARSE_XML_INT(interface_flow_xml, period, struct interface_flow_xml)
PARSE_XML_INT(interface_flow_xml, tx, struct interface_flow_xml)
PARSE_XML_INT(interface_flow_xml, rx, struct interface_flow_xml)
PARSE_XML_INTERFACE(interface_flow_xml, ifname, struct interface_flow_xml, IFNAMSIZ)
PARSE_XML_STRING(interface_flow_xml, time, struct interface_flow_xml, XML_MAX_NAME_LEN)

struct element_node_parameter interface_flow_element_parameter[] = {
//	HASH_NODE_PARAM_ADD(interface_flow_xml, graph_type, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_flow_xml, period, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_flow_xml, tx, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_flow_xml, rx, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(interface_flow_xml, ifname, TYPE_XML_INTERFACE),
	HASH_NODE_PARAM_ADD(interface_flow_xml, time, TYPE_XML_STRING),
	{"", NULL, NULL, NULL, TYPE_XML_INT}
};

extern char * interface_flow_show(void *ptr);
struct module_node_parameter interface_flow_module_parameter = {
	"interface_flow_xml",
	NULL,
	NULL,
	NULL,
	interface_flow_show,
	NULL,
	NULL,
	NULL,
	1,
	sizeof(struct interface_flow_xml),
};

#if 0
char * interface_flow_show(void *ptr)
{
	char ifname[IFNAMSIZ + 1];
	struct interface_flow_xml *data = (struct interface_flow_xml *)ptr;
	memcpy(ifname, data->ifname, IFNAMSIZ);

	xmlNode *cur;
	xmlNode *child;
	char * send_buf;
	xmlDoc *global_doc ;
	char tmp_buf[20];

	child = xml_start_more(&global_doc, "interface_flow_xml");

	struct listnode *nn = NULL;
	struct listnode *nn_tmp = NULL;
	struct interface_flow *if_node = NULL;
	struct interface_flow_point *record_node = NULL;

	if (data->ifname[0]) {
		LIST_LOOP (interface_flow_record, if_node, nn) {
			if(strcmp(if_node->ifname, ifname) == 0) {
				if(data->graph_type == 1) {
					LIST_LOOP (if_node->record_1m, record_node, nn_tmp) {
						cur = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );

						memset(tmp_buf, 0, 20);
						snprintf(tmp_buf, 19, "%d", record_node->graph_type);
						vsos_xmlNewChild( cur, NULL, BAD_CAST"graph_type", BAD_CAST tmp_buf);

						memset(tmp_buf, 0, 20);
						snprintf(tmp_buf, 19, "%d", record_node->tx);
						vsos_xmlNewChild( cur, NULL, BAD_CAST"tx", BAD_CAST tmp_buf);

						memset(tmp_buf, 0, 20);
						snprintf(tmp_buf, 19, "%d", record_node->rx);
						vsos_xmlNewChild( cur, NULL, BAD_CAST"rx", BAD_CAST tmp_buf);

						vsos_xmlNewChild( cur, NULL, BAD_CAST"ifname", BAD_CAST record_node->ifname);
						vsos_xmlNewChild( cur, NULL, BAD_CAST"time", BAD_CAST record_node->time);
					}
				}
				if(data->graph_type == 2) {
					LIST_LOOP (if_node->record_1h, record_node, nn_tmp) {
						cur = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );

						memset(tmp_buf, 0, 20);
						snprintf(tmp_buf, 19, "%d", record_node->graph_type);
						vsos_xmlNewChild( cur, NULL, BAD_CAST"graph_type", BAD_CAST tmp_buf);

						memset(tmp_buf, 0, 20);
						snprintf(tmp_buf, 19, "%d", record_node->tx);
						vsos_xmlNewChild( cur, NULL, BAD_CAST"tx", BAD_CAST tmp_buf);

						memset(tmp_buf, 0, 20);
						snprintf(tmp_buf, 19, "%d", record_node->rx);
						vsos_xmlNewChild( cur, NULL, BAD_CAST"rx", BAD_CAST tmp_buf);

						vsos_xmlNewChild( cur, NULL, BAD_CAST"ifname", BAD_CAST record_node->ifname);
						vsos_xmlNewChild( cur, NULL, BAD_CAST"time", BAD_CAST record_node->time);
					}
				}
				if(data->graph_type == 3) {
					LIST_LOOP (if_node->record_3h, record_node, nn_tmp) {
						cur = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );

						memset(tmp_buf, 0, 20);
						snprintf(tmp_buf, 19, "%d", record_node->graph_type);
						vsos_xmlNewChild( cur, NULL, BAD_CAST"graph_type", BAD_CAST tmp_buf);

						memset(tmp_buf, 0, 20);
						snprintf(tmp_buf, 19, "%d", record_node->tx);
						vsos_xmlNewChild( cur, NULL, BAD_CAST"tx", BAD_CAST tmp_buf);

						memset(tmp_buf, 0, 20);
						snprintf(tmp_buf, 19, "%d", record_node->rx);
						vsos_xmlNewChild( cur, NULL, BAD_CAST"rx", BAD_CAST tmp_buf);

						vsos_xmlNewChild( cur, NULL, BAD_CAST"ifname", BAD_CAST record_node->ifname);
						vsos_xmlNewChild( cur, NULL, BAD_CAST"time", BAD_CAST record_node->time);
					}
				}
			}
		}
	} else {
		if(data->graph_type == 1) {
#define ONE_HOUR_POINTS 60
			char time[ONE_HOUR_POINTS][XML_MAX_TIME_LEN];
			u32 total_tx[ONE_HOUR_POINTS];
			u32 total_rx[ONE_HOUR_POINTS];
			int tmp_num = 0;
			int tmp_seq = 0;

			memset(total_tx, 0, sizeof(u32)*ONE_HOUR_POINTS);
			memset(total_rx, 0, sizeof(u32)*ONE_HOUR_POINTS);
			memset(time, 0, ONE_HOUR_POINTS*XML_MAX_TIME_LEN);

			LIST_LOOP (interface_flow_record, if_node, nn) {
				tmp_seq = 0;
				LIST_LOOP (if_node->record_1m, record_node, nn_tmp) {
					total_tx[tmp_seq] += record_node->tx;
					total_rx[tmp_seq] += record_node->rx;
					strncpy(time[tmp_seq], record_node->time, XML_MAX_TIME_LEN-1);
					tmp_seq++;
				}
			}

			for (tmp_num = 0; tmp_num < ONE_HOUR_POINTS; tmp_num++) {
				cur = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );

				memset(tmp_buf, 0, 20);
				snprintf(tmp_buf, 19, "%d", data->graph_type);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"graph_type", BAD_CAST tmp_buf);

				memset(tmp_buf, 0, 20);
				snprintf(tmp_buf, 19, "%d", total_tx[tmp_num]);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"tx", BAD_CAST tmp_buf);

				memset(tmp_buf, 0, 20);
				snprintf(tmp_buf, 19, "%d", total_rx[tmp_num]);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"rx", BAD_CAST tmp_buf);

				vsos_xmlNewChild( cur, NULL, BAD_CAST"ifname", BAD_CAST "");
				vsos_xmlNewChild( cur, NULL, BAD_CAST"time", BAD_CAST time[tmp_num]);
			}
		} else if(data->graph_type == 2)	{
#define ONE_DAY_POINTS 24
			char time[ONE_DAY_POINTS][XML_MAX_TIME_LEN];
			u32 total_tx[ONE_DAY_POINTS];
			u32 total_rx[ONE_DAY_POINTS];
			int tmp_num = 0;
			int tmp_seq = 0;

			memset(total_tx, 0, sizeof(u32)*ONE_DAY_POINTS);
			memset(total_rx, 0, sizeof(u32)*ONE_DAY_POINTS);
			memset(time, 0, ONE_DAY_POINTS*XML_MAX_TIME_LEN);

			LIST_LOOP (interface_flow_record, if_node, nn) {
				tmp_seq = 0;
				LIST_LOOP (if_node->record_1h, record_node, nn_tmp) {
					total_tx[tmp_seq] += record_node->tx;
					total_rx[tmp_seq] += record_node->rx;
					strncpy(time[tmp_seq], record_node->time, XML_MAX_TIME_LEN-1);
					tmp_seq++;
				}
			}

			for (tmp_num = 0; tmp_num < ONE_DAY_POINTS; tmp_num++) {
				cur = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );

				memset(tmp_buf, 0, 20);
				snprintf(tmp_buf, 19, "%d", data->graph_type);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"graph_type", BAD_CAST tmp_buf);

				memset(tmp_buf, 0, 20);
				snprintf(tmp_buf, 19, "%d", total_tx[tmp_num]);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"tx", BAD_CAST tmp_buf);

				memset(tmp_buf, 0, 20);
				snprintf(tmp_buf, 19, "%d", total_rx[tmp_num]);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"rx", BAD_CAST tmp_buf);

				vsos_xmlNewChild( cur, NULL, BAD_CAST"ifname", BAD_CAST "");
				vsos_xmlNewChild( cur, NULL, BAD_CAST"time", BAD_CAST time[tmp_num]);
			}
		} else if(data->graph_type == 3) {
#define SEVEN_DAY_POINTS 56
			char time[SEVEN_DAY_POINTS][XML_MAX_TIME_LEN];
			u32 total_tx[SEVEN_DAY_POINTS];
			u32 total_rx[SEVEN_DAY_POINTS];
			int tmp_num = 0;
			int tmp_seq = 0;

			memset(total_tx, 0, sizeof(u32)*SEVEN_DAY_POINTS);
			memset(total_rx, 0, sizeof(u32)*SEVEN_DAY_POINTS);
			memset(time, 0, SEVEN_DAY_POINTS*XML_MAX_TIME_LEN);

			LIST_LOOP (interface_flow_record, if_node, nn) {
				tmp_seq = 0;
				LIST_LOOP (if_node->record_3h, record_node, nn_tmp) {
					total_tx[tmp_seq] += record_node->tx;
					total_rx[tmp_seq] += record_node->rx;
					strncpy(time[tmp_seq], record_node->time, XML_MAX_TIME_LEN-1);
					tmp_seq++;
				}
			}

			for (tmp_num = 0; tmp_num < SEVEN_DAY_POINTS; tmp_num++) {
				cur = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );

				memset(tmp_buf, 0, 20);
				snprintf(tmp_buf, 19, "%d", data->graph_type);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"graph_type", BAD_CAST tmp_buf);

				memset(tmp_buf, 0, 20);
				snprintf(tmp_buf, 19, "%d", total_tx[tmp_num]);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"tx", BAD_CAST tmp_buf);

				memset(tmp_buf, 0, 20);
				snprintf(tmp_buf, 19, "%d", total_rx[tmp_num]);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"rx", BAD_CAST tmp_buf);

				vsos_xmlNewChild( cur, NULL, BAD_CAST"ifname", BAD_CAST "");
				vsos_xmlNewChild( cur, NULL, BAD_CAST"time", BAD_CAST time[tmp_num]);
			}
		}

	}
	send_buf = xml_end(global_doc, INTERFACE_FLOW_TMP);
	return send_buf;
}
#endif
char * interface_flow_show(void *ptr)
{
	char ifname[IFNAMSIZ + 1];
	struct interface_flow_xml *data = (struct interface_flow_xml *)ptr;
	memcpy(ifname, data->ifname, IFNAMSIZ);

	xmlNode *cur1, *cur2, *cur3;
	xmlNode *child;
	char * send_buf;
	xmlDoc *global_doc ;
	char tmp_buf[20];

	child = xml_start_more(&global_doc, "interface_flow_xml");

	struct listnode *nn = NULL;
	struct listnode *nn_tmp = NULL;
	struct interface_flow *if_node = NULL;
	struct interface_flow_point *record_node = NULL;

	cur1 = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );

	memset(tmp_buf, 0, 20);
	snprintf(tmp_buf, 19, "%d", data->period);
	vsos_xmlNewChild( cur1, NULL, BAD_CAST"period", BAD_CAST tmp_buf);

	cur2 = vsos_xmlNewChild( cur1, NULL, BAD_CAST"data", BAD_CAST NULL );

	if (data->ifname[0]) {
		/*
				LIST_LOOP (interface_flow_record, if_node, nn)
				{
					if(strcmp(if_node->ifname, ifname) == 0)
					{
						if(data->period == 0)
						{
							LIST_LOOP (if_node->record_30s, record_node, nn_tmp)
							{
								cur3 = vsos_xmlNewChild( cur2, NULL, BAD_CAST"group", BAD_CAST tmp_buf);

								memset(tmp_buf, 0, 20);
								snprintf(tmp_buf, 19, "%d", record_node->graph_type);

								vsos_xmlNewChild( cur3, NULL, BAD_CAST"group", BAD_CAST tmp_buf);
								memset(tmp_buf, 0, 20);
								snprintf(tmp_buf, 19, "%d", record_node->tx);
								vsos_xmlNewChild( cur3, NULL, BAD_CAST"tx", BAD_CAST tmp_buf);

								memset(tmp_buf, 0, 20);
								snprintf(tmp_buf, 19, "%d", record_node->rx);
								vsos_xmlNewChild( cur3, NULL, BAD_CAST"rx", BAD_CAST tmp_buf);

								vsos_xmlNewChild( cur3, NULL, BAD_CAST"ifname", BAD_CAST record_node->ifname);
								vsos_xmlNewChild( cur3, NULL, BAD_CAST"time", BAD_CAST record_node->time);
							}
						}
					}
				}
		*/
	} else {
		if(data->period == 0) {
#define P_30MIN_POINTS 60
			char time[P_30MIN_POINTS][XML_MAX_TIME_LEN];
			u32 total_tx[P_30MIN_POINTS];
			u32 total_rx[P_30MIN_POINTS];
			int tmp_num = 0;
			int tmp_seq = 0;

			memset(total_tx, 0, sizeof(u32)*P_30MIN_POINTS);
			memset(total_rx, 0, sizeof(u32)*P_30MIN_POINTS);
			memset(time, 0, P_30MIN_POINTS*XML_MAX_TIME_LEN);

			LIST_LOOP (interface_flow_record, if_node, nn) {
				tmp_seq = 0;
				LIST_LOOP (if_node->record_30s, record_node, nn_tmp) {
					total_tx[tmp_seq] += record_node->tx;
					total_rx[tmp_seq] += record_node->rx;
					strncpy(time[tmp_seq], record_node->time, XML_MAX_TIME_LEN-1);
					tmp_seq++;
				}
			}

			for (tmp_num = 0; tmp_num < P_30MIN_POINTS; tmp_num++) {
				cur3 = vsos_xmlNewChild( cur2, NULL, BAD_CAST"group", BAD_CAST NULL );

				vsos_xmlNewChild( cur3, NULL, BAD_CAST"ifname", BAD_CAST "");
				vsos_xmlNewChild( cur3, NULL, BAD_CAST"date", BAD_CAST time[tmp_num]);

				memset(tmp_buf, 0, 20);
				snprintf(tmp_buf, 19, "%d", total_tx[tmp_num]);
				vsos_xmlNewChild( cur3, NULL, BAD_CAST"tx", BAD_CAST tmp_buf);

				memset(tmp_buf, 0, 20);
				snprintf(tmp_buf, 19, "%d", total_rx[tmp_num]);
				vsos_xmlNewChild( cur3, NULL, BAD_CAST"rx", BAD_CAST tmp_buf);
			}
		} else if(data->period == 1)	{
#define P_3HOUR_POINTS 60
			char time[P_3HOUR_POINTS][XML_MAX_TIME_LEN];
			u32 total_tx[P_3HOUR_POINTS];
			u32 total_rx[P_3HOUR_POINTS];
			int tmp_num = 0;
			int tmp_seq = 0;

			memset(total_tx, 0, sizeof(u32)*P_3HOUR_POINTS);
			memset(total_rx, 0, sizeof(u32)*P_3HOUR_POINTS);
			memset(time, 0, P_3HOUR_POINTS*XML_MAX_TIME_LEN);

			LIST_LOOP (interface_flow_record, if_node, nn) {
				tmp_seq = 0;
				LIST_LOOP (if_node->record_3m, record_node, nn_tmp) {
					total_tx[tmp_seq] += record_node->tx;
					total_rx[tmp_seq] += record_node->rx;
					strncpy(time[tmp_seq], record_node->time, XML_MAX_TIME_LEN-1);
					tmp_seq++;
				}
			}

			for (tmp_num = 0; tmp_num < P_3HOUR_POINTS; tmp_num++) {
				cur3 = vsos_xmlNewChild( cur2, NULL, BAD_CAST"group", BAD_CAST NULL );

				vsos_xmlNewChild( cur3, NULL, BAD_CAST"ifname", BAD_CAST "");
				vsos_xmlNewChild( cur3, NULL, BAD_CAST"date", BAD_CAST time[tmp_num]);

				memset(tmp_buf, 0, 20);
				snprintf(tmp_buf, 19, "%d", total_tx[tmp_num]);
				vsos_xmlNewChild( cur3, NULL, BAD_CAST"tx", BAD_CAST tmp_buf);

				memset(tmp_buf, 0, 20);
				snprintf(tmp_buf, 19, "%d", total_rx[tmp_num]);
				vsos_xmlNewChild( cur3, NULL, BAD_CAST"rx", BAD_CAST tmp_buf);
			}
		} else if(data->period== 2) {
#define P_24HOUR_POINTS 24
			char time[P_24HOUR_POINTS][XML_MAX_TIME_LEN];
			u32 total_tx[P_24HOUR_POINTS];
			u32 total_rx[P_24HOUR_POINTS];
			int tmp_num = 0;
			int tmp_seq = 0;

			memset(total_tx, 0, sizeof(u32)*P_24HOUR_POINTS);
			memset(total_rx, 0, sizeof(u32)*P_24HOUR_POINTS);
			memset(time, 0, P_24HOUR_POINTS*XML_MAX_TIME_LEN);

			LIST_LOOP (interface_flow_record, if_node, nn) {
				tmp_seq = 0;
				LIST_LOOP (if_node->record_1h, record_node, nn_tmp) {
					total_tx[tmp_seq] += record_node->tx;
					total_rx[tmp_seq] += record_node->rx;
					strncpy(time[tmp_seq], record_node->time, XML_MAX_TIME_LEN-1);
					tmp_seq++;
				}
			}

			for (tmp_num = 0; tmp_num < P_24HOUR_POINTS; tmp_num++) {
				cur3 = vsos_xmlNewChild( cur2, NULL, BAD_CAST"group", BAD_CAST NULL );

				vsos_xmlNewChild( cur3, NULL, BAD_CAST"ifname", BAD_CAST "");
				vsos_xmlNewChild( cur3, NULL, BAD_CAST"date", BAD_CAST time[tmp_num]);

				memset(tmp_buf, 0, 20);
				snprintf(tmp_buf, 19, "%d", total_tx[tmp_num]);
				vsos_xmlNewChild( cur3, NULL, BAD_CAST"tx", BAD_CAST tmp_buf);

				memset(tmp_buf, 0, 20);
				snprintf(tmp_buf, 19, "%d", total_rx[tmp_num]);
				vsos_xmlNewChild( cur3, NULL, BAD_CAST"rx", BAD_CAST tmp_buf);
			}
		} else if(data->period == 3) {
#define P_7DAY_POINTS 56
			char time[P_7DAY_POINTS][XML_MAX_TIME_LEN];
			u32 total_tx[P_7DAY_POINTS];
			u32 total_rx[P_7DAY_POINTS];
			int tmp_num = 0;
			int tmp_seq = 0;

			memset(total_tx, 0, sizeof(u32)*P_7DAY_POINTS);
			memset(total_rx, 0, sizeof(u32)*P_7DAY_POINTS);
			memset(time, 0, P_7DAY_POINTS*XML_MAX_TIME_LEN);

			LIST_LOOP (interface_flow_record, if_node, nn) {
				tmp_seq = 0;
				LIST_LOOP (if_node->record_3h, record_node, nn_tmp) {
					total_tx[tmp_seq] += record_node->tx;
					total_rx[tmp_seq] += record_node->rx;
					strncpy(time[tmp_seq], record_node->time, XML_MAX_TIME_LEN-1);
					tmp_seq++;
				}
			}

			for (tmp_num = 0; tmp_num < P_7DAY_POINTS; tmp_num++) {
				cur3 = vsos_xmlNewChild( cur2, NULL, BAD_CAST"group", BAD_CAST NULL );

				vsos_xmlNewChild( cur3, NULL, BAD_CAST"ifname", BAD_CAST "");
				vsos_xmlNewChild( cur3, NULL, BAD_CAST"date", BAD_CAST time[tmp_num]);

				memset(tmp_buf, 0, 20);
				snprintf(tmp_buf, 19, "%d", total_tx[tmp_num]);
				vsos_xmlNewChild( cur3, NULL, BAD_CAST"tx", BAD_CAST tmp_buf);

				memset(tmp_buf, 0, 20);
				snprintf(tmp_buf, 19, "%d", total_rx[tmp_num]);
				vsos_xmlNewChild( cur3, NULL, BAD_CAST"rx", BAD_CAST tmp_buf);
			}
		} else if(data->period == 4) {
#define P_30DAY_POINTS 60
			char time[P_30DAY_POINTS][XML_MAX_TIME_LEN];
			u32 total_tx[P_30DAY_POINTS];
			u32 total_rx[P_30DAY_POINTS];
			int tmp_num = 0;
			int tmp_seq = 0;

			memset(total_tx, 0, sizeof(u32)*P_30DAY_POINTS);
			memset(total_rx, 0, sizeof(u32)*P_30DAY_POINTS);
			memset(time, 0, P_30DAY_POINTS*XML_MAX_TIME_LEN);

			LIST_LOOP (interface_flow_record, if_node, nn) {
				tmp_seq = 0;
				LIST_LOOP (if_node->record_12h, record_node, nn_tmp) {
					total_tx[tmp_seq] += record_node->tx;
					total_rx[tmp_seq] += record_node->rx;
					strncpy(time[tmp_seq], record_node->time, XML_MAX_TIME_LEN-1);
					tmp_seq++;
				}
			}

			for (tmp_num = 0; tmp_num < P_30DAY_POINTS; tmp_num++) {
				cur3 = vsos_xmlNewChild( cur2, NULL, BAD_CAST"group", BAD_CAST NULL );

				vsos_xmlNewChild( cur3, NULL, BAD_CAST"ifname", BAD_CAST "");
				vsos_xmlNewChild( cur3, NULL, BAD_CAST"date", BAD_CAST time[tmp_num]);

				memset(tmp_buf, 0, 20);
				snprintf(tmp_buf, 19, "%d", total_tx[tmp_num]);
				vsos_xmlNewChild( cur3, NULL, BAD_CAST"tx", BAD_CAST tmp_buf);

				memset(tmp_buf, 0, 20);
				snprintf(tmp_buf, 19, "%d", total_rx[tmp_num]);
				vsos_xmlNewChild( cur3, NULL, BAD_CAST"rx", BAD_CAST tmp_buf);
			}
		}

	}
	send_buf = xml_end(global_doc, INTERFACE_FLOW_TMP);
	return send_buf;
}
/*end of interface flow implemation*/



/**********************interface statisitc**********************/
struct interface_statistics interface_statistics_data;

PARSE_XML_INTERFACE(interface_statistics, name, struct interface_statistics, XML_MAX_NAME_LEN)
PARSE_XML_INT(interface_statistics, type, struct interface_statistics)

struct element_node_parameter interface_statistics_element_parameter[] = {
	HASH_NODE_PARAM_ADD(interface_statistics, name, TYPE_XML_INTERFACE),
	HASH_NODE_PARAM_ADD(interface_statistics, type, TYPE_XML_INT),
	{"", NULL, NULL, NULL, TYPE_XML_INT}
};

int xml_vlan_interface_stat_clear(char *ifname)
{
	char buf[64];
	memset(buf, 0, 64);
	if(ifname!= NULL) {
		strncpy(buf, ifname, 64);
	}

	dataplane_request(UIPC_BR_TR_CLEAR_COUNTER, buf, 64, 64, 0);

	return 0;
}

int xml_vlan_interface_stat(char *ifname, struct interface_info *ife)
{
	struct vlan_if_info {
		char vlan_name[16];
		unsigned long long rx_packets;
		unsigned long long tx_packets;
		unsigned long long rx_bytes;
		unsigned long long tx_bytes;
		unsigned long rx_packets_traffic;
		unsigned long tx_packets_traffic;
		unsigned long rx_bytes_traffic;
		unsigned long tx_bytes_traffic;
	} vlan_buf;

	memset(&vlan_buf, 0, sizeof(vlan_buf));
	strncpy(vlan_buf.vlan_name, ifname, 16);
	dataplane_request(UIPC_TB_BR_STAT, (char*)&vlan_buf, sizeof(vlan_buf), sizeof(vlan_buf), 0);

	ife->stats.rx_packets=vlan_buf.rx_packets;
	ife->stats.rx_bytes=vlan_buf.rx_bytes;
	ife->stats.tx_packets=vlan_buf.tx_packets;
	ife->stats.tx_bytes=vlan_buf.tx_bytes;
	ife->rx_packets_traffic = vlan_buf.rx_packets_traffic;
	ife->tx_packets_traffic = vlan_buf.tx_packets_traffic;
	ife->rx_bytes_traffic = vlan_buf.rx_bytes_traffic;
	ife->tx_bytes_traffic = vlan_buf.tx_bytes_traffic;

	return 0;
}
int xml_trunk_interface_stat(char *ifname, struct interface_info *ife)
{
	struct tr_if_info {
		char tr_name[16];
		unsigned long long rx_packets;
		unsigned long long tx_packets;
		unsigned long long rx_bytes;
		unsigned long long tx_bytes;
	} tr_buf;

	memset(&tr_buf, 0, sizeof(tr_buf));
	strncpy(tr_buf.tr_name, ifname, 16);
	dataplane_request(UIPC_TB_TRUNK_STAT, (char*)&tr_buf, sizeof(tr_buf), sizeof(tr_buf), 0);

	ife->stats.rx_packets=tr_buf.rx_packets;
	ife->stats.rx_bytes=tr_buf.rx_bytes;
	ife->stats.tx_packets=tr_buf.tx_packets;
	ife->stats.tx_bytes=tr_buf.tx_bytes;

	return 0;
}

extern int dp_netns_get_vrfid_by_ifname(char *ifname);

char *xml_interface_statistics_showall(void *ptr)
{
	xmlNode *cur;
	xmlNode *child;
	char *ifname = NULL;
	char * send_buf;
	FILE *fp;
	char buf[1024 + 1];
	int ret;
	struct interface_info ife;
	struct interface_statistics *if_statistics = NULL;
	xmlDoc *global_doc ;
	char tmp_name[64];
	int cur_vsysid = GUI_CUR_VSYSID, if_vsysid;

	if_statistics = (struct interface_statistics *)ptr;
	if(if_statistics)
		ifname = if_statistics->name;

	/* Open /proc/net/dev. */
	ret = 0;
	int ver =0;
	fp = fopen (_PATH_PROC_NET_DEV, "r");
	if (fp == NULL)
		return NULL;

	/* Drop header lines. */
	fgets (buf, 1024, fp);
	fgets (buf, 1024, fp);
	ver =procnetdev_version(buf);

	child = xml_start_more(&global_doc, "interface_statistics");

	while (fgets (buf, 1024, fp) != NULL) {
		buf[1024] = '\0';
		char *s, name[IFNAMSIZ];
		s = get_name(name, buf);
		if(strlen(ifname)==0 ||!strcmp(name,ifname)) {
			char buf[50];
			struct prefix addr;
			memset(&ife,0,sizeof (struct interface_info));
			strncpy(ife.name,name,IFNAMSIZ);

			if(if_type_get(name) != IF_ETHERNET)
				continue;
			if(!strcmp(ife.name, "npec"))
				continue;
			if(!strncmp(ife.name, "swi", 3))
				continue;
			if(!strncmp("nsa", name, 3))
				continue;
			if(!strncmp(ife.name, "swi",3))
				continue;
			if(!strncmp(ife.name, "loop", 4))
				continue;
			if(!strncmp(ife.name, "lo", 2))
				continue;
			if(!strncmp(ife.name, "eth", 3))
				continue;
			if(strchr(ife.name, '.'))
				continue;

			if(if_statistics->type==0 && (!strncmp(ife.name, "tvi", 3)))
				continue;
			if(if_statistics->type==1 && (strncmp(ife.name, "tvi", 3)))
				continue;
			if(if_statistics->type==2 && (strncmp(ife.name, "vlan", 4)))
				continue;

			get_dev_fields(ver, s, &ife);
			if(fetch_interface(&ife)<0)
				continue;

            if_vsysid = dp_netns_get_vrfid_by_ifname(ife.name);
            if (if_vsysid != GUI_CUR_VSYSID && cur_vsysid != 0) {
                continue;
            }

			if(!strncmp(ife.name, "tvi", 3)) {
				if(xml_trunk_interface_stat(name, &ife)<0) {
					ife.stats.rx_bytes=0;
					ife.stats.rx_packets=0;
					ife.stats.tx_bytes=0;
					ife.stats.tx_packets=0;
				}
			}
			if(!strncmp(ife.name, "vlan", 4)) {
				if(xml_vlan_interface_stat(name, &ife)<0) {
					ife.stats.rx_bytes=0;
					ife.stats.rx_packets=0;
					ife.stats.tx_bytes=0;
					ife.stats.tx_packets=0;
				}
			}


			cur = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );
			memset(tmp_name, 0, 64);
			if_get_alias_by_name(ife.name, tmp_name);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"name", BAD_CAST tmp_name);

			if(ife.flags& IFF_RUNNING)
				vsos_xmlNewChild( cur, NULL, BAD_CAST"status", BAD_CAST "1");
			else
				vsos_xmlNewChild( cur, NULL, BAD_CAST"status", BAD_CAST "0");


			if(ife.statistics_valid == 0) {
				memset(buf, 0, 50);
				snprintf(buf, 49, "%Lu", ife.stats.rx_bytes);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"rx_bytes", BAD_CAST buf);

				memset(buf, 0, 50);
				snprintf(buf, 49, "%Lu", ife.stats.tx_bytes);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"tx_bytes", BAD_CAST buf);

				memset(buf, 0, 50);
				snprintf(buf, 49, "%Lu", ife.stats.rx_packets);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"rx_packets", BAD_CAST buf);

				memset(buf, 0, 50);
				snprintf(buf, 49, "%Lu", ife.stats.tx_packets);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"tx_packets", BAD_CAST buf);

				memset(buf, 0, 50);
				snprintf(buf, 49, "%lu", ife.stats.rx_dropped);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"rx_dropped", BAD_CAST buf);

				memset(buf, 0, 50);
				snprintf(buf, 49, "%lu", ife.stats.tx_dropped);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"tx_dropped", BAD_CAST buf);

				memset(buf, 0, 50);
				snprintf(buf, 49, "%lu", ife.stats.rx_errors);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"rx_errors", BAD_CAST buf);

				memset(buf, 0, 50);
				snprintf(buf, 49, "%lu", ife.stats.tx_errors);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"tx_errors", BAD_CAST buf);

				memset(buf, 0, 50);
				snprintf(buf, 49, "%lu", ife.stats.rx_fifo_errors);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"rx_overruns", BAD_CAST buf);

				memset(buf, 0, 50);
				snprintf(buf, 49, "%lu", ife.stats.tx_fifo_errors);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"tx_overruns", BAD_CAST buf);

				memset(buf, 0, 50);
				snprintf(buf, 49, "%lu", ife.stats.collisions);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"collisions", BAD_CAST buf);


				memset(buf, 0, 50);
				snprintf(buf, 49, "%.3f", (float)(ife.rx_bytes_traffic<<3)/1024);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"rx_speed_bytes", BAD_CAST buf);

				memset(buf, 0, 50);
				snprintf(buf, 49, "%.3f", (float)(ife.tx_bytes_traffic<<3)/1024);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"tx_speed_bytes", BAD_CAST buf);

				memset(buf, 0, 50);
				snprintf(buf, 49, "%lu", ife.rx_packets_traffic);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"rx_speed_packets", BAD_CAST buf);

				memset(buf, 0, 50);
				snprintf(buf, 49, "%lu", ife.tx_packets_traffic);
				vsos_xmlNewChild( cur, NULL, BAD_CAST"tx_speed_packets", BAD_CAST buf);

			}
		}
	}

	fclose(fp);/*?*/
	send_buf = xml_end(global_doc, ZEBRA_SAVE_TMP);
	return send_buf;
}

unsigned long xml_interface_statistics_del(void *ptr)
{
	FILE *fp=NULL;
	char buf[1024+1];
	int ret=0;
	int ver=0;
	char *ifname = NULL;
	struct interface_statistics *if_statistics = NULL;
	int cur_vsysid = GUI_CUR_VSYSID, if_vsysid;

	if_statistics = (struct interface_statistics *)ptr;
	if(if_statistics->name[0])
		ifname = if_statistics->name;

	/* Open /proc/net/dev */
	fp = fopen(_PATH_PROC_NET_DEV, "r");
	if(NULL==fp) {
		return -1;
	}

	/* Drop header lines */
	fgets(buf, 1024, fp);
	fgets(buf, 1024, fp);

	while(fgets(buf, 1024, fp)!=NULL) {
		buf[1024]='\0';
		char *s, name[IFNAMSIZ];
		s = get_name(name, buf);
		if_vsysid = dp_netns_get_vrfid_by_ifname(name);
        if (if_vsysid != GUI_CUR_VSYSID) {
            continue;
        }
		if((ifname==NULL)||!strcmp(name, ifname)) {
			if (strncmp(name, "vlan", 4) == 0)
				xml_vlan_interface_stat_clear(name);
			else
				bos_dev_ioctl(name, BOS_SIOETHIFCLRCNT, NULL);
		}
	}
	fclose(fp);
	return ret;
}

int interface_statistics_logcontent(unsigned char log, void *ptr, char *Operate)
{
    struct interface_statistics* data = (struct interface_statistics*)ptr;
    switch(log)
    {
        case WEB_OPERATION_MOD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Mod interface_statistics.");

        case WEB_OPERATION_ADD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Add interface_statistics.");

        case WEB_OPERATION_DEL:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Del interface_statistics.");

        default:
            return 0;
    }
}

struct module_node_parameter interface_statistics_module_parameter = {
	"interface_statistics",
	NULL,
	xml_interface_statistics_del,
	NULL,
	xml_interface_statistics_showall,
	NULL,
	xml_interface_statistics_showall,
	NULL,
	0,
	sizeof(struct interface_statistics),
	.func_logcontent = interface_statistics_logcontent,
};

/**********************interface statisitc**********************/
struct tb_special_interface {
	char interface_name[XML_MAX_NAME_LEN + 1];
} tb_special_interface_data;

PARSE_XML_STRING(tb_special_interface, interface_name, struct tb_special_interface, IFNAMSIZ)

struct element_node_parameter tb_special_interface_element_parameter[] = {
	HASH_NODE_PARAM_ADD(tb_special_interface, interface_name, TYPE_XML_STRING),
	{"", NULL, NULL, NULL, TYPE_XML_INT}
};

char *xml_tb_special_interface_showall(void *ptr)
{
	xmlNode *cur;
	xmlNode *child;
	char *ifname;
	char * send_buf;
	FILE *fp;
	char buf[1024 + 1];
	int ret;

	struct tb_special_interface *if_node = NULL;

	xmlDoc *global_doc ;

	if_node = (struct tb_special_interface *)ptr;
	ifname = if_node->interface_name;

	/* Open /proc/net/dev. */
	ret = 0;
	int ver =0;
	fp = fopen (_PATH_PROC_NET_DEV, "r");
	if (fp == NULL)
		return NULL;

	/* Drop header lines. */
	fgets (buf, 1024, fp);
	fgets (buf, 1024, fp);
	ver =procnetdev_version(buf);

	child = xml_start_more(&global_doc, "tb_special_interface");

	while (fgets (buf, 1024, fp) != NULL) {
		buf[1024] = '\0';
		char *s, name[IFNAMSIZ];
		s = get_name(name, buf);
		if(strlen(ifname)==0 ||!strcmp(name,ifname)) {
			char buf[50];

			if(strncmp(name, "lo", 2))
				continue;

			cur = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );

			vsos_xmlNewChild( cur, NULL, BAD_CAST"interface_name", BAD_CAST name);
		}
	}

	fclose(fp);
	send_buf = xml_end(global_doc, ZEBRA_SAVE_TMP);
	return send_buf;
}

struct module_node_parameter tb_special_interface_module_parameter = {
	"tb_special_interface",
	NULL,
	NULL,
	NULL,
	xml_tb_special_interface_showall,
	NULL,
	NULL,
	NULL,
	1,
	sizeof(struct tb_special_interface),
};



/**********************tb_interface**********************/
struct tb_interface {
	char vlan_name[XML_MAX_NAME_LEN + 1];
	char type[XML_MAX_NAME_LEN + 1];  /* null, route, etc */
	char vrf_name[XML_MAX_NAME_LEN + 1];
} tb_interface_data;

PARSE_XML_STRING(tb_interface, vlan_name, struct tb_interface, IFNAMSIZ)
PARSE_XML_STRING(tb_interface, type, struct tb_interface, XML_MAX_NAME_LEN)
PARSE_XML_STRING(tb_interface, vrf_name, struct tb_interface, XML_MAX_NAME_LEN)

struct element_node_parameter tb_interface_element_parameter[] = {
	HASH_NODE_PARAM_ADD(tb_interface, vlan_name, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(tb_interface, type, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(tb_interface, vrf_name, TYPE_XML_STRING),
	{"", NULL, NULL, NULL, TYPE_XML_INT}
};

char *xml_tb_interface_showall(void *ptr)
{
	xmlNode *cur;
	xmlNode *child;
	char *ifname;
	char * send_buf;
	FILE *fp;
	char buf[1024 + 1];
	int ret;
	int ifvsysid, vsysid;

	struct tb_interface *if_node = NULL;

	xmlDoc *global_doc ;

    vsysid = GUI_CUR_VSYSID;
	if_node = (struct tb_interface *)ptr;
	ifname = if_node->vlan_name;

	/* Open /proc/net/dev. */
	ret = 0;
	int ver =0;
	fp = fopen (_PATH_PROC_NET_DEV, "r");
	if (fp == NULL)
		return NULL;

	/* Drop header lines. */
	fgets (buf, 1024, fp);
	fgets (buf, 1024, fp);
	ver =procnetdev_version(buf);

	child = xml_start_more(&global_doc, "tb_interface");

	while (fgets (buf, 1024, fp) != NULL) {
		buf[1024] = '\0';
		char alias_name[64];
		unsigned short if_adminflags;

		char *s, name[IFNAMSIZ], vrf_name[XML_MAX_NAME_LEN];
		s = get_name(name, buf);
		if(strlen(ifname)==0 ||!strcmp(name,ifname)) {
			char buf[50];

			if(!strncmp(name, "mgt", 3) || !strncmp(name, "lo", 2) || !strncmp(name, "eha", 3)/* || (strchr(name, '.')!=NULL)*/)
				continue;

			if_adminflags = if_adminflags_get(name);

			if (strcmp(if_node->type, "route") == 0) {
				if ((if_adminflags & IFF_USEDBY_BR)
						|| (if_adminflags & IFF_USEDBY_VLAN)
						|| (if_adminflags & IFF_USEDBY_TRUNK))
					continue;
			} else if (strcmp(if_node->type, "nat") == 0) {
				if ((if_adminflags & IFF_USEDBY_VLAN)
						|| (if_adminflags & IFF_USEDBY_TRUNK))
					continue;
			} else {
				if(!strncmp(name, "gre", 3))
					continue;
				if(!strncmp(name, "tunl", 4))
					continue;
				if(!strncmp(name, "sit", 3))
					continue;
				if(strncmp(name, "vlan", 4)) {
					if(/*(if_adminflags & IFF_USEDBY_BR) 
							|| */(if_adminflags & IFF_USEDBY_VLAN)
							|| (if_adminflags & IFF_USEDBY_TRUNK))
						continue;
				}
			}

#ifdef CONFIG_VSYS
            ifvsysid = dp_netns_get_vrfid_by_ifname(name);
            if (ifvsysid != vsysid) {
                continue;
            }
#endif

			memset(alias_name, 0, 64);
			if_get_alias_by_name(name, alias_name);

			cur = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );

			vsos_xmlNewChild( cur, NULL, BAD_CAST"vlan_name", BAD_CAST (alias_name[0]?alias_name:name));
		}
	}

	fclose(fp);
	send_buf = xml_end(global_doc, ZEBRA_SAVE_TMP);
	return send_buf;
}

struct module_node_parameter tb_interface_module_parameter = {
	"tb_interface",
	NULL,
	NULL,
	NULL,
	xml_tb_interface_showall,
	xml_tb_interface_showall,
	NULL,
	NULL,
	1,
	sizeof(struct tb_interface),
};

#define INTERFACE_LISTEN_TMP "/tmp/.xml_interface_listen"

struct interface_listen_xml interface_listen_data;

/*interface_listen entry*/
PARSE_XML_INTERFACE(interface_listen, name, struct interface_listen_xml, XML_MAX_NAME_LEN)
PARSE_XML_STRING(interface_listen, listen_mode, struct interface_listen_xml, XML_MAX_SHORT_STRING)

struct element_node_parameter interface_listen_element_parameter[] =
{
	HASH_NODE_PARAM_ADD(interface_listen, name, TYPE_XML_INTERFACE),
	HASH_NODE_PARAM_ADD(interface_listen, listen_mode, TYPE_XML_STRING),
	{"", NULL, NULL, NULL, TYPE_XML_INT}
};

unsigned long xml_interface_listen_mod(void *ptr)
{
	int rc;
	struct interface_listen_xml *listen_detail = (struct interface_listen_xml *)ptr;
	int cmd = UIPC_DEV_SET_LISTEN_MODE;

	if (listen_detail->listen_mode[0] == 'd')
		cmd = UIPC_DEV_NO_SET_LISTEN_MODE;

	dataplane_request(cmd, listen_detail->name, IFNAMSIZ, IFNAMSIZ, 0);

	if (cmd == UIPC_DEV_SET_LISTEN_MODE)
		xml_eth_stat_flag_set(listen_detail->name, ETH_CMD_ADMINFLAGS_LISTEN,1);
	else
		xml_eth_stat_flag_set(listen_detail->name, ETH_CMD_ADMINFLAGS_LISTEN,0);

	return 0;
}

char *xml_interface_listen_showall(void *ptr)
{
	xmlNode *cur;
	xmlNode *child;
	char *ifname;
	char * send_buf;
	FILE *fp;
	char buf[1024 + 1];
	int ret;
	int ifvsysid, vsysid;

	struct interface_listen_xml *if_node = NULL;

	xmlDoc *global_doc ;

	if_node = (struct interface_listen_xml *)ptr;
	ifname = if_node->name;

	/* Open /proc/net/dev. */
	ret = 0;
	int ver =0;
	fp = fopen (_PATH_PROC_NET_DEV, "r");
	if (fp == NULL)
		return NULL;

	/* Drop header lines. */
	fgets (buf, 1024, fp);
	fgets (buf, 1024, fp);
	ver =procnetdev_version(buf);

	child = xml_start_more(&global_doc, "interface_listen");

	while (fgets (buf, 1024, fp) != NULL) {
		buf[1024] = '\0';

		char *s, name[IFNAMSIZ];
		s = get_name(name, buf);
		if(strlen(ifname)==0 ||!strcmp(name,ifname)) {
			char buf[50];
			char alias_name[64];
			struct ifdev_info dev_info;

			if((strncmp(name, "xge", 3) && strncmp(name, "ge", 2))
				|| (strchr(name, '.')))
				continue;
			
			vsysid = GUI_CUR_VSYSID;
#ifdef CONFIG_VSYS
			ifvsysid = dp_netns_get_vrfid_by_ifname(name);
			if (ifvsysid != vsysid) {
				continue;
			}
#endif

			memset(&dev_info, 0, sizeof(dev_info));
			strncpy(dev_info.ifname, name, IFNAMSIZ);
			dataplane_request(UIPC_DEV_GET_MODE, (char *)&dev_info,
							  sizeof(dev_info), sizeof(dev_info), 0);

			cur = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );

			memset(alias_name, 0, 64);
			if_get_alias_by_name(name, alias_name);
			vsos_xmlNewChild( cur, NULL, BAD_CAST"name", BAD_CAST (alias_name[0]?alias_name:name));

			vsos_xmlNewChild( cur, NULL, BAD_CAST"listen_mode", 
				BAD_CAST ((dev_info.adminflags & IFF_LISTEN_MODE) ? "enable" : "disable"));
		}
	}

	fclose(fp);
	send_buf = xml_end(global_doc, INTERFACE_LISTEN_TMP);
	return send_buf;
}

int interface_listen_logcontent(unsigned char log, void *ptr, char *Operate)
{
    struct interface_listen_xml* data = (struct interface_listen_xml*)ptr;
    switch(log)
    {
        case WEB_OPERATION_MOD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Mod interface_listen.");

        case WEB_OPERATION_ADD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Add interface_listen.");

        case WEB_OPERATION_DEL:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Del interface_listen.");

        default:
            return 0;
    }
}

struct module_node_parameter interface_listen_module_parameter =
{
	"interface_listen",
	NULL,
	NULL,
	xml_interface_listen_mod,
	xml_interface_listen_showall, 
	NULL, 
	xml_interface_listen_showall,
	NULL,
	0,
	sizeof(struct interface_listen_xml),
	.func_logcontent = interface_listen_logcontent,
};

#ifdef CONGFIG_ADC_SUPPORT
/**********************link_wizard_interface**********************/
struct link_wizard_interface {
	char name[XML_MAX_NAME_LEN + 1];
	char real_name[XML_MAX_NAME_LEN + 1];
	char ip[64];
	char isp[64];
} link_wizard_interface_data;

PARSE_XML_STRING(link_wizard_interface, name, struct link_wizard_interface, IFNAMSIZ)
PARSE_XML_STRING(link_wizard_interface, real_name, struct link_wizard_interface, IFNAMSIZ)
PARSE_XML_IP(link_wizard_interface, ip, struct link_wizard_interface)
PARSE_XML_STRING(link_wizard_interface, isp, struct link_wizard_interface, 64)



struct element_node_parameter link_wizard_interface_element_parameter[] = {
	HASH_NODE_PARAM_ADD(link_wizard_interface, name, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(link_wizard_interface, real_name, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(link_wizard_interface, ip, TYPE_XML_IP),
	HASH_NODE_PARAM_ADD(link_wizard_interface, isp, TYPE_XML_STRING),
	{"", NULL, NULL, NULL, TYPE_XML_INT}
};

int xml_get_isp_by_ip(char *ip_addr, char *isp_buf)
{
#ifdef CONGFIG_ADC_SUPPORT
	struct {
		struct oam_data_st oh;
		char info[128];
	} buf;

	memset(&buf, 0, sizeof buf);
	buf.oh.cmd_code = ISP_LOOKUP;
	buf.oh.mod_id = VTYSH_INDEX_GTM;
	strncpy(buf.info, ip_addr, 128);

	write(client_xml[VTYSH_INDEX_GTM].fd, &buf, sizeof buf);

	while (1) {
		int nbytes;

		nbytes = read (client_xml[VTYSH_INDEX_GTM].fd, &buf, sizeof(buf));

		if (nbytes <= 0 && errno != EINTR) {
			break;
		}
		if (nbytes > 0) {
			if(buf.oh.cmd_code == 2) {
				strncpy(isp_buf,buf.info, 128);
				continue;
			}
			break;
		}
	}

#endif
	return 0;
}

int xml_link_wizard_interface_ipv4_isp_fill(xmlNode *cur, char *if_name)
{
	xmlNode *cur1;
	int ret = 0;
	struct {
		struct oam_data_st oh;
		struct zebra_ip_addr_st data;
	}  buf;

	struct prefix prefix;
	char tmpbuf[32];
	int have_ip = 0;

	memset(&buf, 0, sizeof buf);
	buf.oh.cmd_code = ZEBRA_GET_ADDRESS_ALL;
	buf.oh.mod_id = VTYSH_INDEX_ZEBRA;
	strncpy(buf.data.ifname , if_name, XML_MAX_NAME_LEN);

	write(client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof buf);

	while (1) {
		int nbytes;

		nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof(buf));

		if (nbytes <= 0 && errno != EINTR) {
			break;
		}
		if(buf.oh.cmd_code == 2) {
//			char *tmp_ptr = NULL;
			char isp_buf[128] = {0};
			int isp_num;

			if(have_ip)
				continue;

//			cur1 = vsos_xmlNewChild(cur, NULL, BAD_CAST"group", BAD_CAST NULL);

			memset(&prefix, 0, sizeof(struct prefix));
			memset(tmpbuf, 0, 32);
			memcpy(&prefix, buf.data.addr_str, sizeof(struct prefix));
			prefix2str(&prefix, tmpbuf,32);
//			tmp_ptr = strchr(tmpbuf, '/');
//			*tmp_ptr = '\0';

			xml_get_isp_by_ip(tmpbuf, isp_buf);

			if(!strncmp(isp_buf, "ISP_CMCC", strlen("ISP_CMCC")))
				isp_num = 0;
			else if(!strncmp(isp_buf, "ISP_UNICOM", strlen("ISP_UNICOM")))
				isp_num = 1;
			else if(!strncmp(isp_buf, "ISP_CTT", strlen("ISP_CTT")))
				isp_num = 3;
			else if(!strncmp(isp_buf, "ISP_CT", strlen("ISP_CT")))
				isp_num = 2;
			else if(!strncmp(isp_buf, "ISP_CERNET", strlen("ISP_CERNET")))
				isp_num = 4;
			else
				isp_num = 5;

			vsos_xmlNewChild(cur, NULL, BAD_CAST"ip", BAD_CAST tmpbuf);
			sprintf(tmpbuf, "%d", isp_num);
			vsos_xmlNewChild(cur, NULL, BAD_CAST"isp", BAD_CAST tmpbuf);

			have_ip = 1;

			continue;
		}
		if (nbytes > 0) {
			break;
		}
	}

	if(have_ip==0) {
		vsos_xmlNewChild(cur, NULL, BAD_CAST"ip", BAD_CAST "no ip address");
		vsos_xmlNewChild(cur, NULL, BAD_CAST"isp", BAD_CAST "5");
	}

	return 0;
}

char *xml_link_wizard_interface_showall(void *ptr)
{
	xmlNode *cur;
	xmlNode *child;
	char *ifname;
	char * send_buf;
	FILE *fp;
	char buf[1024 + 1];
	int ret;
	struct interface_info ife;

	struct tb_interface *if_node = NULL;

	xmlDoc *global_doc ;

	if_node = (struct tb_interface *)ptr;
	ifname = if_node->vlan_name;

	/* Open /proc/net/dev. */
	ret = 0;
	int ver =0;
	fp = fopen (_PATH_PROC_NET_DEV, "r");
	if (fp == NULL)
		return NULL;

	/* Drop header lines. */
	fgets (buf, 1024, fp);
	fgets (buf, 1024, fp);
	ver =procnetdev_version(buf);

	child = xml_start_more(&global_doc, "link_wizard_interface");

	while (fgets (buf, 1024, fp) != NULL) {
		buf[1024] = '\0';
		char alias_name[64];

		char *s, name[IFNAMSIZ];
		s = get_name(name, buf);
		if(strlen(ifname)==0 ||!strcmp(name,ifname)) {
			char buf[50];

			if(!strncmp(name, "mgt", 3) || !strncmp(name, "lo", 2) || !strncmp(name, "eha", 3) || (strchr(name, '.')!=NULL))
				continue;

			if(strncmp(name, "vlan", 4)) {
				unsigned short if_adminflags = if_adminflags_get(name);
				if((if_adminflags & IFF_USEDBY_BR) || (if_adminflags & IFF_USEDBY_VLAN) || (if_adminflags & IFF_USEDBY_TRUNK))
					continue;
			}

			strncpy(ife.name,name,IFNAMSIZ);
			if(fetch_interface(&ife)<0)
				continue;

			memset(alias_name, 0, 64);
			if_get_alias_by_name(name, alias_name);

			cur = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );

			vsos_xmlNewChild( cur, NULL, BAD_CAST"name", BAD_CAST (alias_name[0]?alias_name:name));
			vsos_xmlNewChild( cur, NULL, BAD_CAST"real_name", BAD_CAST (name));
			xml_link_wizard_interface_ipv4_isp_fill(cur, name);

			if(ife.flags&IFF_RUNNING) {
				vsos_xmlNewChild( cur, NULL, BAD_CAST"status", BAD_CAST "1");
			} else {
				vsos_xmlNewChild( cur, NULL, BAD_CAST"status", BAD_CAST "0");
			}
		}
	}

	fclose(fp);
	send_buf = xml_end(global_doc, ZEBRA_SAVE_TMP);
	return send_buf;
}

struct module_node_parameter link_wizard_interface_module_parameter = {
	"link_wizard_interface",
	NULL,
	NULL,
	NULL,
	xml_link_wizard_interface_showall,
	xml_link_wizard_interface_showall,
	NULL,
	NULL,
	1,
	sizeof(struct link_wizard_interface),
};


/****************************network wizard******************************/
struct network_wizard_interface {
	char name[XML_MAX_NAME_LEN + 1];
} network_wizard_interface_data;


struct network_wizard {
	struct wizard_interface *wizard_interface;
	struct wizard_route *wizard_route;
} network_wizard_data;


struct wizard_interface {
	char name[XML_MAX_NAME_LEN + 1];
	char real_name[XML_MAX_NAME_LEN + 1];
	char ip[64];
	char isp[64];
	struct wizard_interface *next;
} wizard_interface_data;


struct wizard_route {
	char dst_ip[XML_MAX_IP_LEN + 1];
	int nh_type;
	char nh_ip[XML_MAX_IP_LEN + 1];
	char oif[XML_MAX_NAME_LEN];
	int distance;//<1-255>
	int weigh;//<1-100>
	struct wizard_route *next;
} wizard_route_data;

struct wizard_ip_node {
	char ip[64];
	char name[64];
	struct wizard_ip_node *next;
} *wizard_ip_list = NULL;

struct wizard_route_node {
	char dst_ip[64];
	char nh_ip[64];
	char oif[64];
	int nh_type;
	struct wizard_route_node *next;
} *wizard_route_list = NULL;


PARSE_XML_STRING(network_wizard_interface, name, struct network_wizard_interface, IFNAMSIZ)

struct element_node_parameter network_wizard_interface_element_parameter[] = {
	HASH_NODE_PARAM_ADD(network_wizard_interface, name, TYPE_XML_STRING),
	{"", NULL, NULL, NULL, TYPE_XML_INT}
};


PARSE_XML_STRUCT(network_wizard, wizard_interface, struct network_wizard, struct wizard_interface)
PARSE_XML_STRUCT(network_wizard, wizard_route, struct network_wizard, struct wizard_route)

struct element_node_parameter network_wizard_element_parameter[] = {
	HASH_NODE_PARAM_ADD(network_wizard, wizard_interface, TYPE_XML_STRUCT),
	HASH_NODE_PARAM_ADD(network_wizard, wizard_route, TYPE_XML_STRUCT),
	{"", NULL, NULL, NULL, TYPE_XML_INT}
};


PARSE_XML_STRING(wizard_interface, name, struct wizard_interface, XML_MAX_NAME_LEN)
PARSE_XML_STRING(wizard_interface, real_name, struct wizard_interface, IFNAMSIZ)
PARSE_XML_IPMASK(wizard_interface, ip, struct wizard_interface)
PARSE_XML_STRING(wizard_interface, isp, struct wizard_interface, 64)

struct element_node_parameter wizard_interface_element_parameter[] = {
	HASH_NODE_PARAM_ADD(wizard_interface, name, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(wizard_interface, real_name, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(wizard_interface, ip, TYPE_XML_IPMASK),
	HASH_NODE_PARAM_ADD(wizard_interface, isp, TYPE_XML_STRING),
	{"", NULL, NULL, NULL, TYPE_XML_INT}
};


PARSE_XML_STRING(wizard_route, dst_ip, struct wizard_route, XML_MAX_IP_LEN)
PARSE_XML_INT(wizard_route, nh_type, struct wizard_route)
PARSE_XML_STRING(wizard_route, nh_ip, struct wizard_route, XML_MAX_IP_LEN)
PARSE_XML_INTERFACE(wizard_route, oif, struct wizard_route, XML_MAX_NAME_LEN)
PARSE_XML_INT(wizard_route, distance, struct wizard_route)
PARSE_XML_INT(wizard_route, weigh, struct wizard_route)

struct element_node_parameter wizard_route_element_parameter[] = {
	HASH_NODE_PARAM_ADD(wizard_route, dst_ip, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(wizard_route, nh_type, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(wizard_route, nh_ip, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(wizard_route, oif, TYPE_XML_INTERFACE),
	HASH_NODE_PARAM_ADD(wizard_route, distance, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(wizard_route, weigh, TYPE_XML_INT),
	{"", NULL, NULL, NULL, TYPE_XML_INT}
};


char *xml_network_wizard_interface_show(void *ptr)
{
	xmlNode *cur;
	xmlNode *child;
	char * send_buf;
	FILE *fp;
	char buf[1024 + 1];
	int ret;
	xmlDoc *global_doc ;

	/* Open /proc/net/dev. */
	ret = 0;
	int ver =0;
	fp = fopen (_PATH_PROC_NET_DEV, "r");
	if (fp == NULL)
		return NULL;

	/* Drop header lines. */
	fgets (buf, 1024, fp);
	fgets (buf, 1024, fp);
	ver =procnetdev_version(buf);

	child = xml_start_more(&global_doc, "network_wizard_interface");

	while (fgets (buf, 1024, fp) != NULL) {
		buf[1024] = '\0';

		char *s, name[IFNAMSIZ];
		s = get_name(name, buf);

		char buf[50];

		if(!strncmp(name, "mgt", 3) || !strncmp(name, "lo", 2) || !strncmp(name, "eha", 3) || (strchr(name, '.')!=NULL))
			continue;

		if(!strncmp(name, "tvi", 3))
			continue;

		if(strncmp(name, "vlan", 4)) {
			unsigned short if_adminflags = if_adminflags_get(name);
			if((if_adminflags & IFF_USEDBY_BR) || (if_adminflags & IFF_USEDBY_VLAN) || (if_adminflags & IFF_USEDBY_TRUNK))
				continue;
		} else {
			continue;
		}

		cur = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );

		vsos_xmlNewChild( cur, NULL, BAD_CAST"name", BAD_CAST (name));

	}

	if(wizard_ip_list!=NULL) {
		struct wizard_ip_node **node, *tmp_node;
		node = &wizard_ip_list;
		while(tmp_node = *node) {
			*node = tmp_node->next;
			XFREE(MTYPE_TMP, tmp_node);
		}
		wizard_ip_list = NULL;
	}

	if(wizard_route_list!=NULL) {
		struct wizard_route_node **node, *tmp_node;
		node = &wizard_route_list;
		while(tmp_node = *node) {
			*node = tmp_node->next;
			XFREE(MTYPE_TMP, tmp_node);
		}
		wizard_route_list = NULL;
	}

	fclose(fp);
	send_buf = xml_end(global_doc, ZEBRA_SAVE_TMP);
	return send_buf;
}

struct module_node_parameter network_wizard_interface_module_parameter = {
	"network_wizard_interface",
	NULL,
	NULL,
	NULL,
	xml_network_wizard_interface_show,
	NULL,
	NULL,
	NULL,
	1,
	sizeof(struct network_wizard_interface),
};

int xml_wizard_interface_ip_check(char *ip_addr, char *dev_name)
{
	int ret = 0;
	struct {
		struct oam_data_st oh;
		struct zebra_ip_addr_st data;
	}  buf;

	memset(&buf, 0, sizeof buf);
	buf.oh.cmd_code = ZEBRA_CHECK_ADDRESS;
	buf.oh.mod_id = VTYSH_INDEX_ZEBRA;
	strncpy(buf.data.ifname , dev_name, XML_MAX_NAME_LEN);
	strncpy(buf.data.addr_str , ip_addr, XML_IP_LEN );

	write(client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof buf);

	while (1) {
		int nbytes;

		nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof(buf));

		if (nbytes <= 0 && errno != EINTR) {
			return 0;
		}

		if (nbytes > 0) {
			ret = buf.oh.cmd_code;
			if (ret != 0)
				return  buf.oh.cmd_code;
			break;
		}
	}

	return 0;
}

#if 0
int  ip_make_mask(int logmask)
{
	if (logmask)
		return htonl(~((1<<(32-logmask))-1));
}
int ip_is_subnet_broadcast(int addr ,int masklen)
{
	int mask=ip_make_mask(masklen);
	int bc = addr | ~mask;
	if(bc==addr)
		return 1;
	else
		return 0;
}
#endif
char *xml_wizard_interface_show(void *ptr)
{
	xmlNode *cur;
	xmlNode *child;
	char * send_buf;
	char buf[128] = {0};
	xmlDoc *global_doc ;
	int isp_num;
	int ret, error=0;

	struct wizard_interface *wi = (struct wizard_interface *)ptr;

	child = xml_start_more(&global_doc, "wizard_interface");
	cur = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );

	vsos_xmlNewChild( cur, NULL, BAD_CAST"name", BAD_CAST wi->name);
	vsos_xmlNewChild( cur, NULL, BAD_CAST"real_name", BAD_CAST wi->real_name);
	vsos_xmlNewChild( cur, NULL, BAD_CAST"ip", BAD_CAST wi->ip);

	memset(buf, 0, 128);
	xml_get_isp_by_ip(wi->ip, buf);
	if(!strncmp(buf, "ISP_CMCC", strlen("ISP_CMCC")))
		isp_num = 0;
	else if(!strncmp(buf, "ISP_UNICOM", strlen("ISP_UNICOM")))
		isp_num = 1;
	else if(!strncmp(buf, "ISP_CTT", strlen("ISP_CTT")))
		isp_num = 3;
	else if(!strncmp(buf, "ISP_CT", strlen("ISP_CT")))
		isp_num = 2;
	else if(!strncmp(buf, "ISP_CERNET", strlen("ISP_CERNET")))
		isp_num = 4;
	else
		isp_num = 5;

	memset(buf, 0, 128);
	sprintf(buf, "%d", isp_num);

	vsos_xmlNewChild( cur, NULL, BAD_CAST"isp", BAD_CAST buf);

	memset(buf, 0, 128);
	if_get_alias_by_name(wi->real_name, buf);
	if(strncmp(buf, wi->name, 64)) {
		if(if_check_aliasname(wi->name)) {
			error = 4;
		}
	}

	if(error==0) {
		ret = xml_wizard_interface_ip_check(wi->ip, wi->real_name);
		if(ret == 0) {
			struct prefix_ipv4 cp;
			memset(&cp, 0, sizeof(cp));
			str2prefix_ipv4 (wi->ip, &cp);
			if(ip_is_subnet_broadcast(cp.prefix.s_addr,cp.prefixlen)) {
				error = 3;
			}
		} else if(ret == 35) {
			error = 1;
		} else {
			error = 3;
		}
	}

	if(error == 0) {
		struct prefix prefix1, prefix2;
		struct wizard_ip_node **node, *tmp_node;
		node = &wizard_ip_list;
		str2prefix(wi->ip, &prefix1);
		while(tmp_node = *node) {
			str2prefix(tmp_node->ip, &prefix2);
			if(prefix_same_net(&prefix1, &prefix2)) {
				error = 1;
				break;
			}
			if(!strncmp(tmp_node->name, wi->name, 64)) {
				error = 4;
				break;
			}
			node = &(tmp_node->next);
		}
		if(error == 0) {
			tmp_node = XMALLOC(MTYPE_TMP, sizeof(struct wizard_ip_node));
			memset(tmp_node, 0, sizeof(struct wizard_ip_node));
			strncpy(tmp_node->ip, wi->ip, 64);
			strncpy(tmp_node->name, wi->name, 64);
			tmp_node->next = wizard_ip_list;
			wizard_ip_list = tmp_node;
		}
	}

	memset(buf, 0, 128);
	sprintf(buf, "%d", error);
	vsos_xmlNewChild( cur, NULL, BAD_CAST"error", BAD_CAST buf);

	send_buf = xml_end(global_doc, ZEBRA_SAVE_TMP);
	return send_buf;
}

unsigned long xml_wizard_interface_del(void *ptr)
{
	struct wizard_interface *wi = (struct wizard_interface *)ptr;
	struct wizard_ip_node **node, *tmp_node;
	if(wizard_ip_list) {
		node = &wizard_ip_list;
		while(tmp_node = *node) {
			if(!strncmp(tmp_node->ip, wi->ip, 64)) {
				*node = tmp_node->next;
				XFREE(MTYPE_TMP, tmp_node);
				break;
			}
			node = &(tmp_node->next);
		}
	}

	return 0;
}



int wizard_interface_logcontent(unsigned char log, void *ptr, char *Operate)
{
    struct wizard_interface* data = (struct wizard_interface*)ptr;
    switch(log)
    {
        case WEB_OPERATION_MOD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Mod wizard_interface.");

        case WEB_OPERATION_ADD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Add wizard_interface.");

        case WEB_OPERATION_DEL:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Del wizard_interface.");

        default:
            return 0;
    }
}

struct module_node_parameter wizard_interface_module_parameter = {
	"wizard_interface",
	NULL,
	xml_wizard_interface_del,
	NULL,
	xml_wizard_interface_show,
	NULL,
	NULL,
	NULL,
	1,
	sizeof(struct wizard_interface),
	.func_logcontent = wizard_interface_logcontent,
};



unsigned long xml_wizard_route_del(void *ptr)
{
	struct wizard_route *wr = (struct wizard_route *)ptr;
	struct wizard_route_node **node, *tmp_node;
	if(wizard_route_list) {
		node = &wizard_route_list;
		while(tmp_node = *node) {
			if(tmp_node->nh_type==wr->nh_type) {
				if(tmp_node->nh_type==0) {
					if(!strncmp(tmp_node->nh_ip, wr->nh_ip, 64) && !strncmp(tmp_node->dst_ip, wr->dst_ip, 64)) {
						*node = tmp_node->next;
						XFREE(MTYPE_TMP, tmp_node);
						break;
					}
				} else {
					if(!strncmp(tmp_node->oif, wr->oif, 64) && !strncmp(tmp_node->dst_ip, wr->dst_ip, 64)) {
						*node = tmp_node->next;
						XFREE(MTYPE_TMP, tmp_node);
						break;
					}
				}
			}
			node = &(tmp_node->next);
		}
	}

	return 0;
}

char *xml_wizard_route_show(void *ptr)
{
	xmlNode *cur;
	xmlNode *child;
	char * send_buf;
	char buf[64] = {0};
	xmlDoc *global_doc ;
	int ret=0, error = 0;

	struct wizard_route *wr = (struct wizard_route *)ptr;

	child = xml_start_more(&global_doc, "wizard_route");
	cur = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );

	vsos_xmlNewChild( cur, NULL, BAD_CAST"dst_ip", BAD_CAST wr->dst_ip);
	if(wr->nh_type == 0) {
		vsos_xmlNewChild( cur, NULL, BAD_CAST"nh_type", BAD_CAST "0");
		vsos_xmlNewChild( cur, NULL, BAD_CAST"nh_ip", BAD_CAST wr->nh_ip);
	} else {
		vsos_xmlNewChild( cur, NULL, BAD_CAST"nh_type", BAD_CAST "1");
		vsos_xmlNewChild( cur, NULL, BAD_CAST"oif", BAD_CAST wr->oif);
	}

	memset(buf, 0, 64);
	snprintf(buf, 64, "%d", wr->distance);
	vsos_xmlNewChild( cur, NULL, BAD_CAST"distance", BAD_CAST buf);
	memset(buf, 0, 10);
	snprintf(buf, 9, "%d", wr->weigh);
	vsos_xmlNewChild( cur, NULL, BAD_CAST"weigh", BAD_CAST buf);

	struct in_addr addr_tmp;
	ret = inet_aton (wr->nh_ip, &addr_tmp);
	if(addr_tmp.s_addr == 0) {
		error = 2;
	} else if (IPV4_NET127 (ntohl(addr_tmp.s_addr)) || IN_CLASSD (ntohl(addr_tmp.s_addr))) {
		error = 2;
	}

	if(error==0) {
		struct wizard_route_node **node, *tmp_node;
		node = &wizard_route_list;
		while(tmp_node = *node) {
			if(tmp_node->nh_type == wr->nh_type) {
				if(tmp_node->nh_type==0) {
					if(!strncmp(tmp_node->nh_ip, wr->nh_ip, 64) && !strncmp(tmp_node->dst_ip, wr->dst_ip, 64)) {
						error = 1;
						break;
					}
				} else {
					if(!strncmp(tmp_node->oif, wr->oif, 64) && !strncmp(tmp_node->dst_ip, wr->dst_ip, 64)) {
						error = 1;
						break;
					}
				}
			}
			node = &(tmp_node->next);
		}

		if(error == 0) {
			tmp_node = XMALLOC(MTYPE_TMP, sizeof(struct wizard_route_node));
			memset(tmp_node, 0, sizeof(struct wizard_route_node));
			tmp_node->nh_type = wr->nh_type;
			strncpy(tmp_node->dst_ip, wr->dst_ip, 64);
			strncpy(tmp_node->nh_ip, wr->nh_ip, 64);
			strncpy(tmp_node->oif, wr->oif, 64);
			tmp_node->next = wizard_route_list;
			wizard_route_list = tmp_node;
		}
	}

	memset(buf, 0, 64);
	sprintf(buf, "%d", error);
	vsos_xmlNewChild( cur, NULL, BAD_CAST"error", BAD_CAST buf);

	send_buf = xml_end(global_doc, ZEBRA_SAVE_TMP);
	return send_buf;
}

int wizard_route_logcontent(unsigned char log, void *ptr, char *Operate)
{
    struct wizard_route* data = (struct wizard_route*)ptr;
    switch(log)
    {
        case WEB_OPERATION_MOD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Mod wizard_route.");

        case WEB_OPERATION_ADD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Add wizard_route.");

        case WEB_OPERATION_DEL:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Del wizard_route.");

        default:
            return 0;
    }
}

struct module_node_parameter wizard_route_module_parameter = {
	"wizard_route",
	NULL,
	xml_wizard_route_del,
	NULL,
	xml_wizard_route_show,
	NULL,
	NULL,
	NULL,
	1,
	sizeof(struct wizard_route),
	.func_logcontent = wizard_route_logcontent,
};



unsigned long xml_network_wizard_add(void *ptr)
{
	assert (NULL != ptr);
	struct network_wizard *nw_data = ptr;
	struct wizard_interface *nw_interface = nw_data->wizard_interface;
	struct wizard_route *nw_route = nw_data->wizard_route;

	int ret = 0;

	if(nw_interface) {
		while(nw_interface) {
			if_rename(nw_interface->real_name, nw_interface->name);

			struct {
				struct oam_data_st oh;
				struct zebra_ip_addr_st data;
			}  buf;

			memset(&buf, 0, sizeof buf);
			buf.oh.cmd_code = ZEBRA_ADD_ADDRESS;
			buf.oh.mod_id = VTYSH_INDEX_ZEBRA;
			strncpy(buf.data.ifname , nw_interface->real_name, XML_MAX_NAME_LEN);
			strncpy(buf.data.addr_str, nw_interface->ip, XML_IP_LEN );
			buf.data.addr_type = 0;
			buf.data.ha_unitID = 0;

			write(client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof buf);

			while (1) {
				int nbytes;

				nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &buf, sizeof(buf)-1);

				if (nbytes <= 0 && errno != EINTR) {
					return 0;
				}

				if (nbytes > 0) {
					ret = buf.oh.cmd_code;
					break;
				}
			}
			nw_interface = nw_interface->next;
		}
	}

	if(nw_route) {
		while(nw_route) {
			struct {
				struct oam_data_st oh;
				struct zebra_static_route_st data;
			}  cmd;

			memset(&cmd, 0, sizeof cmd);

			cmd.oh.cmd_code = ZEBRA_STATIC_ROUTE;
			cmd.oh.mod_id = 0;
			strcpy(cmd.data.dest_str, nw_route->dst_ip);
			if(nw_route->nh_type == 0)
				strcpy(cmd.data.gate_str, nw_route->nh_ip);
			else if(nw_route->nh_type == 1)
				strcpy(cmd.data.gate_str, nw_route->oif);
			cmd.data.distance = (int)nw_route->distance;
			cmd.data.weight = nw_route->weigh;
			cmd.data.add_cmd = 1;

			write(client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd, sizeof cmd);

			while (1) {
				int nbytes;

				nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd, sizeof(cmd)-1);

				if (nbytes <= 0 && errno != EINTR) {
					return 0;
				}

				if (nbytes > 0) {
					ret = cmd.oh.cmd_code;
					break;
				}
			}

			nw_route = nw_route->next;
		}
	}

	if(wizard_ip_list!=NULL) {
		struct wizard_ip_node **node, *tmp_node;
		node = &wizard_ip_list;
		while(tmp_node = *node) {
			*node = tmp_node->next;
			XFREE(MTYPE_TMP, tmp_node);
		}
		wizard_ip_list = NULL;
	}

	if(wizard_route_list!=NULL) {
		struct wizard_route_node **node, *tmp_node;
		node = &wizard_route_list;
		while(tmp_node = *node) {
			*node = tmp_node->next;
			XFREE(MTYPE_TMP, tmp_node);
		}
		wizard_route_list = NULL;
	}

	return 0;
}


int network_wizard_logcontent(unsigned char log, void *ptr, char *Operate)
{
    struct network_wizard* data = (struct network_wizard*)ptr;
    switch(log)
    {
        case WEB_OPERATION_MOD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Mod network_wizard.");

        case WEB_OPERATION_ADD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Add network_wizard.");

        case WEB_OPERATION_DEL:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Del network_wizard.");

        default:
            return 0;
    }
}

struct module_node_parameter network_wizard_module_parameter = {
	"network_wizard",
	xml_network_wizard_add,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	1,
	sizeof(struct network_wizard),
	.func_logcontent = network_wizard_logcontent,
};
/****************************network wizard******************************/


struct wizard_state {
	int state;
} wizard_state_data;

PARSE_XML_INT(wizard_state, state, struct wizard_state)

struct element_node_parameter wizard_state_element_parameter[] = {
	HASH_NODE_PARAM_ADD(wizard_state, state, TYPE_XML_INT),
	{"", NULL, NULL, NULL, TYPE_XML_INT}
};

int xml_if_has_ip(char *ifname)
{
	int num=0;
	int nbytes;

	struct {
		struct oam_data_st oh;
		struct zebra_ip_addr_st data;
	}  cmd;

	memset(&cmd, 0, sizeof cmd);
	cmd.oh.cmd_code = ZEBRA_GET_ADDRESS_ALL;
	cmd.oh.mod_id = VTYSH_INDEX_ZEBRA;
	strncpy(cmd.data.ifname , ifname, XML_MAX_NAME_LEN);

	write(client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd, sizeof cmd);

	while (1) {
		nbytes = read (client_xml[VTYSH_INDEX_ZEBRA].fd, &cmd, sizeof(cmd));
		if (nbytes <= 0 && errno != EINTR) {
			break;
		}

		if (nbytes > 0) {
			if(cmd.oh.cmd_code == 2) {
				if(cmd.data.addr_type!=2) {
					num++;
				}
				continue;
			}

			break;
		}
	}

	return num;
}

char *xml_wizard_state_show(void *ptr)
{
	xmlNode *cur;
	xmlNode *child;
	char * send_buf;
	char buf[1024 + 1] = {0};
	xmlDoc *global_doc ;
	int ret=0;
	FILE *fp;

	child = xml_start_more(&global_doc, "wizard_state");
	cur = vsos_xmlNewChild( child, NULL, BAD_CAST"group", BAD_CAST NULL );

	/* Open /proc/net/dev. */
	int ver =0;
	fp = fopen (_PATH_PROC_NET_DEV, "r");
	if (fp == NULL)
		return NULL;
	/* Drop header lines. */
	fgets (buf, 1024, fp);
	fgets (buf, 1024, fp);
	ver =procnetdev_version(buf);

	while (fgets (buf, 1024, fp) != NULL) {
		buf[1024] = '\0';

		char *s, name[IFNAMSIZ];
		s = get_name(name, buf);

		char buf[50];

		if(!strncmp(name, "mgt", 3) || !strncmp(name, "lo", 2) || !strncmp(name, "eha", 3) || (strchr(name, '.')!=NULL))
			continue;
		/*
				if(!strncmp(name, "tvi", 3))
					continue;
		*/
		if(strncmp(name, "vlan", 4)) {
			unsigned short if_adminflags = if_adminflags_get(name);
			if((if_adminflags & IFF_USEDBY_BR) || (if_adminflags & IFF_USEDBY_VLAN) || (if_adminflags & IFF_USEDBY_TRUNK))
				continue;
		}

		if(xml_if_has_ip(name)) {
			ret = 1;
		}
	}

	fclose(fp);

	vsos_xmlNewChild( cur, NULL, BAD_CAST"state", BAD_CAST ret==0?"1":"0");

	send_buf = xml_end(global_doc, ZEBRA_SAVE_TMP);
	return send_buf;
}

struct module_node_parameter wizard_state_module_parameter = {
	"wizard_state",
	NULL,
	NULL,
	NULL,
	xml_wizard_state_show,
	NULL,
	NULL,
	NULL,
	1,
	sizeof(struct wizard_state),
};
#endif

PARSE_XML_STRING(ifname_items, ifname, struct xml_if_member, IFNAME_SIZE)

struct element_node_parameter if_member_element_parameter[] =
{
	HASH_NODE_PARAM_ADD(ifname_items, ifname, TYPE_XML_STRING),
	{"", NULL, NULL, NULL, TYPE_XML_INT}
};

PARSE_XML_STRING(if_sync_group, group_name, struct xml_if_gang, XML_MAX_NAME_LEN)
PARSE_XML_INT(if_sync_group, group_status, struct xml_if_gang)
PARSE_XML_STRUCT(if_sync_group, ifname_items, struct xml_if_gang, struct xml_if_member)

struct element_node_parameter if_gang_element_parameter[] =
{
	HASH_NODE_PARAM_ADD(if_sync_group, group_name, TYPE_XML_STRING),
	HASH_NODE_PARAM_ADD(if_sync_group, group_status, TYPE_XML_INT),
	HASH_NODE_PARAM_ADD(if_sync_group, ifname_items, TYPE_XML_STRUCT),
	{"", NULL, NULL, NULL, TYPE_XML_INT}
};

struct xml_if_gang if_gang_cfg;

unsigned long xml_if_gang_add(void *ptr)
{
	int rc, retval = 0;
	int inlen, outlen;
	char emsg_buf[IPCM_EMSG_SIZE];
	struct if_gang_sole_hdr *req, *rsp;
	const struct xml_if_gang *gang_xml = ptr;
	const struct xml_if_member *member_xml;
	int err;

	req = get_prealloc_buf();
	rsp = get_prealloc_outbuf();
	bzero(req, sizeof(*req));
	outlen = sizeof(*rsp);
	strvcpy(req->gang.name, gang_xml->group_name);

	inlen = sizeof(*req);
	req->opt_code = NEW_IF_GANG;
	rc = ipc_send_and_recv(APP_ZEBRA, IPCMSG_ZEBRA, 
		req, inlen, &retval, rsp, &outlen);
	IPC_XML_RET(rc, retval);

	inlen = sizeof(*req) + 
		IFG_MAX_MEMBERS * sizeof(struct if_member_info);
	req->opt_code = IF_GANG_SET_MEMBER;
	for (member_xml = gang_xml->ifname_items; 
		member_xml; member_xml = member_xml->next) {
		if (req->gang.nr_members >= IFG_MAX_MEMBERS)
			break;

		strvcpy(req->gang.if_members[
			req->gang.nr_members].ifname, member_xml->ifname);
		req->gang.nr_members++;
	}

	rc = ipc_send_and_recv(APP_ZEBRA, IPCMSG_ZEBRA, 
		req, inlen, &retval, rsp, &outlen);
	err = IPC_XML_RV(rc, retval);
	if (err) {
		ipc_save_emsg(emsg_buf, sizeof(emsg_buf));
		goto eout;
	}

	return 0;
eout:
	req->opt_code = DEL_IF_GANG;
	ipc_send_and_recv(APP_ZEBRA, IPCMSG_ZEBRA, 
		req, sizeof(*req), &retval, rsp, &outlen);
	ipc_load_emsg(emsg_buf);
	return err;
}

unsigned long xml_if_gang_del(void *ptr)
{
	int rc, retval = 0;
	int outlen;
	struct if_gang_sole_hdr *req, *rsp;
	const struct xml_if_gang *gang_xml = ptr;

	req = get_prealloc_buf();
	rsp = get_prealloc_outbuf();
	bzero(req, sizeof(*req));
	outlen = sizeof(*rsp);
	req->opt_code = DEL_IF_GANG;
	strvcpy(req->gang.name, gang_xml->group_name);

	rc = ipc_send_and_recv(APP_ZEBRA, IPCMSG_ZEBRA, 
		req, sizeof(*req), &retval, rsp, &outlen);
	IPC_XML_RET(rc, retval);

	return 0;
}

unsigned long xml_if_gang_mod(void *ptr)
{
	int rc, retval = 0;
	int inlen, outlen;
	char emsg_buf[IPCM_EMSG_SIZE];
	struct if_gang_sole_hdr *req, *rsp;
	const struct xml_if_gang *gang_xml = ptr;
	const struct xml_if_member *member_xml;

	req = get_prealloc_buf();
	rsp = get_prealloc_outbuf();
	bzero(req, sizeof(*req));
	outlen = sizeof(*rsp);
	strvcpy(req->gang.name, gang_xml->group_name);

	inlen = sizeof(*req) + 
		IFG_MAX_MEMBERS * sizeof(struct if_member_info);
	req->opt_code = IF_GANG_SET_MEMBER;
	for (member_xml = gang_xml->ifname_items; 
		member_xml; member_xml = member_xml->next) {
		if (req->gang.nr_members >= IFG_MAX_MEMBERS)
			break;

		strvcpy(req->gang.if_members[
			req->gang.nr_members].ifname, member_xml->ifname);
		req->gang.nr_members++;
	}

	rc = ipc_send_and_recv(APP_ZEBRA, IPCMSG_ZEBRA, 
		req, inlen, &retval, rsp, &outlen);
	IPC_XML_RET(rc, retval);

	return 0;
}

#define IF_GANG_NUM 20

char *xml_if_gang_showall(void *ptr)
{
	int rc, retval = 0;
	int outlen;
	struct if_gang_hdr *req, *rsp;
	const struct gui_xml_attr *sattr = gui_get_attr(ptr);
	xmlNode *root, *parent, *child, *sub_child;
	xmlDoc *global_doc;
	int i, j, link_up;

	req = get_prealloc_buf();
	rsp = get_prealloc_outbuf();
	bzero(req, sizeof(*req));
	req->opt_code = SHOWA_IF_GANG;
	req->ctx.pos = attr_calc_pos(sattr);
	root = xml_start_more(&global_doc, "if_sync_group");

	for_attr_num(req->ctx.num, sattr, IF_GANG_NUM) {
		outlen = sizeof(*rsp) + 
			req->ctx.num * sizeof(struct if_gang_info);
		rc = ipc_send_and_recv(APP_ZEBRA, IPCMSG_ZEBRA, 
			req, sizeof(*req), &retval, rsp, &outlen);
		if (rc < 0) {
			GUISH_DEBUG("Error: %s\n", ipc_get_emsg());
			break;
		}

		if (!rsp->ctx.num)
			break;

		for (i = 0; i < rsp->ctx.num; i++) {
			link_up = 0;
			for (j = 0; j < rsp->gangs[i].nr_members; j++) {
				if (rsp->gangs[i].if_members[j].link_up)
					link_up++;
			}

			parent = vsos_xmlNewChild(root, NULL, 
				BAD_CAST "group", BAD_CAST NULL);
			vsos_xmlNewChild(parent, NULL, 
				BAD_CAST "group_name", BAD_CAST rsp->gangs[i].name);
			vsos_xmlNewChild(parent, NULL, 
				BAD_CAST "group_status", 
				BAD_CAST utoa_dec(link_up == rsp->gangs[i].nr_members));
			child = vsos_xmlNewChild(parent, NULL, 
				BAD_CAST "ifname_items", BAD_CAST NULL);

			for (j = 0; j < rsp->gangs[i].nr_members; j++) {
				sub_child = vsos_xmlNewChild(child, NULL, 
					BAD_CAST "group", BAD_CAST NULL);
				vsos_xmlNewChild(sub_child, NULL,
					BAD_CAST "ifname", 
					BAD_CAST rsp->gangs[i].if_members[j].ifname);
			}
		}

		req->ctx.pos = rsp->ctx.pos + rsp->ctx.num;
	} end_for_attr_num;

	xml_add_page(root, sattr, (int)rsp->ctx.total);
	return xml_end(global_doc, ZEBRA_SAVE_TMP);
}

char *xml_if_gang_showone(void *ptr)
{
	int rc, retval = 0;
	int outlen;
	struct if_gang_sole_hdr *req, *rsp;
	const struct xml_if_gang *gang_xml = ptr;
	const struct gui_xml_attr *sattr = gui_get_attr(ptr);
	xmlNode *root, *parent, *child, *sub_child;
	xmlDoc *global_doc;
	int i, link_up;

	req = get_prealloc_buf();
	rsp = get_prealloc_outbuf();
	bzero(req, sizeof(*req));
	req->opt_code = SHOWO_IF_GANG;
	strvcpy(req->gang.name, gang_xml->group_name);
	root = xml_start_more(&global_doc, "if_sync_group");

	outlen = sizeof(*rsp);
	rc = ipc_send_and_recv(APP_ZEBRA, IPCMSG_ZEBRA, 
		req, sizeof(*req), &retval, rsp, &outlen);
	if (rc < 0) {
		GUISH_DEBUG("Error: %s\n", ipc_get_emsg());
		goto out;
	}

	link_up = 0;
	for (i = 0; i < rsp->gang.nr_members; i++) {
		if (rsp->gang.if_members[i].link_up)
			link_up++;
	}

	parent = vsos_xmlNewChild(root, NULL, 
		BAD_CAST "group", BAD_CAST NULL);
	vsos_xmlNewChild(parent, NULL, 
		BAD_CAST "group_name", BAD_CAST rsp->gang.name);
	vsos_xmlNewChild(parent, NULL, 
		BAD_CAST "group_status", 
		BAD_CAST utoa_dec(link_up == rsp->gang.nr_members));
	child = vsos_xmlNewChild(parent, NULL, 
		BAD_CAST "ifname_items", BAD_CAST NULL);

	for (i = 0; i < rsp->gang.nr_members; i++) {
		sub_child = vsos_xmlNewChild(child, NULL, 
			BAD_CAST "group", BAD_CAST NULL);
		vsos_xmlNewChild(sub_child, NULL,
			BAD_CAST "ifname", 
			BAD_CAST rsp->gang.if_members[i].ifname);
	}

out:
	return xml_end(global_doc, ZEBRA_SAVE_TMP);
}

int if_sync_group_logcontent(unsigned char log, void *ptr, char *Operate)
{
    struct xml_if_gang* data = (struct xml_if_gang*)ptr;
    switch(log)
    {
        case WEB_OPERATION_MOD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Mod if_sync_group.");

        case WEB_OPERATION_ADD:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Add if_sync_group.");

        case WEB_OPERATION_DEL:
            return snprintf(Operate, WEB_LOG_OPERATION_LEN - 1, "Del if_sync_group.");

        default:
            return 0;
    }
}

struct module_node_parameter if_gang_module_parameter =
{
	"if_sync_group",
	xml_if_gang_add,
	xml_if_gang_del,
	xml_if_gang_mod,
	xml_if_gang_showall,
	NULL,
	xml_if_gang_showone,
	NULL,
	0,
	sizeof(struct xml_if_gang),
	NULL,
	NULL,
	.func_logcontent = if_sync_group_logcontent,
};

static int find_ifm_used(struct xml_if_member ifm_used[], 
	int nr, const char *ifname)
{
	int i;

	for (i = 0; i < nr; i++) {
		if (!strcmp(ifm_used[i].ifname, ifname))
			return i;
	}

	return -1;
}

char *xml_ifm_unused_show(void *ptr)
{
	int rc, retval = 0;
	int outlen;
	struct if_gang_hdr *req, *rsp;
	struct xml_if_member ifm_used[128];
	xmlNode *root, *parent;
	xmlDoc *global_doc;
	int nr, i, j;
	FILE *fp;
	char buf[1024];

	req = get_prealloc_buf();
	rsp = get_prealloc_outbuf();
	bzero(req, sizeof(*req));
	req->opt_code = SHOWA_IF_GANG;
	bzero(ifm_used, sizeof(ifm_used));
	nr = 0;

	for_attr_num(req->ctx.num, &xml_attr, IF_GANG_NUM) {
		outlen = sizeof(*rsp) + 
			req->ctx.num * sizeof(struct if_gang_info);
		rc = ipc_send_and_recv(APP_ZEBRA, IPCMSG_ZEBRA, 
			req, sizeof(*req), &retval, rsp, &outlen);
		if (rc < 0) {
			GUISH_DEBUG("Error: %s\n", ipc_get_emsg());
			break;
		}

		if (!rsp->ctx.num)
			break;

		for (i = 0; i < rsp->ctx.num; i++) {
			for (j = 0; j < rsp->gangs[i].nr_members; j++) {
				if (nr < __ARRAY_SIZE(ifm_used)) {
					strvcpy(ifm_used[nr].ifname, 
						rsp->gangs[i].if_members[j].ifname);
					nr++;
				}
			}
		}

		req->ctx.pos = rsp->ctx.pos + rsp->ctx.num;
	} end_for_attr_num;

	root = xml_start_more(&global_doc, "if_member_unused");

	fp = fopen(_PATH_PROC_NET_DEV, "r");
	if (!fp)
		goto out;

	/* Drop header lines. */
	fgets(buf, 1024, fp);
	fgets(buf, 1024, fp);

	while (fgets(buf, 1024, fp) != NULL) {
		char ifname[IFNAMSIZ];
		buf[1024] = '\0';
		get_name(ifname, buf);

		if (ifg_if_valid(ifname)) {
			if (find_ifm_used(ifm_used, nr, ifname) < 0) {
				parent = vsos_xmlNewChild(root, NULL, 
					BAD_CAST "group", BAD_CAST NULL);
				vsos_xmlNewChild(parent, NULL, 
					BAD_CAST "name", BAD_CAST ifname);
			}
		}
	}

	fclose(fp);
out:
	return xml_end(global_doc, ZEBRA_SAVE_TMP);
}

struct module_node_parameter ifm_unused_module_parameter =
{
	"if_member_unused",
	NULL,
	NULL,
	NULL,
	xml_ifm_unused_show,
	NULL,
	NULL,
	NULL,
	0,
	0,
	NULL,
	NULL,
};

void xml_zebra_init()
{
	/*for interface flow*/
	init_interface_flow();

	/*for interface layout*/
	if_layout_register_init();

	struct module_node *master_module;

	master_module = create_module_node(inf_select_element_parameter,
									   &inf_select_module_parameter, &inf_select_data);
	if (!master_module)
		return ;

	master_module = create_module_node(interface_node_element_parameter,
									   &interface_node_module_parameter, &interface_node_data);
	if (!master_module)
		return ;
	create_sub_element(master_module, "sec_ip", second_ip_element_parameter, sizeof(struct second_ip));

	/***********tb new*************/
	create_sub_element(master_module, "tb_interface_ip", tb_interface_ip_element_parameter, sizeof(struct tb_ip_info));

	master_module = create_module_node(fake_element_parameter,
									   &ptp_if_module_parameter, NULL);
	if (!master_module)
		return ;

	master_module = create_module_node(interface_node_element_parameter,
									   &p2p_if_module_parameter, &interface_node_data);
	if (!master_module)
		return ;
	
	master_module = create_module_node(static_route_element_parameter,
									   &static_route_module_parameter, &static_route_data);
	if (!master_module)
		return ;

	/*??һ??????ָ????ҳ???ϵĸ???Ԫ??*/
	/*?ڶ???????ָ???˸?ҳ??????Щ????*/
	/*????????????ģ??????*/
	master_module = create_module_node(static_mroute_element_parameter,
									   &static_mroute_module_parameter, &static_mroute);
	if (!master_module)
		return ;
	/*??һ??????????ģ????Ϣ*/
	/*?ڶ???????????ģ??????,??????ҳ???ϸ?Ԫ?ص?????*/
	/*??????????????ģ??Ԫ?صĻ??????ɷ???*/
	/*???ĸ??????ǵض????????Ĵ?С*/
	create_sub_element(master_module, "out_interface", static_mroute_sub_element_parameter, sizeof(struct out_interfaces));


	master_module = create_module_node(rib_route_element_parameter,
									   &rib_route_module_parameter, &rib_route_data);
	if (!master_module)
		return ;
	master_module = create_module_node(default_gate_element_parameter,
									   &default_gate_module_parameter, &default_gate_data);
	if (!master_module)
		return ;
	master_module = create_module_node(rename_element_parameter,
									   &rename_module_parameter, &rename_data);
	if (!master_module)
		return ;

	master_module = create_module_node(p2p_if_element_parameter,
									   &p2p_if_module_parameter, &p2p_if_data);
	if (!master_module)
		return ;
	
	master_module = create_module_node(interface_status_element_parameter,
									   &interface_status_module_parameter, &interface_status_data);
	if (!master_module)
		return ;
	master_module = __create_module_node(interface_infomation_element_parameter,
									   &interface_infomation_module_parameter, &interface_infomation_data, CATEG_MONITOR);
	if (!master_module)
		return ;
	master_module = create_module_node(loop_interface_element_parameter,
									   &loopback_interface_module_parameter, &loop_interface_data);
	if (!master_module)
		return ;
	master_module = __create_module_node(interface_layout_element_parameter,
									   &interface_layout_module_parameter, &interface_layout_data, CATEG_MONITOR);
	if (!master_module)
		return ;
	create_sub_element(master_module, "grid_detail", grid_detail_element_parameter, sizeof(struct grid_detail));


	master_module = create_module_node(interface_flow_element_parameter,
									   &interface_flow_module_parameter, &interface_flow_xml_data);
	if (!master_module)
		return ;

	master_module = __create_module_node(interface_statistics_element_parameter,
									   &interface_statistics_module_parameter, &interface_statistics_data, CATEG_STATISTICS);
	if (!master_module)
		return ;

	master_module = create_module_node(tb_special_interface_element_parameter,
									   &tb_special_interface_module_parameter, &tb_special_interface_data);
	if (!master_module)
		return ;

	master_module = create_module_node(tb_interface_element_parameter,
									   &tb_interface_module_parameter, &tb_interface_data);
	if (!master_module)
		return ;

	master_module = create_module_node(interface_listen_element_parameter, 
									   &interface_listen_module_parameter, &interface_listen_data);
	if (!master_module)
		return;

#ifdef CONGFIG_ADC_SUPPORT
	/***********link wizard interface*************/
	master_module = create_module_node(link_wizard_interface_element_parameter,
									   &link_wizard_interface_module_parameter, &link_wizard_interface_data);
	if (!master_module)
		return ;

	/***********network wizard*************/
	master_module = create_module_node(network_wizard_interface_element_parameter,
									   &network_wizard_interface_module_parameter, &network_wizard_interface_data);
	if (!master_module)
		return ;

	master_module = create_module_node(wizard_interface_element_parameter,
									   &wizard_interface_module_parameter, &wizard_interface_data);
	if (!master_module)
		return ;

	master_module = create_module_node(wizard_route_element_parameter,
									   &wizard_route_module_parameter, &wizard_route_data);
	if (!master_module)
		return ;

	master_module = create_module_node(wizard_state_element_parameter,
									   &wizard_state_module_parameter, &wizard_state_data);
	if (!master_module)
		return ;

	master_module = create_module_node(network_wizard_element_parameter,
									   &network_wizard_module_parameter, &network_wizard_data);
	if (!master_module)
		return ;
	create_sub_element(master_module, "wizard_interface", wizard_interface_element_parameter, sizeof(struct wizard_interface));
	create_sub_element(master_module, "wizard_route", wizard_route_element_parameter, sizeof(struct wizard_route));
#endif

	master_module = create_module_node(if_gang_element_parameter,
		&if_gang_module_parameter, &if_gang_cfg);
	if (!master_module)
		return ;
	create_sub_element(master_module, "ifname_items",
		if_member_element_parameter, sizeof(struct xml_if_member));

	master_module = create_module_node(fake_element_parameter, 
		&ifm_unused_module_parameter, NULL);
	if (!master_module)
		return ;

	return ;
}

