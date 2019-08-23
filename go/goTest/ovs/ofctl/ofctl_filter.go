package ofctl

import (
	"fmt"
	"strings"
	"net"
	"math/big"
)

const (
	// Common Actions.
	CommonActModEtherSrc CommonActionType = iota
	CommonActModEtherDst                 
	CommonActModIPSrc                    
	CommonActModIPDst  
	CommonActModL4Src                    
	CommonActModL4Dst  
	CommonActStripVlan
	CommonActPushVlan
	CommonActOutput  

	//Protocol enum
	ProtoNone ProtocolType = iota
	ProtoIp 
	ProtoArp
	ProtoIcmp

	//ovs Register filed 
	NXM_OF_ETH_DST	RegFieldType = iota
    NXM_OF_ETH_SRC
    NXM_OF_IP_SRC
    NXM_OF_IP_DST
	NXM_OF_IP_TTL
    NXM_OF_ICMP_TYPE
    NXM_OF_ICMP_CODE
    NXM_OF_ARP_OP
    NXM_OF_ARP_SPA
    NXM_OF_ARP_TPA
    NXM_OF_ARP_SHA
    NXM_OF_ARP_THA
	NXM_NX_TUN_IPV4_DST	
	NXM_NX_TUN_IPV4_SRC	

	//MatchAction Type
	RegActionMove RegActionType = iota
	RegActionLoad 
	RegActionPush
	RegActionPop

	MatchFieldInport MatchFieldType = iota
	MatchFieldEthSrc 
	MatchFieldEthDst 
	MatchFieldProto
	MatchFieldVlan
	MatchFieldIPSrc
	MatchFieldIPDst
	MatchFieldVni
	MatchFieldArpTha
	MatchFieldArpSha
	MatchFieldArpOp
	MatchFieldArpTpa
	MatchFieldArpSpa
	MatchFieldIcmpType
	MatchFieldIcmpCode
	
)

type CommonActionType int
type ProtocolType int
type RegFieldType int
type RegActionType int
type MatchFieldType int

var MapProto map[ProtocolType]string = map[ProtocolType]string {ProtoIp:"ip", ProtoArp:"arp", ProtoIcmp:"icmp"}
var MapRegField map[RegFieldType]string = map[RegFieldType]string {
	NXM_OF_ETH_DST:"NXM_OF_ETH_DST[]",
	NXM_OF_ETH_SRC:"NXM_OF_ETH_SRC[]",
	NXM_OF_IP_SRC:"NXM_OF_IP_SRC[]",
	NXM_OF_IP_DST:"NXM_OF_IP_DST[]",
	NXM_OF_IP_TTL:"NXM_OF_IP_TTL[]",
	NXM_OF_ARP_OP:"NXM_OF_ARP_OP[]",
	NXM_OF_ICMP_TYPE:"NXM_OF_ICMP_TYPE[]",
	NXM_OF_ICMP_CODE:"NXM_OF_ICMP_CODE[]",
	NXM_OF_ARP_SPA:"NXM_OF_ARP_SPA[]",
	NXM_OF_ARP_TPA:"NXM_OF_ARP_TPA[]",
	NXM_OF_ARP_SHA:"NXM_OF_ARP_SHA[]",
	NXM_OF_ARP_THA:"NXM_OF_ARP_THA[]",
	NXM_NX_TUN_IPV4_DST:"NXM_NX_TUN_IPV4_DST[]",
	NXM_NX_TUN_IPV4_SRC:"NXM_NX_TUN_IPV4_SRC[]",
}
var MapRegAction map[RegActionType]string = map[RegActionType]string {
	RegActionMove:"move:", 
	RegActionLoad:"load:", 
	RegActionPush:"push:", 
	RegActionPop:"pop:",
}

var MapCommonAction map[CommonActionType]string = map[CommonActionType]string {
	CommonActModEtherSrc:"mod_dl_src:",
	CommonActModEtherDst:"mod_dl_dst:",
	CommonActModIPSrc:"mod_nw_src:",
	CommonActModIPDst:"mod_nw_dst:",
	CommonActModL4Src:"mod_tp_src:",
	CommonActModL4Dst:"mod_tp_dst:",
	CommonActStripVlan:"strip_vlan",
	CommonActPushVlan:"mod_vlan_vid:",
	CommonActOutput:"output:", 
}

var MapMatchField map[MatchFieldType]string = map[MatchFieldType]string {
	MatchFieldInport:"in_port=",
	MatchFieldEthSrc:"dl_src=",
	MatchFieldEthDst:"dl_dst=",
	MatchFieldVlan:"dl_vlan=",
	MatchFieldIPSrc:"nw_src=",
	MatchFieldIPDst:"nw_dst=",
	MatchFieldVni:"tun_id=",
	MatchFieldArpSha:"arp_sha=",
	MatchFieldArpTha:"arp_tha=",
	MatchFieldArpOp:"arp_op=",
	MatchFieldArpSpa:"arp_spa=",
	MatchFieldArpTpa:"arp_tpa=",
	MatchFieldIcmpType:"icmp_type=",
	MatchFieldIcmpCode:"icmp_code=",

}

