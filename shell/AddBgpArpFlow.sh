#!/bin/sh
CONFIG_FILE="./test.json"
BRIDGE="br0"
UPLINK="Uplink"

OFCTL=/usr/local/bin/ovs-ofctl
FLOW_PRIORITY=1000
LOCALDIR=1
GWDIR=2

# actions 
ActionModVlan="mod_vlan_vid:"
ActionOutput="output:"
ActionStripVlan="strip_vlan"
# matchs
MatchInput="in_port="
MatchArpTpa="arp_tpa="
MatchProtoIp="ip"
MatchProtoArp="arp"

function ExecCmd() {
	cmd1=`echo $1`
	cmd2=`echo $2`
	ret=`$cmd1 "$cmd2" 2>&1`
	if [[ $ret != "" ]] 
	then
		echo -e "\033[31m Cmd:$cmd1 \"$cmd2\" Error return: $ret\033[0m"	
	fi
}

function ExecOutput() {
	cmd1=`echo $1`
	cmd2=`echo $2`
	ret=`$cmd1 "$cmd2" | grep duration`
	echo $ret
}

function IpaddrAdd() {
	ipMask=$1
	isExsit=`ip addr show $BRIDGE | grep $ipMask`
	if [[ $isExsit == "" ]]; then
		ip addr add $ipMask dev $BRIDGE 
	fi
}

function IpaddrDel() {
	ipMask=$1
	isExsit=`ip addr show $BRIDGE | grep $ipMask`
	if [[ $isExsit != "" ]]; then
		ip addr del $ipMask dev $BRIDGE 
	fi
}

function MatchGenerate() {
	VLAN=$1 
	IP=$2 
	DIR=$3
	PROTO=$4

	if [[ $DIR == $LOCALDIR ]]
	then
		if [[ $PROTO == $MatchProtoArp ]]
		then
			matchString=`echo "$PROTO,in_port=$UPLINK,arp_tpa=$IP"`
		else 
			matchString=`echo "$PROTO,in_port=$UPLINK,nw_dst=$IP"`
		fi
	else 
		if [[ $PROTO == $MatchProtoArp ]]
		then
			matchString=`echo "$PROTO,in_port=$BRIDGE,arp_tpa=$IP"`
		else 
			matchString=`echo "$PROTO,in_port=$BRIDGE,nw_dst=$IP"`
		fi
	fi

	echo "$matchString"
}

function ActionGenerate() {
	VLAN=$1 
	IP=$2 
	DIR=$3
	PROTO=$4

	if [[ $DIR == $LOCALDIR ]]
	then
		actionString=`echo "actions=$ActionStripVlan,$ActionOutput$BRIDGE"`
	else 
		actionString=`echo "actions=$ActionModVlan$VLAN,$ActionOutput$UPLINK"`
	fi

	echo "$actionString"
}

function isFlowExsit() {
	PROTO=$1 
	VLAN=$2 
	IP=$3
	DIR=$4
	
	matchString=`MatchGenerate $VLAN $IP $DIR $PROTO`	
	cmdBase=`echo $OFCTL dump-flows $BRIDGE`
	cmdFlow=$matchString 
	
	#echo "$cmdBase $cmdFlow"
	ExecOutput "$cmdBase" "$cmdFlow"		
}

function FlowAdd() {
	PROTO=$1 
	VLAN=$2 
	IP=$3
	DIR=$4

	isExist=`isFlowExsit $PROTO $VLAN $IP $DIR`
	echo "isExist : $isExist"
	if [[ $isExist == "" ]] 
	then
		matchString=`MatchGenerate $VLAN $IP $DIR $PROTO`	
		actionString=`ActionGenerate $VLAN $IP $DIR $PROTO`	
		cmdBase=`echo $OFCTL add-flow $BRIDGE`
		cmdFlow=`echo "$matchString $actionString"`
		echo -e "\033[44;37;5m $cmdBase $cmdFlow\033[0m"
		ExecCmd "$cmdBase" "$cmdFlow"		
	fi
}

function FlowDel() {
	PROTO=$1 
	VLAN=$2 
	IP=$3
	DIR=$4

	isExist=`isFlowExsit $PROTO $VLAN $IP $DIR`
	echo "isExist : $isExist"
	if [[ $isExist == "" ]] 
	then
		matchString=`MatchGenerate $VLAN $IP $DIR $PROTO`	
		cmdBase=`echo $OFCTL del-flows $BRIDGE`
		cmdFlow=`echo "$matchString"`
		echo -e "\033[44;37;5m $cmdBase $cmdFlow\033[0m"
		ExecCmd "$cmdBase" "$cmdFlow"		
	fi
}

function ProcessFlows() {
	OP=$1
	
	Array_len=`cat $CONFIG_FILE | jq '.|length'`
	arr=`cat $CONFIG_FILE | jq '.[]|.vlan'`
	array_vlan=(${arr//" "/})
	arr=`cat $CONFIG_FILE | jq '.[]|.ip'`
	array_ip=(${arr//" "/})
	arr=`cat $CONFIG_FILE | jq '.[]|.gw'`
	array_gw=(${arr//" "/})

	for ((i=0; i<$Array_len; i++))	
	{
		array_ip[$i]=$(echo ${array_ip[$i]} | tr -d "\"")
		array_gw[$i]=$(echo ${array_gw[$i]} | tr -d "\"")
		ipmask=${array_ip[$i]}
		#echo "----ipmask:$ipmask"
		ipsplitmask=(`echo $ipmask |tr '\/' ' '`)
		echo "index: $i, vlan: ${array_vlan[$i]}, ip:${ipsplitmask[0]}, mask:${ipsplitmask[1]} gw:${array_gw[$i]}"			
		if [[ $OP == "add" ]]
		then
			IpaddrAdd "${array_ip[$i]}"
			FlowAdd "ip" ${array_vlan[$i]} ${ipsplitmask[0]} $LOCALDIR 
			FlowAdd "ip" ${array_vlan[$i]} ${array_gw[$i]} $GWDIR 
			FlowAdd "arp" ${array_vlan[$i]} ${ipsplitmask[0]} $LOCALDIR 
			FlowAdd "arp" ${array_vlan[$i]} ${array_gw[$i]} $GWDIR 
		else 
			IpaddrDel "${array_ip[$i]}"
			FlowDel "ip" ${array_vlan[$i]} ${ipsplitmask[0]} $LOCALDIR 
			FlowDel "ip" ${array_vlan[$i]} ${array_gw[$i]} $GWDIR 
			FlowDel "arp" ${array_vlan[$i]} ${ipsplitmask[0]} $LOCALDIR 
			FlowDel "arp" ${array_vlan[$i]} ${array_gw[$i]} $GWDIR 

		fi
	}
}

if [[ $1 == "delete" ]]
then
ProcessFlows "del"
else 
ProcessFlows "add"
fi

