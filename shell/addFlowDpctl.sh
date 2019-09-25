#!/bin/sh 
INSTALLDIR=/home/dujie/ovs/install
APPCTL=$INSTALLDIR/bin/ovs-appctl
CTL=$INSTALLDIR/share/openvswitch/scripts/ovs-ctl
OFCTL=$INSTALLDIR/bin/ovs-ofctl

VTEPIP="10.97.240.144"
BRMAC="98:03:9b:cb:f7:c4"
DOWNGWMAC="84:d9:31:ee:44:54"
UPGWMAC="84:d9:31:ee:44:54"
BGPLOCALIP="10.97.238.2"
BGPGWIP="10.97.238.1"
UPLINKINDEX=1
BRINDEX=2
DOWNLINKINDEX=3
VTEPINDEX=4
UFIDBASE="12345678-0000-0000-0000-00001234"
UFIDINDEX=1
UFIDEXT= 

# BM infomations
BMIP="10.255.1.11"  
VLAN=11
VNI="0x22b8"
HOSTID="10.96.74.19"
VMIP="10.10.10.10"
GWMAC="6c:92:bf:5e:7f:1a"
VMMAC="00:00:01:01:01:03"

function ExecCmd() {
	cmd1=`echo $1`
	cmd2=`echo $2`
	cmd3=`echo $3`
	ret=`$cmd1 "$cmd2" "$cmd3" 2>&1`
	echo "$cmd1 \"$cmd2\" \"$cmd3\""
	if [[ $ret != "" ]]
		then
		echo -e "\033[31m Cmd:$cmd1 \"$cmd2\" \"$cmd3\" Error return: $ret\033[0m"
	fi
}

function AddUFIDExt() {
	let UFIDINDEX++ 
	UFIDEXT=$(printf %04x $UFIDINDEX)
}

function AddBgpLocalFlow() {
	LOCALIP=$1
	GWIP=$2 

	UFIDEXT=$(printf %04x $UFIDINDEX)
	UFID=$UFIDBASE$UFIDEXT
	cmd1="$APPCTL dpctl/add-flow"
	cmd2="ufid:$UFID,in_port($DOWNLINKINDEX),eth(),eth_type(0x0806),arp(tip=$LOCALIP,op=1)"
	cmd3="$BRINDEX"
	ExecCmd "$cmd1" "$cmd2" "$cmd3"
	
	AddUFIDExt	
	UFID=$UFIDBASE$UFIDEXT
	cmd2="ufid:$UFID,in_port($BRINDEX),eth(),eth_type(0x0806),arp(tip=$GWIP,op=1)"
	cmd3="$DOWNLINKINDEX"
	ExecCmd "$cmd1" "$cmd2" "$cmd3"

	AddUFIDExt	
	UFID=$UFIDBASE$UFIDEXT
	cmd2="ufid:$UFID,in_port($BRINDEX),eth(),eth_type(0x0800),ipv4(dst=$GWIP)"
	cmd3="$DOWNLINKINDEX"
	ExecCmd "$cmd1" "$cmd2" "$cmd3"
	
	AddUFIDExt	
	UFID=$UFIDBASE$UFIDEXT
        cmd2="ufid:$UFID,in_port($DOWNLINKINDEX),eth(),eth_type(0x0800),ipv4(dst=$LOCALIP)"
	cmd3="$BRINDEX"
	ExecCmd "$cmd1" "$cmd2" "$cmd3"
	
	AddUFIDExt	
	UFID=$UFIDBASE$UFIDEXT
	cmd2="ufid:$UFID,in_port($DOWNLINKINDEX),eth(),eth_type(0x0806),arp()"
	cmd3="$BRINDEX"
	ExecCmd "$cmd1" "$cmd2" "$cmd3"
}

