package ovs

import (
	"os/exec"
	"strings"
	"fmt"	
	"net"	
    "github.com/vishvananda/netlink"
	"bytes"
)

/*ovs-db , dpdk-init config will add to Start.sh 

ovsdb-tool create /usr/local/etc/openvswitch/conf.db /home/openvswitch-2.11.1/vswitchd/vswitch.ovsschema
ovsdb-server --remote=punix:/usr/local/var/run/openvswitch/db.sock \
						--remote=db:Open_vSwitch,Open_vSwitch,manager_options \
						--private-key=db:Open_vSwitch,SSL,private_key \
						--certificate=db:Open_vSwitch,SSL,certificate \
						--bootstrap-ca-cert=db:Open_vSwitch,SSL,ca_cert \
						 --pidfile --detach
														 
ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-init=true
*/

/*
# ovs commands
ovs-ctl status 
ovs-ctl start     
ovs-vsctl add-br br0 -- set bridge br0 datapath_type=netdev
ovs-vsctl add-port br0 vnic0 -- set Interface vnic0 \
    type=dpdk options:dpdk-devargs=0000:05:00.0
vs-vsctl add-port br0 vxlan0 -- set interface vxlan0 type=vxlan options:local_ip=10.97.240.146 options:remote_ip=flow options:key=flow

# add route for vxlan outer ip
ovs-appctl ovs/route/add 10.96.74.19/32 br0 10.96.244.9

# ip addr add 
*/

const (
	Ovsctl = "/usr/local/share/openvswitch/scripts/ovs-ctl"
	OvsVsctl = "/usr/local/bin/ovs-vsctl"
	Ovsappctl = "/usr/local/bin/ovs-appctl"
)

type OvsConfig struct {
	BridgeName        string 
	UplinkPortName    string
	DownlinkPortName  string
	VtepIpAddress     string 
	UplinkIPMask	  string 
	UplinkGw          string  
	DownlinkIPMask    string 
	DownlinkGW        string 
	UplinkPCI         string 
	DownlinkPCI       string  
	OvsDBSockPath     string  
}


func OvsExecCmd(Command string) error {
	cmd := exec.Command("/bin/bash", "-c", Command)
	w := bytes.NewBuffer(nil)
    cmd.Stderr = w	
	fmt.Printf("will run: %s\n", Command)
	if err := cmd.Run(); err != nil {
		fmt.Printf("Exec cmd error: %v, cmd: %s\n", err, Command)
		fmt.Printf("Stderr: %s\n", string(w.Bytes()))	
		return err
	}
	//fmt.Printf("cmd: %s\n", Command)
	return nil	
}


func OvsExecOutput(Command string) ([]byte, error) {
	cmd := exec.Command("/bin/bash", "-c", Command)
	fmt.Printf("will run: %s\n", Command)
	ret, err := cmd.Output()
	if err != nil {
		fmt.Printf("Exec cmd error: %v, cmd: %s\n", err, Command)
		return ret, err
	}
	//fmt.Printf("cmd: %s\n", Command)
	return ret, err	
}


func IsOvsInited() bool{
	args := []string {
		Ovsctl,
		"status",
	//	"|",
	//	"grep",
	//	"not",
	}

	cmdString := strings.Join(args, " ")
	out, error := OvsExecOutput(cmdString)
	if (error == nil) {
		fmt.Printf("Output: %s\n", out)
		return strings.Contains(string(out), "not")			
	}
	
	return false
}

func IsBridgeExsit (BridgeName string) bool {
	args := []string {
		OvsVsctl,
		"list-br",
	}
	
	cmdString := strings.Join(args, " ")
	out, error := OvsExecOutput(cmdString)
	if (error == nil) {
		fmt.Printf("Output: %s\n", out)
		return strings.Contains(string(out), BridgeName)			
	}
	
	return false 
}

func IsPortExsit (BridgeName string, PortName string) bool {
	args := []string {
		OvsVsctl,
		"list-ports",
		BridgeName,
	}

	cmdString := strings.Join(args, " ")
	out, error := OvsExecOutput(cmdString)
	if (error == nil) {
		fmt.Printf("Output: %s\n", out)
		return strings.Contains(string(out), PortName)			
	}
	
	return false 
}


func IsRouteExsit (IPMask string) bool {
	args := []string {
		Ovsappctl,
		"ovs/route/show",
	}

	cmdString := strings.Join(args, " ")
	out, error := OvsExecOutput(cmdString)
	if (error == nil) {
		fmt.Printf("Output: %s\n", out)
		return strings.Contains(string(out), IPMask)			
	}
	
	return false 
}

