/*
	epuckterminalbt.cpp
	
	(c) 2012 Alexander Förster (alexander at idsia dot ch) - IDSIA/USI/SUPSI
	
	Based on:
	-   epuckuploadbt.cc
		(c) 2007 Stephane Magnenat (stephane at magnenat dot net) - Mobots group - LSRO1 - EPFL
		libbluetooth based uploader for e-puck

	-	epuckupload.c
		Created by Olivier Michel on 12/15/06.
		(inpired from epuckupload perl script by Xavier Raemy and Thomas Lochmatter)
		Copyright 2006 Cyberbotics Ltd. All rights reserved.
		
	-	libbluetooth samples from Internet
*/

#include <cstdlib>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>

const unsigned programVersion = 1;
const char *programName = "epuck bluetooth terminal (using libbluetooth)";
const char *copyrightInfo = "(c) 2012 Alexander Förster, Olivier Michel, Stephane Magnenat";

int connectToEPuck(const bdaddr_t *ba)
{
	// set the connection parameters (who to connect to)
	struct sockaddr_rc addr;
	addr.rc_family = AF_BLUETOOTH;
	addr.rc_channel = (uint8_t) 1;
	addr.rc_bdaddr = *ba;
	//memcpy(addr.rc_bdaddr, ba, sizeof(bdaddr_t));
	
	// allocate a socket
	int rfcommSock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	
	// connect to server
	int status = ::connect(rfcommSock, (struct sockaddr *)&addr, sizeof(addr));
	
	if (status == 0)
	{
		return rfcommSock;
	}
	else
	{
		return -1;
	}
}

void closeConnectionToEPuck(int rfcommSock)
{
	close(rfcommSock);
}	


bool scanBluetoothAndCommunicate(int robotId)
{
	// open device
	int devId = hci_get_route(NULL);
	if (devId < 0)
	{
		std::cerr << "Error, can't get bluetooth adapter ID" << std::endl;
		return false;
	}
	
	// open socket
	int sock = hci_open_dev(devId);
	if (sock < 0)
	{
		std::cerr << "Error,can't open bluetooth adapter" << std::endl;
		return false;
	}

	// query
	std::cout << "Scanning bluetooth:" << std::endl;
	//int length  = 8; /* ~10 seconds */
	int length  = 4; /* ~5 seconds */
	inquiry_info *info = NULL;
	// device id, query length (last 1.28 * length seconds), max devices, lap ??, returned array, flag
	int devicesCount = hci_inquiry(devId, length, 255, NULL, &info, 0);
	if (devicesCount < 0)
	{
		std::cerr << "Error, can't query bluetooth" << std::endl;
		close(sock);
		return false;
	}
	
	// print devices
	int fd = -2;
	for (int i = 0; i < devicesCount; i++)
	{
		char addrString[19];
		char addrFriendlyName[256];
		ba2str(&(info+i)->bdaddr, addrString);
		if (hci_read_remote_name(sock, &(info+i)->bdaddr, 256, addrFriendlyName, 0) < 0)
			strcpy(addrFriendlyName, "[unknown]");
		printf("\t%s %s\n", addrString, addrFriendlyName);
		if (strncmp("e-puck_", addrFriendlyName, 7) == 0)
		{
			int id;
			sscanf(addrFriendlyName + 7, "%d", &id);
			if (sscanf(addrFriendlyName + 7, "%d", &id) && (id == robotId))
			{
				std::cout << "Contacting e-puck " << id << std::endl;
				fd = connectToEPuck(&(info+i)->bdaddr);
				if (fd==-1)
				{
					std::cerr << "Error: e-puck " << robotId << " cannot connect" << std::endl;	
					break;				
				}
				// TODO: read and write here
				closeConnectionToEPuck(fd);
				break;
			}
		}
	}
	
	if (fd==-2)
		std::cerr << "Error: e-puck " << robotId << " not found" << std::endl;
	
	free(info);
	close(sock);
	
	return fd;
}

int main(int argc,char *argv[])
{
	int robot_id=0;
	
	std::cout << programName << " version " << programVersion << "\n" << copyrightInfo << std::endl;
	if (argc!=2)
	{
		std::cerr << "Error, wrong number of arguments, usage:\n" << argv[0] << " ROBOT_ID" << std::endl;
		return 1;
	}
	sscanf(argv[1],"%d",&robot_id);
	if (scanBluetoothAndCommunicate(robot_id)>=0)
	{
		std::cout << "Communicatin successful :-)" << std::endl;
		return 0;
	}
	else
	{
		std::cout << "Communication failure :-(" << std::endl;
		return 3;
	}
}
