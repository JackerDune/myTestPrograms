package ofctl

import (
	"os/exec"
	"fmt"
	"strings"
)

const (
	OvsBinPath = "/usr/local/bin/"
	FilterActionAdd  = "add-flow"
	FilterActionDel  = "del-flows"
	FilterActionShow = "dump-flows"
)


type OFCTL interface {
	FilterAdd(filter Filter) error
	FilterDel(filter Filter) error
	FilterDump(filter Filter) error
}

type CmdOfctl struct {
	OfctlBin string
}

func NewCmd() (OFCTL, error) {
/*
	BinPath, err := exec.LookPath("ovs-ofctl")
	if err != nil {
		return nil, fmt.Errorf("failed to find `ofctl`: %s", err)
	}
*/	
	// add for Test 
	 
	m := &CmdOfctl {
		OfctlBin: OvsBinPath + "ovs-ofctl",
	}

	return m, nil
}

func FilterFindExsit(filter Filter) bool {
	/* prepare the cmd */
	args := []string{
		OvsBinPath + "ovs-ofctl",
		FilterActionShow,
		filter.Attrs().Iface,
	}
	
	rule := strings.Join(args, " ")
	rule_match := filter.Key()
	rule = rule + " " + rule_match
	cmd := exec.Command("/bin/bash", "-c", rule)
	out, err := cmd.Output()
	fmt.Printf("err:%s, cmd: %s\n", err, rule)
	if err == nil {
		outstring:= string(out)
		fmt.Printf("Output string is : %s, cmd: %s\n", out, rule)
		return	strings.Contains(outstring, rule_match)
	}

	return false
}

func (m *CmdOfctl) FilterAdd(filter Filter) error {

	/* prepare the cmd */
	args := []string{
		m.OfctlBin,
		FilterActionAdd,
		filter.Attrs().Iface,
	}

	rule := strings.Join(args, " ")
	cmdElements := filter.FinalCmd()
	rule_match_act := strings.Join(cmdElements, ",")
	rule = rule + " " + rule_match_act 
	/* check it ... */
	exist := FilterFindExsit(filter)
	if exist == true {
		fmt.Printf("filter already exists: %s\n", rule)
		return nil
	}
	
	/*  should run the cmd now */
	cmd := exec.Command("/bin/bash", "-c", rule)
	if err := cmd.Run(); err != nil {
		fmt.Printf("filter add cmd error: %v, cmd: %s\n", err, rule)
		return err
	}
	return nil
}

func (m *CmdOfctl) FilterDel(filter Filter) error {

	/* prepare the cmd */
	args := []string{
		m.OfctlBin,
		FilterActionDel,
		filter.Attrs().Iface,
	}
	
	rule := strings.Join(args, " ")
	rule_match := filter.Key()
	rule = rule + " " + rule_match
	cmd := exec.Command("/bin/bash", "-c", rule)
	if err := cmd.Run(); err != nil {
		fmt.Printf("filter del cmd error: %v, cmd: %s", err, rule)
		return err
	}

	return nil
}

func (m *CmdOfctl) FilterDump(filter Filter) error {

	/* prepare the cmd */
	args := []string{
		m.OfctlBin,
		FilterActionShow,
		filter.Attrs().Iface,
	}
	
	rule := strings.Join(args, " ")
	rule_match := filter.Key()
	rule = rule + " " + rule_match
	cmd := exec.Command("/bin/bash", "-c", rule)
	if err := cmd.Run(); err != nil {
		fmt.Printf("filter dump cmd error: %v, cmd: %s", err, rule)
		return err
	}

	return nil
}
