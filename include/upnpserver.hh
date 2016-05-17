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

#ifndef UPNPSERVER_HH
#define UPNPSERVER_HH

#include <algorithm>
#include <string>
#include <cstring>
#include <mutex>
#include <thread>

#include <cybergarage/http/HTTPRequest.h>
#include <cybergarage/upnp/CyberLink.h>
#include <cybergarage/upnp/ssdp/SSDPPacket.h>
#include <cybergarage/net/DatagramPacket.h>
#include <cybergarage/upnp/xml/DeviceData.h>
#include <cybergarage/xml/Parser.h>
#include <cybergarage/http/HTTPRequest.h>
#include <cybergarage/upnp/CyberLink.h>
#include <cybergarage/upnp/StateVariable.h>

#include "threadsafe_queue.hh"
#include "event.hh"

using namespace std;
using namespace CyberLink;
using namespace CyberHTTP;

/**
 * @class virtual_upnp
 * @brief The virtual UPnP device which runs locally and acts as the remote UPnP devices. This class extends UPnP's Device class and adds all the capabilites to support remote action control and query control.
 * @author Matti
 * @author Amit Soni
 * @author Vu Ba Tien Dung
 *
 */
class virtual_upnp : public Device, public ActionListener, public QueryListener {
private:
	HTTPRequest* m_lastHTTPReq;
	blocking_queue<event> *upnpserver_hbox;
	
public:
	virtual_upnp(int port);
	virtual_upnp(const char *devName, int port);
	void setQueue(blocking_queue<event> *_hbox);
			
	// overload the ActionListener
	void httpRequestReceived(HTTPRequest*);
	bool actionControlReceived(Action *action);
	bool queryControlReceived(StateVariable *stateVar);
	void resetListeners();
};

/**
 * @class upnp_server
 * @brief UPnP server holds information of all remote UPnP service devices and manages an instance of virtual UPnP service device.
 * @author Vu Ba Tien Dung
 *
 */
class upnp_server {
private:
	virtual_upnp* server;	
	blocking_queue<event> *upnpserver_hbox;
		
	string totalDescription;
	int startport;
	
public:
	upnp_server();
	upnp_server(const char *devName);
	~upnp_server();
	
	// init and run the upnp server
	void run();
	bool start();
	void setQueue(blocking_queue<event> *_hbox);
	
	// event listener
	void onMessage(const event& msg);	
	
	void newEmbeddedDevice(const event&);
	void newEmbeddedService(const event&);
	void startEmbeddedDevice(const event&);			
	void restartEmbeddedDevice(const event&);	
	void delEmbeddedDevice(const event&);
	
	void actionResponseReceived(const event&);
	
	void search_and_replace(string &str, const string &oldsubstr, const string &newsubstr);
};

#endif
