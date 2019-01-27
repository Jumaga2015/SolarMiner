SolarMiner v1.0 
by Jose Peral 2019-01-27

This is a C program for raspberry Pi to mine cyptocurrencies using solar excedent power.

SolarMiner v1.0 will connect by a USB wifi dongle to the Solax-Boost-X1 inverter 
to monitor the solar excedent available, and will control 2 Sonoff smart relays 
to enable them gradually when there is excess solar power avaible.
When ever the imported power grows avobe the configured threshold of -200W, the
loads/miners will be disengaged gradually.
 
Miners hashrate is monitored thought the Bitmain Antminer API.

Hardware requeriments:
  - Raspberry Pi
  - USB Wifi dongle.
  - x1 Solar PV array with Solax-Boost-X1 inverter with Wifi dongle.
  - x2 Sonoff smart switches.
  - x2 Antminer S9 with Antminer-S9-all-201812051512-autofreq-user-Update2UBI-NF.tar.gz firmware
    .Set Low Power Mode	 ON
    .Low Power Enhaced Mode	 ON

Network requeriments:
  - connect wlan0 to the solax wifi AP.
    . edit /etc/wpa_supplicant/wpa_supplicant.conf file and let only one Wifi config
    .# ping -I wlan0 5.8.8.8
   
  - connect lan0 to the internet using your home router, or wifi repeater.
    .# ping -I eth0 192.168.0.1

Configuration:
  - Please edit config.h file with 
    .Your IFTTT applet name and APIKEYS https://ifttt.com/services/maker_webhooks/settings
    .Your Antminer's S9 fixed IP´s
    .Your Antminer's power usage
 
Compilation instructions
  - execute ./compile
 
Launch instructions
  - execute ./solarminer


To do:
  - Enable or disable the regulation loop based on the sunrise and sunset time +/- a delay.
  - Control more than 2 Sonoff loads. 
  - Enable miners fine regulation modifying mining frequency on the fly.
  - Mining pool API relatime monitoring.
  - Realtime profit calculations.
  - Realtime wallet monitoring. 
 
Questions/Suggestions:  japeralsoler@gmail.com

Telegram: https://t.me/joinchat/AB8sJw6BQtMC47_3JxDN4A

BTC Donations accepted: 3PfsbcTPQvpDybyeZLWm7jbNavcbNoFyH4
 
 
