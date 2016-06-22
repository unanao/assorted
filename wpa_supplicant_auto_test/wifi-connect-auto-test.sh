#!/bin/sh
##################################################
# Description : Wifi connect automatic based on wap_supplicant  
# Maintainer: Jianjiao Sun <jianjiaosun@163.com>
#
# Last Updated: May 22th 2015
#
# Base on Enviroment should be modified based on the environment
# Max : Execute times
# connect info:  
#		ssid and password is input
# 		Others only not default should input 
# WPA_COMMAND and DHCP_COMMAND is based on environment
##################################################

############################## Base on environment############################
MAX=1 #execute times

#example 1 --dateway is not default
ssid_array[0]="WIFI_#16_NETGEAR_5G"
passwd_array[0]="12345678"

#example 2 --default
ssid_array[1]="WIFI_#3_ASUS_2G4"
passwd_array[1]="00000003"

#example 3 -- not deault
ssid_array[2]="WIFI_#16_NETGEAR_5G"
passwd_array[2]="12345678"
gateway_array[2]="192.168.5.1"
netproto_array[2]="RSN"
netgroup_array[2]="CCMP TKIP"
pairwise_array[2]="CCMP"


WPA_COMMAND="/bin/wpa_cli"
#WPA_COMMAND="/3rd/bin/wpa_supplicant/common/wpa_cli"

DHCP_COMMAND="/sbin/udhcpc --timeout=3 --retries=10 -b -i wlan0 -s /usr/share/udhcpc/default.script -p /tmp/udhcpc-wlan0.pid"
###################################################################################

DEF_GATEWAY="192.168.1.1"
DEF_WPA_NET_PROTO="RSN"
DEF_WPA_NET_GROUP="CCMP TKIP"
DEF_WPA_PAIRWISE="CCMP"

DEF_LOG_DIR="./log"
log_dir=$DEF_LOG_DIR
log_dir_suffix=0
while [ -f $log_dir ]
do
	let log_dir_suffix++	
done
log_dir=$DEF_LOG_DIR$log_dir_suffix
mkdir $log_dir

FAIL_FILE=$log_dir/"fail.log"
RESULT_FILE=$log_dir/"result.log"
SUCCESS_FILE=$log_dir/"success.log"

array_size=${#ssid_array[@]}
failed_num=0
cur_num=0;

function get_gateway()
{
	if [ -n $1 ]
	then
		if [ ! -z $1 ]
		then
			echo $1 
			return
		fi
	fi

	echo $DEF_GATEWAY
}

function get_netproto()
{
	if [ -n $1 ]
	then
		if [ ! -z $1 ]
		then
			echo $1 
			return
		fi
	fi

	echo $DEF_WPA_NET_PROTO
}

function get_netgroup()
{
	if [ -n $1 ]
	then
		if [ ! -z $1 ]
		then
			echo $1 
			return
		fi
	fi

	echo $DEF_WPA_NET_GROUP
}

function get_pairwise()
{
	if [ -n $1 ]
	then
		if [ ! -z $1 ]
		then
			echo $1 
			return
		fi
	fi

	echo $DEF_WPA_PAIRWISE
}

for((i=1;i<=$MAX;i++))
do
	for((j=0;j<$array_size;j++))
	do
		conn_ssid=${ssid_array[j]}
		passwd=${passwd_array[j]}

		gate_way=$(get_gateway ${gateway_array[j]})
		net_proto=$(get_netproto ${netproto_array[j]})
		net_group=$(get_netgroup ${netgroup_array[j]})
		pair_wise=$(get_pairwise ${pairwise_array[j]})

		let cur_num++
		echo "connect-wifi-number :" $cur_num "ssid:"$conn_ssid "passwd:"$passwd "gate_way:"$gate_way "net_proto:"$net_proto "net_group:"$net_group "pair wise":$pair_wise


		$WPA_COMMAND -i wlan0 SCAN
		sleep 2

		$WPA_COMMAND -i wlan0 SCAN_RESULTS
		$WPA_COMMAND -i wlan0 DISCONNECT
		$WPA_COMMAND -i wlan0 REMOVE_NETWORK all

		$WPA_COMMAND -i wlan0 ADD_NETWORK
		$WPA_COMMAND -i wlan0 SET_NETWORK 0 auth_alg OPEN

		$WPA_COMMAND -i wlan0 SET_NETWORK 0 key_mgmt WPA-PSK

		echo "set network proto" $net_proto
		$WPA_COMMAND -i wlan0 SET_NETWORK 0 proto $net_proto
		echo "set psk : " $passwd
		$WPA_COMMAND -i wlan0 SET_NETWORK 0 psk \"$passwd\"

		echo "set network pairwise"
		$WPA_COMMAND -i wlan0 SET_NETWORK 0 pairwise $pair_wise

		echo "set network group"
		$WPA_COMMAND -i wlan0 SET_NETWORK 0 group $net_group

		$WPA_COMMAND -i wlan0 SET_NETWORK 0 scan_ssid 1

		echo "set_network ssid : " $conn_ssid
		$WPA_COMMAND -i wlan0 SET_NETWORK 0 ssid \"$conn_ssid\"

		$WPA_COMMAND -i wlan0 ENABLE_NETWORK 0
		$WPA_COMMAND -i wlan0 RECONNECT

		sleep 4
		$DHCP_COMMAND

		sleep 4
		ping -c 1 $gate_way | grep "64 bytes from" 
		if [ $? != 0 ]
		then
			echo "connect-wifi-failed, ssid:"$conn_ssid "execute-number:"$cur_num >> $FAIL_FILE
			let failed_num++
		else
			echo "connect-wifi-success, ssid:"$conn_ssid "execute-number:"$cur_num >> $SUCCESS_FILE
		fi

	done
done

echo "execute-result-failed-number:"$failed_num >> $RESULT_FILE
