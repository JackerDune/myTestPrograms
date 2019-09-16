package main

import (
	. "./ovs/ofctl"
	. "./ovs"
	"fmt"
)

/* Flower Example
vs-ofctl add-flow br0 priority=60000,table=0,in_port=dpdk1,arp,arp_tpa=2.2.2.2,arp_op=1,actions=move:"NXM_OF_ETH_SRC[]->NXM_OF_ETH_DST[]",mod_dl_src:"98:03:9b:cb:f7:c4",load:"0x02->NXM_OF_ARP_OP[]",move:"NXM_NX_ARP_SHA[]->NXM_NX_ARP_THA[]",load:"0x98039bcbf7c4->NXM_NX_ARP_SHA[]",move:"NXM_OF_ARP_SPA[]->NXM_OF_ARP_TPA[]",load:"0x02020202->NXM_OF_ARP_SPA[]",output:in_port
ovs-ofctl add-flow br0 priority=60000,in_port=dpdk1,ip,nw_src=10.255.1.11,dl_vlan=11,actions=strip_vlan,mod_dl_dst:00:00:01:01:01:03,set_tunnel:8888,load:"0xa604a13->NXM_NX_TUN_IPV4_DST[]",output:vtep
ovs-ofctl add-flow br0 priority=60000,ip,in_port=vtep,tun_id=8888,nw_dst=10.255.1.11,actions=mod_vlan_vid:11,mod_dl_dst:6c:92:bf:5e:7f:1a,output:dpdk1
*/

func TestOVSCmd() {
	OvsInit()
	OvsBridgeAdd("br0")		
	OvsDPDKPortAdd("br0", "Uplink", "0000:05:00.0")		
	OvsVxlanPortAdd("br0", "vxlan0", "172.17.0.1")		
	OvsRouteAdd("192.10.10.10/24", "br0", "1.1.1.1")		
	OvsIpaddrAdd("br0", "1.1.1.1/24")
}

type CommonPort struct {
	Inface		string
	Priority    int	
	VmIp		string 
	Vlan		uint32 
	VmMac		string 
	VmHostIP	string 					
	Vni			uint32
	Table		int	
	PhyHostMac	string
	VlanGwIP	string
}

func ProxyArpRespose(m *CommonPort, Inport string) {
	Filter := &Flower {
		FlowerAttrs: FilterAttrs {
			Iface: m.Inface,
			Priority:m.Priority,
			Table: m.Table,	
		},
		MatchKeys:[]FlowerKey{
			&CommonMatchKey{
				Type: MatchFieldInport,
				ValueS:Inport,
			},
			&CommonMatchKey{
				Type: MatchFieldProto,
				ValueS:"arp",
			},
			&CommonMatchKey{
				Type: MatchFieldVlan,
				ValueS:fmt.Sprintf("%d",m.Vlan),
			},
			&CommonMatchKey{
				Type: MatchFieldArpTpa,
				ValueS:m.VlanGwIP,
			},
			&CommonMatchKey{
				Type: MatchFieldArpOp,
				ValueI: ArpOpRequest,
			},
		},
		Actions:[]Action {
			&RegFieldAction{
				Type: RegActionMove,
				ActionField:[]RegFieldType {
					NXM_OF_ETH_SRC,
					NXM_OF_ETH_DST,
				},	
			},
			&CommonAction{
				Action:CommonActModEtherSrc,
				ValueS:"00:00:00:00:00:99",	
			},
			&RegFieldAction{
				Type: RegActionLoad,
				ActionField: []RegFieldType{
					NXM_OF_ARP_OP,
				},	
				ActionValue:[]string {
					fmt.Sprintf("%d", ArpOpReply),
				},	
			},
			&RegFieldAction{
				Type: RegActionMove,
				ActionField: []RegFieldType{
					NXM_OF_ARP_SHA,
					NXM_OF_ARP_THA,
				},	
			},
			&RegFieldAction{
				Type: RegActionLoad,
				ActionField: []RegFieldType{
					NXM_OF_ARP_SHA,
				},	
				ActionValue:[]string {
					MacStringToHex("00:00:00:00:00:99"),
				},	
			},
			&RegFieldAction{
				Type: RegActionMove,
				ActionField: []RegFieldType{
					NXM_OF_ARP_SPA,
					NXM_OF_ARP_TPA,
				},	
			},
			&RegFieldAction{
				Type: RegActionLoad,
				ActionField: []RegFieldType{
					NXM_OF_ARP_SPA,
				},	
				ActionValue:[]string {
					InetIntToHex(m.VlanGwIP),
				},	
			},
			&CommonAction{
				Action:CommonActOutput,
				ValueS:"in_port",	
			},
		},
	}
	
	Ovsbin,err := NewCmd()
	if (err != nil) { 
		fmt.Printf("error !\n")
	}
	fmt.Printf("---------ProxyArpFlow---------begin\n")
	Ovsbin.FilterAdd(Filter)
//	fmt.Printf("\n")	
//	Ovsbin.FilterDump(Filter)
//	fmt.Printf("\n")	
//	Ovsbin.FilterDel(Filter)
//	fmt.Printf("\n")
	fmt.Printf("---------ProxyArpFlow---------end\n")
	
}