type Action interface {
	Cmd() []string
}

type CommonAction struct {
	ValueS      string
	Action		CommonActionType 
}

func (m *CommonAction) Cmd() []string {
	var ActString = MapCommonAction[m.Action]
	if (ActString == "") {
		fmt.Printf("invalid register action type \n")	
		return nil	
	}
	
	args := []string {
		ActString,
	}
	
	if (m.Action >= CommonActModEtherSrc && m.Action <= CommonActOutput) {
		if (m.ValueS != "") {
			args = append(args, m.ValueS)
		}
	} else {
		fmt.Printf("unknown common action type: %d \n", int(m.Action))
		return nil 
	}

	retString := []string {strings.Join(args,"")}
	return retString 
}

type RegFieldAction struct {
	Type RegActionType
	ActionField []RegFieldType
	ActionValue []string	
}

func (m *RegFieldAction) Cmd() []string {
	var ActString = MapRegAction[m.Type]
	
	if (ActString == "") {
		fmt.Printf("invalid register action type \n")	
		return nil	
	}

	args := []string {
		ActString,
	}

	switch m.Type {
		case RegActionMove:
			args = append(args, "\"", MapRegField[m.ActionField[0]], "->", MapRegField[m.ActionField[1]], "\"")
			break
		case RegActionLoad:
			args = append(args, "\"", m.ActionValue[0], "->", MapRegField[m.ActionField[0]], "\"")
			break
		case RegActionPush:
			args = append(args, "\"", MapRegField[m.ActionField[0]], "\"")
			break
		case RegActionPop:
			args = append(args, "\"", MapRegField[m.ActionField[0]], "\"")
			break	
	}

	retString := []string {strings.Join(args,"")}
	return retString 
}


type EncapVxlanAction struct {
	LocalIP         string
	RemoteIP        string
	DstPort         int
	TunnelID        uint32
}

func (m *EncapVxlanAction) Cmd() []string {
	args := []string {
		"set_tunnel:",	
	}

	// trans ip string to HEX IP address
	Ip := InetAtoN(m.RemoteIP)
	IpHexString := fmt.Sprintf("0x%x", Ip)
	//add tunnelId info
	args[0] += fmt.Sprintf("%d", m.TunnelID)
	
	// use for load action set tunnel dst ip
	LoadAction := RegFieldAction {
		Type: RegActionLoad,
		ActionField: []RegFieldType {
			NXM_NX_TUN_IPV4_DST,
		},			
		ActionValue:[]string {
			IpHexString,
		},
	}
	
	args[0] +="," + LoadAction.Cmd()[0]
	return args 
}

func InetAtoN(ip string) int64 {
	ret := big.NewInt(0)
	ret.SetBytes(net.ParseIP(ip).To4())
	return ret.Int64()
}

type FlowerKey interface {
	Field() []string
}

type CommonMatchKey struct {
	Type MatchFieldType
	ValueS string
	ValueI uint32	
}

func (m *CommonMatchKey) Field() []string {
	var ActString = MapMatchField[m.Type]
	if (ActString == "" && m.Type != MatchFieldProto) {
		fmt.Printf("invalid match field type \n")	
		return nil	
	}

	if (m.ValueS != "") {
		ActString += m.ValueS
	}else {
		ActString += fmt.Sprintf("%d", m.ValueI) 
	}

	args := []string {
		ActString,
	}
		
	return args
}


type FilterAttrs struct {
	Iface    string
	Table    int
	Priority int
	Offload  string
}

type Flower struct {
	FlowerAttrs FilterAttrs
	MatchKeys []FlowerKey
	Actions []Action
}

type Filter interface {
	Key() string
	Attrs() FilterAttrs
	FinalCmd() []string
}

func (m *Flower) Key() string {
	args := []string{
		"table=",	
	}
	args[0] += fmt.Sprintf("%d", m.FlowerAttrs.Table) //del and dump flows not have priority key words		
	// Match Fileds.
	for _, match := range m.MatchKeys {
		args = append(args, match.Field()...)
	}

	return strings.Join(args, ",")
}

func (m *Flower) Attrs() FilterAttrs {
	return m.FlowerAttrs
}

func (m *Flower) FinalCmd() []string {
	matchString := []string {
		"table=", 
	}
	
	matchString[0] += fmt.Sprintf("%d", m.FlowerAttrs.Table) + ",priority=" + fmt.Sprintf("%d", m.FlowerAttrs.Priority) 

	// Match Fileds.
	for _, match := range m.MatchKeys {
		matchString = append(matchString, match.Field()...)
	}
	
	actionString := []string {
	}
	
	// Actions.
	for _, action := range m.Actions {
		actionString = append(actionString, action.Cmd()...)
	}
	
	args := []string {
		strings.Join(matchString, ","),
	}
	action := "action=" + strings.Join(actionString, ",")
	args = append(args, action)
			
	return args
}

