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

#ifndef HBOX_HH
#define HBOX_HH

#include <iostream>
#include <sstream>
#include <string>
#include <deque>
#include <cstdio>
#include <cstdlib>
#include <list>
#include <mutex>
#include <thread>
#include <exception>
#include <log4cpp/Category.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/BasicLayout.hh>
#include <boost/lexical_cast.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <getopt.h>
#include "xmppclient.hh"
#include "configfile.hh"
#include "descriptionfile.hh"
#include "upnpserver.hh"
#include "upnpclient.hh"
#include "threadsafe_queue.hh"
#include "hboxinfo.hh"
#include "upnpdevice.hh"
#include "event.hh"

using namespace std;
using namespace log4cpp;

namespace ba=boost::asio;
namespace bs=boost::system;

typedef boost::shared_ptr<ba::ip::tcp::socket> socket_ptr;
typedef boost::shared_ptr<ba::io_service> io_service_ptr;
typedef deque<io_service_ptr> ios_deque;

// Helper functions for logging
#define HBOX_DEBUG(a) hbox::log \
		<< log4cpp::Priority::DEBUG << " <" \
		<< __FILE__ << "::" << __FUNCTION__ << " (" << __LINE__ << ")> : " << a

#define HBOX_INFO(a) hbox::log \
		<< log4cpp::Priority::INFO << " <" \
		<< __FILE__ << "::" << __FUNCTION__ << " (" << __LINE__ << ")> : " << a

#define HBOX_NOTICE(a) hbox::log \
		<< log4cpp::Priority::NOTICE << " <" \
		<< __FILE__ << "::" << __FUNCTION__ << " (" << __LINE__ << ")> : " << a

#define HBOX_WARN(a) hbox::log \
		<< log4cpp::Priority::WARN << " <" \
		<< __FILE__ << "::" << __FUNCTION__ << " (" << __LINE__ << ")> : " << a

#define HBOX_ERROR(a) hbox::log \
		<< log4cpp::Priority::ERROR << " <" \
		<< __FILE__ << "::" << __FUNCTION__ << " (" << __LINE__ << ")> : " << a

#define HBOX_CRIT(a) hbox::log \
		<< log4cpp::Priority::CRIT << " <" \
		<< __FILE__ << "::" << __FUNCTION__ << " (" << __LINE__ << ")> : " << a

#define HBOX_ALERT(a) hbox::log \
		<< log4cpp::Priority::ALERT << " <" \
		<< __FILE__ << "::" << __FUNCTION__ << " (" << __LINE__ << ")> : " << a

#define HBOX_FATAL(a) hbox::log \
		<< log4cpp::Priority::FATAL << " <" \
		<< __FILE__ << "::" << __FUNCTION__ << " (" << __LINE__ << ")> : " << a


/**
 * @class hbox
 * @brief Main class which starts the software and initialize all related objects.
 * @author Matti
 * @author Vu Ba Tien Dung
 *
 */
class hbox {
private:
	// xmppclient object
	xmpp_client client;
	
	// upnp objects
	upnp_server virtualUpnpServer;
	upnp_client virtualControlPoint;
	
	// background logging
	bool background;
	// debug level
	int debuglevel;
	// append to the current log file
	bool appendlog;
	
	// communication channels
	blocking_queue<event> hbox_xmpp;
	blocking_queue<event> hbox_upnpserver;
	blocking_queue<event> hbox_upnpclient;
	blocking_queue<event> xmpp_hbox;
	blocking_queue<event> upnpserver_hbox;
	blocking_queue<event> upnpclient_hbox;	
	
	// hbox manager
	hbox_info self_hbox;
	list<hbox_info*> remote_hbox_es;
	int maxPort;
	
	boost::thread_group thr_grp;
	deque<ba::io_service::work> io_service_work;
	int THREAD_NUM;	
	
public:
	// the file which contains username and password of the xmppclient
	static string config_file;
	// log messages are stored in this file
	static string log_file;
	// handles c++ log messages
	static Category &log;
	
	// public methods
	hbox();
	~hbox();
	void getArguments(int argc, char *argv[]);
	void init();
	void start();
	void eventDispatching();
	
	// internal managament methods
	void newNeighborHbox(event&);
	void delNeighborHbox(event&);	
	
	void newLocalUPnPDevice(event&);
	void newLocalMediaUPnPDevice(event&);
	void addNetworkInfoLocalUPnPDevice(event& temp);
	void newLocalUPnPService(event&);
	void startLocalUPnPDevice(event&);
	void delLocalUPnPDevice(event&);
	void confirmLocalDatabases();
	
	void newRemoteUPnPDevice(event&);
	void setRemoteUPnPDevicePort(event&);
	void newRemoteUPnPService(event&);
	void startRemoteUPnPDevice(event&);
	void delRemoteUPnPDevice(event&);
	void saveSourceHboxToDescription(string&, const string&);
	
	void sendAction(event& temp);
	void sendActionResponse(event& temp);
	void actionControlReceived(event& temp);
	void actionResponseReceived(event& temp);
	void fixResourceURL(event& temp);

	void addHbox(hbox_info* other);
	hbox_info* getHbox(string hboxName);	
};

#endif