func VlanToVxlan(m *CommonPort, Inport string, Outport string) {
	Filter := &Flower {
		FlowerAttrs: FilterAttrs {
			Iface: m.Inface,
			Priority:m.Priority,
			Table: m.Table,	
		},
		MatchKeys:[]FlowerKey{
			&CommonMatchKey{
				Type: MatchFieldInport,
				ValueS:Inport,
			},
			&CommonMatchKey{
				Type: MatchFieldProto,
				ValueS:"ip",
			},
			&CommonMatchKey{
				Type: MatchFieldIPDst,
				ValueS:m.VmIp,
			},
			&CommonMatchKey{
				Type: MatchFieldVlan,
				ValueI:m.Vlan,
			},
		},
		Actions:[]Action {
			&CommonAction{
				Action:CommonActModEtherDst,
				ValueS:m.VmMac,	
			},
			&EncapVxlanAction {
				RemoteIP: m.VmHostIP,
				TunnelID: m.Vni,
			},
			&CommonAction{
				Action:CommonActOutput,
				ValueS:Outport,	
			},
		},
	}
	
	Ovsbin,err := NewCmd()
	if (err != nil) { 
		fmt.Printf("error !\n")
	}
	fmt.Printf("---------VlanToVxlan---------begin\n")
	Ovsbin.FilterAdd(Filter)
	fmt.Printf("\n")	
	Ovsbin.FilterDump(Filter)
	fmt.Printf("\n")	
	Ovsbin.FilterDel(Filter)
	fmt.Printf("\n")
	fmt.Printf("---------VlanToVxlan---------end\n")
}

func VxlanToVlan(m *CommonPort, Inport string, Outport string) {
	Filter := &Flower {
		FlowerAttrs: FilterAttrs {
			Iface: m.Inface,
			Priority:m.Priority,
			Table: m.Table,	
		},
		MatchKeys:[]FlowerKey{
			&CommonMatchKey{
				Type: MatchFieldInport,
				ValueS: Inport,
			},
			&CommonMatchKey{
				Type: MatchFieldProto,
				ValueS:"ip",
			},
			&CommonMatchKey{
				Type: MatchFieldVni,
				ValueI: m.Vni,
			},
		},
		Actions:[]Action {
			&CommonAction{
				Action:CommonActPushVlan,
				ValueS:fmt.Sprintf("%d", m.Vlan),	
			},
			&CommonAction{
				Action:CommonActModEtherDst, 
				ValueS:m.PhyHostMac,	
			},
			&CommonAction{
				Action:CommonActOutput,
				ValueS:Outport,	
			},
		},
	}
	
	Ovsbin,err := NewCmd()
	if (err != nil) { 
		fmt.Printf("error !\n")
	}
	fmt.Printf("---------VxlanToVlan---------begin\n")
	Ovsbin.FilterAdd(Filter)
//	fmt.Printf("\n")	
//	Ovsbin.FilterDump(Filter)
//	fmt.Printf("\n")	
//	Ovsbin.FilterDel(Filter)
//	fmt.Printf("\n")
	fmt.Printf("---------VxlanToVlan---------end\n")
}

