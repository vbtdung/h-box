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

#ifndef UPNPCLIENT_HH
#define UPNPCLIENT_HH

#include <string>
#include <list>
#include <vector>
#include <unordered_set>

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
#include "upnpdevice.hh"

using namespace CyberLink;
using namespace std;

/**
 * @class upnp_client
 * @brief The local-and-virtual control point which scans the local UPnP service devices and invoke actions on the behalf of the remote hbox.
 * @author Matti
 * @author Amit Soni
 * @author Vu Ba Tien Dung
 *
 */
class upnp_client : public ControlPoint, public DeviceChangeListener {
private:
	blocking_queue<event> *upnpclient_hbox;
	unordered_set<string> rootDevices;
	
public:
	upnp_client();
	string getHttpContent(Device *dev);
	list<upnp_service> collectServices(Device *dev);
	bool isMediaServer(Device *dev);
	string findMediaServerNetInfo(Device *dev);
	
	// init and start the control point
	void setQueue(blocking_queue<event> *_hbox);
	void run();
			
	// event listener
	void onMessage(const event& msg);
	void invokeAction(const event& msg);
	
	// overload methods for DeviceChangeListener
	void deviceAdded(Device *dev);
	void deviceRemoved(Device *dev);
};

#endif
