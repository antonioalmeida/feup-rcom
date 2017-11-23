#!/bin/bash
/etc/init.d/networking restart
ifconfig eth0 up
ifconfig
ifconfig eth0 172.16.50.254/24
ifconfig eth1 up
ifconfig
ifconfig eth1 172.16.51.253
route add -net 172.16.50.0/24 gw 172.16.50.1
route add -net 172.16.51.0/24 gw 172.16.51.253
route add default gw 172.16.51.254
route -n
echo 1 > /proc/sys/net/ipv4/ip_forward
echo 0 > /proc/sys/net/ipv4/icmp_echo_ignore_broadcasts
