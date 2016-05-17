/*
Copyright (c) 2010-2012 Aalto University

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef HBOXINFO_HH
#define HBOXINFO_HH

#include <string>
#include <list>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <sys/wait.h>
#include "upnpdevice.hh"

#define IGD_DEVICETYPE "urn:schemas-upnp-org:device:InternetGatewayDevice:1"
#define IGD_WANIPCON_SERVICETYPE "urn:schemas-upnp-org:service:WANIPConnection:1"
#define IGD_WANCOMIFCFG_SERVICETYPE "urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1"

using namespace std;

/**
 * @class communication_info
 * @brief The hbox's communication information such as IP address, HIP address, Teredo address and capability.
 * @author Vu Ba Tien Dung
 *
 */
class communication_info {
private:
	string ipAddress;
	string hipAddress;
	string lsiAddress;
	string hipRelayAddress;
	bool isBehindNAT;
	bool isBehindFirewall;
	bool isHipRelay;
	bool isHip;
	bool isTeredo;
	
	int capability;
	
public:
	// RO3
	communication_info();
	communication_info(const string&);
	communication_info(const communication_info&);
	communication_info& operator=(const communication_info&);
	
	void init();
	int getCapability(); // the most preference capability e.g. HIP + relay -> HIP + teredo -> ...
	string findIpAddress();
	string findHit();	
	string findRelayAddress();
	string findLsi(string hipAddress);
	bool associateHip(communication_info&);
	
	string toString();
	
	// getters and setters
	void setIpAddress(const string& ipAddress) { this->ipAddress = ipAddress; }
	void setHipAddress(const string& hipAddress) { this->hipAddress = hipAddress; }
	void setLsiAddress(const string& lsiAddress) { this->lsiAddress = lsiAddress; }
	void setHipRelayAddress(const string& hipRelayAddress) { this->hipRelayAddress = hipRelayAddress; }
	void setBehindNAT(bool isBehindNAT) { this->isBehindNAT = isBehindNAT; }
	void setBehindFirewall(bool isBehindFirewall) { this->isBehindFirewall = isBehindFirewall; }
	void setHipRelay(bool isHipRelay) { this->isHipRelay = isHipRelay; }
	void setHip(bool isHip) { this->isHip = isHip; }
	void setTeredo(bool isTeredo) { this->isTeredo = isTeredo; }
	
	const string& getIpAddress() { return ipAddress; }
	const string& getHipAddress() { return hipAddress; }
	const string& getHipRelayAddress() { return hipRelayAddress; }
	const string& getLsiAddress() { return lsiAddress; }
	bool getBehindNAT() { return isBehindNAT; }
	bool getBehindFirewall() { return isBehindFirewall; }
	bool getHipRelay() { return isHipRelay; }
	bool getHip() { return isHip; }
	bool getTeredo() { return isTeredo; }
};

/**
 * @class hbox_info
 * @brief The hbox's major information such as its communication information, local UPnP service devices and remote hbox's information.
 * @author Vu Ba Tien Dung
 *
 */
class hbox_info {
private:
	string STATE;
	communication_info commInfo;
	list<upnp_device*> localUpnpDevices;
	string name; // JID

public:
	hbox_info();
	~hbox_info();
	
	// getters and setters
	void setState(const string& STATE) { this->STATE = STATE; }
	const string& getState() { return STATE; }
	void setCommInfo(const communication_info& commInfo) { this->commInfo = commInfo; }
	communication_info& getCommInfo() { return commInfo; }
	void setName(const string& name) { this->name = name; }
	const string& getName() { return name; }
	
	void addUpnpDevice(upnp_device*);
	bool removeUpnpDevice(string deviceUDN);
	upnp_device* findUpnpDevice(string deviceUDN);
	list<upnp_device*> getDeviceList() { return localUpnpDevices; }
	
	bool contains(string deviceUDN) {
		for (list<upnp_device*>::iterator it = localUpnpDevices.begin(); it != localUpnpDevices.end(); it++)
			if ((*it)->getDeviceName() == deviceUDN)
				return true;
		return false;
	}
};

#endif
