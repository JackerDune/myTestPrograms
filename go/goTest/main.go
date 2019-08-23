package main

import (
	. "./ovs"
	. "./ovs/ofctl"
	"fmt"
)


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


	//test filter cmd vlan -> vxlan 
	Filter := &Flower {
		FlowerAttrs: FilterAttrs {
			Iface: "br0",
			Priority:50000,
			Table: 0,	
		},
		MatchKeys:[]FlowerKey{
			&CommonMatchKey{
				Type: MatchFieldInport,
				ValueS:"dpdk0",
			},
			&CommonMatchKey{
				Type: MatchFieldProto,
				ValueS:"ip",
			},
			&CommonMatchKey{
				Type: MatchFieldIPDst,
				ValueS:"8.8.8.8",
			},
			&CommonMatchKey{
				Type: MatchFieldVlan,
				ValueI:99,
			},
		},
		Actions:[]Action {
			&CommonAction{
				Action:CommonActModEtherDst,
				ValueS:"00:00:03:00:09:11",	
			},
			&EncapVxlanAction {
				RemoteIP: "100.64.1.10",
				TunnelID: 100,
			},
			&CommonAction{
				Action:CommonActOutput,
				ValueS:"vxlan",	
			},
		},
	}
	
	Ovsbin,err := NewCmd()
	if (err != nil) { 
		fmt.Printf("error !\n")
	}
	Ovsbin.FilterAdd(Filter)
	fmt.Printf("\n")	
	Ovsbin.FilterDump(Filter)
	fmt.Printf("\n")	
	Ovsbin.FilterDel(Filter)
	fmt.Printf("\n")
	
	//test filter cmd vxlan -> vlan 
	Filter = &Flower {
		FlowerAttrs: FilterAttrs {
			Iface: "br0",
			Priority:50000,
			Table: 0,	
		},
		MatchKeys:[]FlowerKey{
			&CommonMatchKey{
				Type: MatchFieldInport,
				ValueS:"vtep",
			},
			&CommonMatchKey{
				Type: MatchFieldProto,
				ValueS:"ip",
			},
			&CommonMatchKey{
				Type: MatchFieldVni,
				ValueI: 8888,
			},
		},
		Actions:[]Action {
			&CommonAction{
				Action:CommonActPushVlan,
				ValueS:fmt.Sprintf("%d", 1000),	
			},
			&CommonAction{
				Action:CommonActModEtherDst, 
				ValueS:"00:01:02:03:03:05",	
			},
			&CommonAction{
				Action:CommonActOutput,
				ValueS:"dpdk1",	
			},
		},
	}
	
	Ovsbin,err = NewCmd()
	if (err != nil) { 
		fmt.Printf("error !\n")
	}
	Ovsbin.FilterAdd(Filter)
	fmt.Printf("\n")	
	Ovsbin.FilterDump(Filter)
	fmt.Printf("\n")	
	Ovsbin.FilterDel(Filter)
	fmt.Printf("\n")
		
}
