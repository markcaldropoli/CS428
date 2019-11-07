import sys
sys.path.insert(0, '/home/p4/tutorials/utils')
import os
import argparse

from run_exercise import ExerciseRunner

class DemoRunner(ExerciseRunner):
    def __init__(self, topo_file, log_dir, pcap_dir,
                 switch_json, bmv2_exe='simple_switch', quiet=False):
	ExerciseRunner.__init__(self, topo_file, log_dir, pcap_dir,
				switch_json, bmv2_exe, quiet)
	
    def program_hosts(self):
	""" Adds static ARP entries and default routes to each mininet host.

            Assumes:
                - A mininet instance is stored as self.net and self.net.start() has
                  been called.
        """
        for host_name in self.topo.hosts():
            h = self.net.get(host_name)
            h_iface = h.intfs.values()[0]
            link = h_iface.link

            sw_iface = link.intf1 if link.intf1 != h_iface else link.intf2
            # phony IP to lie to the host about
            host_id = int(host_name[1:])
            sw_ip = '10.0.%d.254' % host_id

            # Ensure each host's interface name is unique, or else
            # mininet cannot shutdown gracefully
            h.defaultIntf().rename('%s-eth0' % host_name)
            # static arp entries and default routes
            h.cmd('arp -i %s -s %s %s' % (h_iface.name, sw_ip, sw_iface.mac))
            h.cmd('ethtool --offload %s rx off tx off' % h_iface.name)
            h.cmd('ip route add %s dev %s' % (sw_ip, h_iface.name))
            h.setDefaultRoute("via %s" % sw_ip)

	    # disable offload
	    for off in ["rx", "tx", "sg"]:
                cmd = "/sbin/ethtool --offload eth0 %s off" % off
                print cmd
            	h.cmd(cmd)
	
	    # disable ipv6
	    h.cmd("sysctl -w net.ipv6.conf.all.disable_ipv6=1")
            h.cmd("sysctl -w net.ipv6.conf.default.disable_ipv6=1")
            h.cmd("sysctl -w net.ipv6.conf.lo.disable_ipv6=1")
            h.cmd("sysctl -w net.ipv4.tcp_congestion_control=reno")
            h.cmd("iptables -I OUTPUT -p icmp --icmp-type destination-unreachable -j DROP")

def get_args():
    cwd = os.getcwd()
    default_logs = os.path.join(cwd, 'logs')
    default_pcaps = os.path.join(cwd, 'pcaps')
    parser = argparse.ArgumentParser()
    parser.add_argument('-q', '--quiet', help='Suppress log messages.',
                        action='store_true', required=False, default=False)
    parser.add_argument('-t', '--topo', help='Path to topology json',
                        type=str, required=False, default='./topology.json')
    parser.add_argument('-l', '--log-dir', type=str, required=False, default=default_logs)
    parser.add_argument('-p', '--pcap-dir', type=str, required=False, default=default_pcaps)
    parser.add_argument('-j', '--switch_json', type=str, required=False)
    parser.add_argument('-b', '--behavioral-exe', help='Path to behavioral executable',
                                type=str, required=False, default='simple_switch')
    return parser.parse_args()

if __name__ == '__main__':
    args = get_args()
    demo = DemoRunner(args.topo, args.log_dir, args.pcap_dir,
                      args.switch_json, args.behavioral_exe, args.quiet) 

    demo.run_exercise()
