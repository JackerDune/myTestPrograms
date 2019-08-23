package ofctl

import (
//	"os/exec"
	"fmt"
	"strings"
)

const (
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
	// add for test 
	BinPath := "/usr/local/ovs-ofctl"
	m := &CmdOfctl{
		OfctlBin: BinPath,
	}

	return m, nil
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
/*
	exist, err := FindDevFilter(filter.Attrs().Iface, filter.NewHandle(), filter.Attrs().Priority)
	if exist == true {
		log.Info("filter already exists in NIC: %s", args)
		return nil
	}
*/
	/*  should run the cmd now */
	fmt.Printf("will run: %s", rule)
/*
	cmd := exec.Command("/bin/bash", "-c", rule)
	if err = cmd.Run(); err != nil {
		log.Error("filter add cmd error: %v, cmd: %s", err, args)
		return err
	}
*/
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

	fmt.Printf("will run: %s", rule)
/*
	cmd := exec.Command("/bin/bash", "-c", rule)
	if err = cmd.Run(); err != nil {
		log.Error("filter add cmd error: %v, cmd: %s", err, args)
		return err
	}
*/

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

	fmt.Printf("will run: %s", rule)
/*
	cmd := exec.Command("/bin/bash", "-c", rule)
	if err = cmd.Run(); err != nil {
		log.Error("filter add cmd error: %v, cmd: %s", err, args)
		return err
	}
*/

	return nil
}
