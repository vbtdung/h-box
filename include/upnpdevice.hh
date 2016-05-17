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

#ifndef UPNPDEVICE_HH
#define UPNPDEVICE_HH

#include <string>
#include <list>

#include "proxyserver.hh"

using namespace std;

/**
 * @class upnp_service
 * @brief A data structure to represent an UPnP service.
 * @author Vu Ba Tien Dung
 *
 */
class upnp_service {
private:
	string serviceDescription;
	string serviceName;
public:
	/**
	 * Constructor of upnp_service class
	 * @param serviceDescription this is the service's description XML string
	 *
	 */
	upnp_service(const string& serviceDescription) : serviceDescription(serviceDescription) {
		serviceName = "";
	}
	
	/**
	 * Constructor of upnp_service class
	 * @param serviceDescription this is the service's description XML string
	 *
	 */
	upnp_service(const string& serviceName, const string& serviceDescription) : serviceName(serviceName), serviceDescription(serviceDescription) {
	}
	
	// getters and setters
	string getServiceName() const { return serviceName; }	
	string getServiceDescription() const { return serviceDescription; }	
};

/**
 * @class upnp_device
 * @brief UPnP device data structure.
 * @author Dung
 *
 */
class upnp_device {
private:
	string STATE;
	string deviceName;
	string deviceDescription;
	list<upnp_service> upnpServices;
	int remotePort;
	int localPort;
	int rewritePort;
	string ipAddress;
	bool isMediaServer;
	tcp_proxy_server* server;
	
public:
	/**
	 * Constructor of upnp_device class
	 * @param deviceDescription this is the device's description XML string
	 *
	 */
	upnp_device(const string& deviceDescription) : deviceDescription(deviceDescription) { 
		deviceName = "";
		isMediaServer = false;
		STATE = "INIT";
		
		remotePort = 0;
		localPort = 0;
		server = NULL;
	}
	
	~upnp_device() {
		if (server) delete server;
	}
	
	// getters and setters
	string getState() { return STATE; }
	void setState(const string& STATE) { this->STATE = STATE; }
	string getIpAddress() { return ipAddress; }
	void setIpAddress(string ipAddress) { this->ipAddress = ipAddress; } 
	int getRemotePort() { return remotePort; }
	void setRemotePort(int remotePort) { this->remotePort = remotePort; } 
	int getLocalPort() { return localPort; }
	void setLocalPort(int localPort) { this->localPort = localPort; } 
	bool getMediaServer() { return isMediaServer; }
	void setMediaServer(bool isMediaServer) { this->isMediaServer = isMediaServer; } 
	tcp_proxy_server* getServer() { return server; }
	void setServer(tcp_proxy_server* server) { this->server = server; } 
	
	void setDeviceName(string name) { this->deviceName = name; }
	string getDeviceName() { return deviceName; }
	
	void setDeviceDescription(string description) { this->deviceDescription = description; }
	string getDeviceDescription() { return deviceDescription; }
	
	list<upnp_service> getServiceList() { return upnpServices; }
	
	void start() {
		STATE = "READY";
	}
	
	void addUpnpService(const upnp_service& service) {
		for (list<upnp_service>::iterator it = upnpServices.begin(); it != upnpServices.end(); it++)
			if (it->getServiceName() == service.getServiceName())
				return;
		upnpServices.insert(upnpServices.begin(), service);
	}
	
	void removeUpnpService(const string serviceName) {
		list<upnp_service>::iterator it;
		for (it = upnpServices.begin(); it != upnpServices.end(); it++)
			if ((*it).getServiceName() == serviceName) 
				break;
	
		upnpServices.erase(it);
	}
};

#endif