func OvsInit() error{
	args := []string {
		Ovsctl,
		"start",
	}
	var err error = nil 
		
	ret := IsOvsInited()
	if (ret == false) {
		cmdString := strings.Join(args, " ")
		err = OvsExecCmd(cmdString)
	}

	return err
}

func OvsBridgeAdd(BridgeName string) error {
	args := []string {
		OvsVsctl,
		"add-br",
		BridgeName,
		"--",
		"set",
		"bridge",
		BridgeName,
		"datapath_type=netdev",
	}
	var err error = nil 
	
	ret := IsBridgeExsit(BridgeName)
	if (ret == false) {
		cmdString := strings.Join(args, " ")
		err = OvsExecCmd(cmdString)
	}

	return err
}

func OvsDPDKPortAdd(BridgeName string, PortName string, PCI string) error {
	args := []string {
		OvsVsctl,
		"add-port",
		BridgeName,
		PortName,
		"--",
		"set Interface",
		PortName,
		"type=dpdk",
		"options:dpdk-devargs=" + PCI,
	}
	
	var err error = nil 
	ret := IsPortExsit(BridgeName, PortName)
	if (ret == false) {
		cmdString := strings.Join(args, " ")
		err = OvsExecCmd(cmdString)
	}

	return err
}

func OvsVxlanPortAdd(BridgeName string, PortName string, localIP string) error {
	args := []string {
		OvsVsctl,
		"add-port",
		BridgeName,
		PortName,
		"--",
		"set Interface",
		PortName,
		"type=vxlan",
		"options:local_ip=" + localIP,
		"options:remote_ip=flow",
		"options:key=flow",

	}
	
	var err error = nil 
	ret := IsPortExsit(BridgeName, PortName)
	if (ret != false) {
		cmdString := strings.Join(args, " ")
		err = OvsExecCmd(cmdString)
	}

	return err
}


func OvsRouteAdd(IPMask string, BridgeName string, GW string) error {
	args := []string {
		Ovsappctl,
		"ovs/route/add",
		IPMask,
		BridgeName,
		GW,
	}
	
	var err error = nil 
	ret := IsRouteExsit(IPMask)
	if (ret == false) {
		cmdString := strings.Join(args, " ")
		err = OvsExecCmd(cmdString)
	}

	return err
}

func OvsIpaddrAdd(PortName string, IPMask string) error {
	var IPMaskTemp string
	ifc, _ := net.InterfaceByName(PortName)
	addrs, _ := ifc.Addrs()
		
	for _, addr := range addrs {
		IPMaskTemp = addr.String()
		fmt.Printf("%s %s\n", addr.Network(), addr.String())
		if (strings.Contains(IPMaskTemp, ".") == true) {
			if (strings.Compare(IPMaskTemp, IPMask) == 0) {
				fmt.Printf("IP %s already exsit!\n", IPMask)	
				return nil		
			}
		}
	}
	
    hostdevlink, err := netlink.LinkByName(PortName)
	if err != nil {
		fmt.Printf("Get %s netlink Link info failed!\n", PortName)	
		return err
	}

	ipv4Addr, ipv4Net, err := net.ParseCIDR(IPMask)
	if err != nil {
		fmt.Printf("Parse %s ip address failed!\n", IPMask)	
		return  err
	}

	ipNet := &net.IPNet{
		IP:   ipv4Addr,
		Mask: ipv4Net.Mask,
	}
	address := &netlink.Addr{
		IPNet: ipNet,
		Label: "",
	}

	err = netlink.AddrAdd(hostdevlink, address)
	if err != nil {
		fmt.Printf("AddrAdd error: %v", err)
		return  err
	}
	
	return err
}

func OvsPortMacAddrGet(PortName string) string{
	hostdevlink, err := netlink.LinkByName(PortName)
	if err != nil {
		fmt.Printf("Get %s netlink Link info failed!\n", PortName)	
		return ""
	}
	
	retHwString := hostdevlink.Attrs().HardwareAddr.String()

	fmt.Printf("%s\n", retHwString)
	return retHwString 
}

func OvsPortStatusSet(PortName string, down bool) error {
	hostdevlink, err := netlink.LinkByName(PortName)
	if err != nil {
		fmt.Printf("Get %s netlink Link info failed!\n", PortName)	
		return err
	}
	
	if (down == false)	{
		err = netlink.LinkSetUp(hostdevlink)
		if err != nil {
			fmt.Printf("Get %s netlink Link info failed!\n", PortName)	
			return err
		}
	} else {
		err = netlink.LinkSetDown(hostdevlink)
		if err != nil {
			fmt.Printf("Get %s netlink Link info failed!\n", PortName)	
			return err
		}
	}

	return nil
}
