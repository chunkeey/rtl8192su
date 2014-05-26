Release Date: 2010-1207, ver 0019 
RTL8192SE Linux driver
   --This driver supports RealTek rtl8192SE PCI Wireless LAN NIC
     for
     2.6 kernel:
     Fedora Core, Debian, Mandriva, Open SUSE, Gentoo, 
     Ubuntu 7.10/8.04/8.10/9.04/9.10/10.04/10.10, 
     moblin(V2), android-x86_090916, etc.

     2.4 kernel:
     Redhat 9.0/9.1

========================================================================================
				I. Component 
========================================================================================
The driver is composed of several parts:
	1. Firmare to make nic work
           1.1 firmare/RTL8192SE

	2. Module source code
	   2.1 rtllib 
	   2.2 HAL/rtl8192
	
	3. Script to build the modules
	   3.1 Makefile 

	4. Example of supplicant configuration file:
	   4.1 wpa1.conf

	5. Script to run wpa_supplicant
	   5.1 runwpa

========================================================================================
				II. Compile & Installation & uninstall
========================================================================================
You can enter top-level directory of driver and execute follwing command to
Compile, Installation, or uninstall the driver:

        1. Change to Super User
	   sudo su

	2. Compile driver from the source code 
	   make

	3. Install the driver to the kernel
           make install
           reboot

	4. uninstall driver
	   make uninstall

========================================================================================
				III. Start Up Wireless
========================================================================================
You can use two methord to start up wireless:

	1. Install driver like II. and reboot OS, Wireless will be brought 
	   up by GUI, such as NetworkManager

	2. If Wireless is not brought up by GUI, you can use: 
	   ifconfig wlan0 up 

	   Note: some times when you have two wireless NICs on your computer,
		 interface "wlan0" may be changed to "wlan1" or "wlan2", etc. 
		 So before "ifconfig wlan0 up", you can use "iwconfig" to check
		 which interface our NIC is.

	   Note: Don't try to down driver by "ifconfig wlan0 down" when 
		 NetworkManager id opened, because NetworkManager will up
		 driver automatically. 

========================================================================================
				IV. Wireless UI & NetworkManager 
========================================================================================
	1. All latest distributions have UI & NetworkManager to like with AP.
	   And it's more easy to link with AP than commandline,
	   So we suggest you use UI to link with AP.

	2. Don't try to like with AP use commandline with UI or NetworkManager opened.

	3. If you still used commandline to link with AP, Please check if
	   NetworkManager & wpa_supplicant is killed by follwing command:

		ps -x | grep NetworkManager
		ps -x | grep wpa_supplicant

	4. Follwing commandlines(V-VII) are all used under Linux without UI. 

========================================================================================
				V. Set wireless lan MIBs  
========================================================================================
This driver uses Wireless Extension as an interface allowing you to set
Wireless LAN specific parameters.

	1. Current driver supports "iwlist" to show the device status of nic

        	iwlist wlan0 [parameters]

	   you can use follwing parameters:

        	parameter explaination      	[parameters]    
       		-----------------------     	-------------   
        	Show available chan and freq	freq / channel  
        	Show and Scan BSS and IBSS 	scan[ning]          
        	Show supported bit-rate         rate / bit[rate]        

	   For example:
		iwlist wlan0 channel
		iwlist wlan0 scan
		iwlist wlan0 rate

	2. Driver also supports "iwconfig", manipulate driver private ioctls, 
	   to set MIBs.

		iwconfig wlan0 [parameters] [val]

	   you can use follwing parameters:

		parameter explaination      [parameters]        [val] constraints
        	-----------------------     -------------        ------------------
        	Connect to AP by address    ap              	[mac_addr]
        	Set the essid, join (I)BSS  essid             	[essid]
        	Set operation mode          mode                {Managed|Ad-hoc}
        	Set keys and security mode  key/enc[ryption]    {N|open|restricted|off}

	   For example:
		iwconfig wlan0 ap XX:XX:XX:XX:XX:XX
		iwconfig wlan0 essid "ap_name"
		iwconfig wlan0 mode Ad-hoc
		iwconfig wlan0 essid "name" mode Ad-hoc
		iwconfig wlan0 key 0123456789 [2] open
		iwconfig wlan0 key off
		iwconfig wlan0 key restricted [3] 0123456789
        	iwconfig wlan0 key s:12345

	   Note: There are two types of key, "hex" code or "ascii" code. "hex" code
	        only contains hexadecimal characters, "ascii" code is consist of 
                "ascii" characters. Assume the "hex" code key is "0123456789", you 
                are suggested to use command like this "iwconfig wlan0 key 0123456789". 
                Assume the "ascii" code key is "12345", you should enter command 
                like this "iwconfig wlan0 key s:12345".

           Note: Better to set these MIBS without GUI such as NetworkManager and be 
	        sure that our nic has been brought up before these settings. WEP key
	        index 2-4 is not supportted by NetworkManager.

========================================================================================
				VI. Getting IP address (For OS without UI) 
========================================================================================
Before transmit/receive data, you should obtain an IP address use one of
the follwing method: 

	1. static IP address:

		ifconfig wlan0 IP_ADDRESS

	2. dynamic IP address using DHCP:
	   	
		dhclient wlan0
		

========================================================================================
				VII. WPAPSK/WPA2PSK (For OS without UI) 
========================================================================================
Wpa_supplicant helps you to link with WPA/WPA2(include WPA Enterprise) AP, 
in Linux with NetworkManger & UI, UI will help you to link with AP, 
But if there is no UI & Networkmanger in your Linux, you can use 
follwing method to link with WPA/WPA2 AP. 
	
	1. we suppose that your Linux have installed wpa_supplicant & 
	   kernel build with WIRELESS_EXT, In fact, lots of distributions
	   have done like this.

	   But if some distribution not install wpa_supplicant,
	   please download wpa_supplicant from webset and install it.

	2. Edit wpa1.conf to set up SSID and its passphrase.
	   For example, the following setting in "wpa1.conf" 
	   means SSID to join is "BufAG54_Ch6" and its 
	   passphrase is "87654321".

	   network={
			ssid="BufAG54_Ch6"
			#scan_ssid=1 //see note 3
			proto=WPA
			key_mgmt=WPA-PSK
			pairwise=CCMP TKIP
			group=CCMP TKIP WEP104 WEP40
			psk="87654321"
			priority=2
		  }

	   You can download wpa_supplicant and read wpa_supplicant.conf 
	   for more examples.

	3. Execute WPA supplicant:

	   For WEXT interface please use:
		wpa_supplicant -D wext -c wpa1.conf -i wlan0 

	   For IPW interface please use:
           	wpa_supplicant -D ipw -c wpa1.conf -i wlan0  

	   Notice: If the version of Wireless Extension in your system 
		   is equal or larger than 18, WEXT driver interface is 
		   recommended. Otherwise, IPW driver interface is advised.  
		   
		   you can use  "iwconfig -v" To check the version of 
		   wireless extension.

	4. To see detailed description for driver interface and wpa_supplicant, 
	   please type: 

		man wpa_supplicant