function AddBgpLocalFlowUplink() {
	VLAN=$1	
	LOCALIP=$2
	GWIP=$3

	UFIDEXT=$(printf %04x $UFIDINDEX)
	UFID=$UFIDBASE$UFIDEXT
	cmd1="$APPCTL dpctl/add-flow"
	cmd2="ufid:$UFID,in_port($UPLINKINDEX),eth(),eth_type(0x8100),vlan(vid=$VLAN,pcp=0),encap(eth_type(0x0806),arp(tip=$LOCALIP,op=1))"
	cmd3="pop_vlan,$BRINDEX"
	ExecCmd "$cmd1" "$cmd2" "$cmd3"
	
	AddUFIDExt	
	UFID=$UFIDBASE$UFIDEXT
	cmd2="ufid:$UFID,in_port($BRINDEX),eth(),eth_type(0x0806),arp(tip=$GWIP,op=1)"
	cmd3="push_vlan(vid=$VLAN,pcp=0),$UPLINKINDEX"
	ExecCmd "$cmd1" "$cmd2" "$cmd3"

	AddUFIDExt	
	UFID=$UFIDBASE$UFIDEXT
	cmd2="ufid:$UFID,in_port($BRINDEX),eth(),eth_type(0x0800),ipv4(dst=$GWIP)"
	cmd3="push_vlan(vid=$VLAN,pcp=0),$UPLINKINDEX"
	ExecCmd "$cmd1" "$cmd2" "$cmd3"
	
	AddUFIDExt	
	UFID=$UFIDBASE$UFIDEXT
        cmd2="ufid:$UFID,in_port($UPLINKINDEX),eth(),eth_type(0x8100),vlan(vid=$VLAN,pcp=0),encap(eth_type(0x0800),ipv4(dst=$LOCALIP))"
	cmd3="pop_vlan,$BRINDEX"
	ExecCmd "$cmd1" "$cmd2" "$cmd3"
	
	AddUFIDExt	
	UFID=$UFIDBASE$UFIDEXT
	cmd2="ufid:$UFID,in_port($UPLINKINDEX),eth(),eth_type(0x8100),vlan(vid=$VLAN,pcp=0),encap(eth_type(0x0806),arp())"
	cmd3="pop_vlan,$BRINDEX"
	ExecCmd "$cmd1" "$cmd2" "$cmd3"
}

function restartOvs() {
	$CTL stop
	$CTL start 
	$OFCTL del-flows br0 
}

function AddVxlanToVlanFlow() {
	VNI=$1 
	BMIP=$2
	GWMAC=$3
	VLAN=$4

	cmd1="$APPCTL dpctl/add-flow"

	AddUFIDExt	
	UFID=$UFIDBASE$UFIDEXT
	cmd2="ufid:$UFID,in_port($DOWNLINKINDEX),eth(),eth_type(0x0800),ipv4(dst=$VTEPIP,proto=17,tos=0),udp(dst=4789)"
	cmd3="tnl_pop($VTEPINDEX)"
	ExecCmd "$cmd1" "$cmd2" "$cmd3"
	
	AddUFIDExt	
	UFID=$UFIDBASE$UFIDEXT
        cmd2="ufid:$UFID,tunnel(tun_id=$VNI,dst=$VTEPIP),in_port(4),eth(),eth_type(0x0800),ipv4(dst=$BMIP)"
	cmd3="set(eth(dst=$GWMAC)),push_vlan(vid=$VLAN,pcp=0),$UPLINKINDEX"
	ExecCmd "$cmd1" "$cmd2" "$cmd3"

}

function AddVlanToVxlanFlow() {
	VLAN=$1
	VNI=$2
	VMMAC=$3
	HOSTID=$4
	VMIP=$5
	
	cmd1="$APPCTL dpctl/add-flow"
	
	AddUFIDExt	
	UFID=$UFIDBASE$UFIDEXT
	cmd2="ufid:$UFID,in_port($UPLINKINDEX),eth(),eth_type(0x8100),vlan(vid=$VLAN,pcp=0),encap(eth_type(0x0800),ipv4(dst=$VMIP))"
	cmd3="pop_vlan,set(eth(dst=$VMMAC)),tnl_push(tnl_port($VTEPINDEX),header(size=50,type=4,eth(dst=$DOWNGWMAC,src=$BRMAC,dl_type=0x0800),ipv4(src=$VTEPIP,dst=$HOSTID,proto=17,tos=0,ttl=64,frag=0x4000),udp(src=0,dst=4789,csum=0x0),vxlan(flags=0x8000000,vni=$VNI)),out_port(0)),$DOWNLINKINDEX"
	ExecCmd "$cmd1" "$cmd2" "$cmd3"
	
}

restartOvs
AddBgpLocalFlow $BGPLOCALIP $BGPGWIP
AddBgpLocalFlowUplink $VLAN "2.2.2.2" "2.2.2.1" 
AddVxlanToVlanFlow $VNI $BMIP $GWMAC $VLAN $VMIP
AddVlanToVxlanFlow $VLAN $VNI $VMMAC $HOSTID $VMIP
