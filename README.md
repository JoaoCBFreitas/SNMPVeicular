# SNMPVeicular

Prototype SNMP agent to be installed inside a vehicle OBU and allows outside entities to poll data from the vehicles' sensors and change status of actuators.  

###### Note: This prototype was made to work with OBU-MIB which is incomplete. Complete MIB is included in file MIBFULL.txt and to use it will require some work to be done to the prototype

## Installation

### Install snmp

- sudo apt-get install snmpd snmp libsnmp-dev snmp-mibs-downloader
- or through net-snmp website

###### For reference snmpd version that was used to run this project was 5.8

## Creating a SNMP user

This agent and manager were created with SNMPv3 in mind, while you can still use SNMPv2c with the manager included in snmpd it is advised to create a SNMPv3 user. This is done by the following command

- sudo net-snmp-create-v3-user -A SNMPVeicular -a SHA -X SNMPV31cul4r -x AES snmpadmin
- Confirm if the user was correctly created by checking if /usr/share/snmp/snmpd.conf contains this line "rwuser snmpadmin"
- If it does not contain that line, add it

## Install OBU-MIB.txt

Find where MIBs are installed in your system, as standard they are located within /home/$USER/.snmp/mibs  
Copy OBU-MIB.txt to that directory

- sudo cp OBU-MIB.txt /home/$USER/.snmp/mibs or /usr/share/snmp/mibs  
  Load MIB into snmpd with the following command
- export MIBS=+OBU-MIB  
  Otherwise, to make it load the MIB on start up simply add mibs +OBU-MIB to the following file $HOME/.snmp/snmp.conf
- The line "mibs:" in /etc/snmp/snmp.conf should also be commented out

## MakeFile

This project already includes a makefile that will build everything that is needed. The files it will create are the following:

- generator
- manager
- agent

## How to use

Prior to running the program it's recommended to run the script start.sh, it will start snmpd and create virtual a CAN interface  
With this done the project can be run with the following commands

- sudo ./agent
- ./generator
- ./manager

In the generator you can choose what file should be used to generate CAN messages while in the manager you can choose what commands to send to the agent and what sensors are to be monitored, optionally you can also achieve this via the terminal with the included snmpd manager.

## Example snmp commands in SNMPv3

### Traverse a table

- snmpwalk -v3 -a SHA -A SNMPVeicular -x AES -X SNMPV31cul4r -l authPriv -u snmpadmin localhost OBU-MIB::{targetTable}

### Using snmptable command

- snmptable -Os -v3 -a SHA -A SNMPVeicular -x AES -X SNMPV31cul4r -l authPriv -u snmpadmin localhost OBU-MIB:{targetTable}

### Changing columns

Changing saving mode of a request

- snmpset -Ir -v3 -a SHA -A SNMPVeicular -x AES -X SNMPV31cul4r -l authPriv -u snmpadmin localhost OBU-MIB::savingMode.0 = 0

Deleting a request

- snmpset -Ir -v3 -a SHA -A SNMPVeicular -x AES -X SNMPV31cul4r -l authPriv -u snmpadmin localhost OBU-MIB::status.0 = 3

### Adding a new command to commandTable

- snmpset -Ir -v3 -a SHA -A SNMPVeicular -x AES -X SNMPV31cul4r -l authPriv -u snmpadmin localhost OBU-MIB::templateID.1 = 1 OBU-MIB::commandInput.1 = 2 OBU-MIB::commandUser.1 s "Utilizador teste"

### Adding a new request to requestMonitoringDataTable

- snmpset -Ir -v3 -a SHA -A SNMPVeicular -x AES -X SNMPV31cul4r -l authPriv -u snmpadmin localhost OBU-MIB::requestMapID.0 = 199 OBU-MIB::requestStatisticsID.0 = 1 OBU-MIB::savingMode.0 = 0 OBU-MIB::waitTime.0 s "00:00:00" OBU-MIB::durationTime.0 s "01:00:00" OBU-MIB::expireTime.0 s "02:00:00" OBU-MIB::maxNOfSamples.0 = 50 OBU-MIB::loopMode.0 = 2 OBU-MIB::requestUser.0 s "Utilizador teste"