func main() {
	CommonActionTest := CommonAction {
		Action:CommonActPushVlan,
		ValueS:"1000",	
	}

	fmt.Printf("Cmd=%s\n", CommonActionTest.Cmd())
	
	RegFieldActionTest := []RegFieldAction {
		{
			Type:RegActionMove, 
			ActionField:[]RegFieldType{
				NXM_OF_ETH_DST, 
				NXM_OF_ETH_SRC,
			}, 
			ActionValue:[]string{
			},
		},
		{
			Type:RegActionLoad, 
			ActionField:[]RegFieldType{
				NXM_OF_IP_TTL,
			}, 
			ActionValue:[]string{
				"60",
			},
		},
		{
			Type:RegActionPush, 
			ActionField:[]RegFieldType{
				NXM_OF_IP_DST,
			}, 
			ActionValue:[]string{
			},
		},
	}

	//vxlan encap	
	ip :="10.10.1.100"
	ipInt := InetAtoN(ip)
	fmt.Printf("0x%x\n", ipInt)
	fmt.Printf("Cmd=%s, %s, %s\n", RegFieldActionTest[0].Cmd()[0], RegFieldActionTest[1].Cmd()[0], RegFieldActionTest[2].Cmd()[0])

	EncapTest := EncapVxlanAction {
		RemoteIP: "100.64.1.10",
		TunnelID: 100,
	}

	fmt.Printf("Encap Cmd: %s\n", EncapTest.Cmd()[0])

	//match field Test 
	CommonMatchKeyTest := []CommonMatchKey {
		{
			Type: MatchFieldEthDst,
			ValueS:"00:02:03:05:06:08",
		},
		{
			Type: MatchFieldIPDst,
			ValueS:"8.8.8.8",
		},
		{
			Type: MatchFieldVlan,
			ValueI:99,
		},
		{
			Type: MatchFieldProto,
			ValueS:"icmp",
		},
		{
			Type: MatchFieldVni,
			ValueI:145,
		},
	}

	fmt.Printf("len: %d\n", len(CommonMatchKeyTest))
	n :=len(CommonMatchKeyTest)
	for i:=0; i < n; i++ {
		fmt.Printf(" %s ", CommonMatchKeyTest[i].Field()[0])	
	}
	
	fmt.Printf("\n")	

	CommonPortInit := &CommonPort {
		Inface:"br0",
		Priority:60000,
		VmIp: "10.178.1.1", 
		Vlan: 145,
		VmMac: "00:00:55:77:66:99",
		VmHostIP: "10.78.10.66",
		Vni: 5555,
		Table:0,
		PhyHostMac: "00:00:88:99:99:99",
		VlanGwIP: "2.2.2.2",
	}

	VxlanToVlan(CommonPortInit, "dpdk0", "vtep")
	VlanToVxlan(CommonPortInit, "vtep", "dpdk1")
	TestOVSCmd()
	fmt.Printf("port: %s, mac:%s\n", "br0", OvsPortMacAddrGet("br0"))	
	OvsPortStatusSet("br0", true)	
	fmt.Printf("Iphex: %s, Mac: %s\n", InetIntToHex("100.64.9.1"), MacStringToHex("00:88:99:44:66:88"));
	ProxyArpRespose(CommonPortInit, "dpdk0")
}
