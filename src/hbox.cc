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

#include "hbox.hh"

using namespace log4cpp;

// static variables assignment
string hbox::config_file = "/etc/hbox.conf";
string hbox::log_file = "/tmp/hbox.log";

// logging singleton
Category &hbox::log = Category::getInstance("hbox");

/**
 * Hbox constructor
 *
 */
hbox::hbox() { 
	background = false;
	debuglevel = Priority::INFO;
	appendlog = true;
	
	client.setQueue(&xmpp_hbox);
	virtualControlPoint.setQueue(&upnpclient_hbox);
	
	maxPort = 54400;
	THREAD_NUM = 2;
}

/**
 * Hbox destructor
 *
 */
hbox::~hbox() { 
	log4cpp::Category::shutdown();
	
	for (list<hbox_info*>::iterator it = remote_hbox_es.begin(); it != remote_hbox_es.end(); it++) delete (*it);		
	remote_hbox_es.clear();
}

/**
 * Extracts username and password from the configfile as given in argv argument. Also decodes the debug level options
 * @param argc number of flags
 * @param argv flags to be decoded
 *
 */
void hbox::getArguments(int argc, char *argv[]) {
	int c;

	// get the opt and its value out
	while ((c = getopt(argc, argv, "c:bd:hl:tp")) != -1) {
		switch (c) {
			case 'c':
				config_file = std::string(optarg);
				break;
			case 'b':
				background = true;
				break;
			case 'd':
				debuglevel = std::atoi(optarg);
				break;
			case 'l':
				log_file = std::string(optarg);
				break;
			case 't':
				appendlog = false;
				break;
			case '?':
			case 'h':
				fprintf(stderr,
					"usage: %s [options]\n"
					" -p   switch on PubSUb feature\n"
					" -b     Run it in background\n"
					" -c configfilepath  Configuration file path\n"
					" -d 0-7             Debug level (FATAL ALERT CRIT "
					"ERROR WARN NOTICE INFO DEBUG)\n"
					" -l logfilepath     Log file path\n"
					" -t                 Truncate log file\n"
					, argv[0]);
				exit(EXIT_SUCCESS);
		}
	}
}

/**
 * Init all the main components of the program
 * Logging, XMPP client, 
 *
 */
void hbox::init() {
	HBOX_INFO("Initializing");

	// Init and Start Logging
	HBOX_INFO("Logging initializing");
	log.setAdditivity(false);
	log.setPriority(debuglevel * 100);
	log.setAppender(background 
		? (new FileAppender("FileAppender", log_file, appendlog))
		: (new FileAppender("_", ::dup(fileno(stdout))))
	);
	
	// logging is a background process
	if (background) {
		HBOX_INFO("Forking into background");
		if(fork() > 0) exit(EXIT_SUCCESS); // cannot create background process
	}

	// Init XMPP
	HBOX_DEBUG("Reading config file " + config_file);
	ConfigFile cf(config_file);
	
	// read the username/password/server from config file
	string username, password, server;
	username = cf.read<string>("username");
	password = cf.read<string>("password");
	server = cf.read<string>("server");

	// create the description.xml file from config file
	xml_description_file cd = xml_description_file("description.xml");
	if (!cd.changeUDN(username)) {
		HBOX_ERROR("Cannot change description.xml file");
		exit(EXIT_SUCCESS);
	}

	// init the XMPP client object
	try {
		if (!client.init(username, password, server)) {
			HBOX_INFO("XMPP initialization failed");
			exit(EXIT_SUCCESS);
		}
	}
	catch (...) {
		if (!client.init(username, password)) {
			HBOX_INFO("xmpp initialization failed");
			exit(EXIT_SUCCESS);
		}
	}
	
	// Init Virtual UPNP Server
	virtualUpnpServer = upnp_server();	
	virtualUpnpServer.setQueue(&upnpserver_hbox);
	
	// Init the communication information
	communication_info commInfo;
	commInfo.init();
	self_hbox.setCommInfo(commInfo);
}

/**
 * @class xmppclient_thread
 * @brief The thread struct in which the XMPP client process runs (The thread struct is neccessary for C++0x thread)
 * @author Vu Ba Tien Dung
 *
 */
struct xmppclient_thread { // xmpp thread 
	private:
		xmpp_client &client;
		blocking_queue<event> &hbox_xmpp;
		
	public:
		xmppclient_thread(xmpp_client& client_, blocking_queue<event>& hbox_xmpp_) : client(client_), hbox_xmpp(hbox_xmpp_) { }
		
		void operator()() {	
			client.run(); 
			
			while (true) {
				event temp;
				
				(client.getclient())->recv(1);
				
				if (hbox_xmpp.pop(temp)) {
					client.onMessage(temp);					
				}
			}
		}
};

/**
 * @class upnpserver_thread
 * @brief The thread struct in which the local-and-virtual UPnP server process runs (The thread struct is neccessary for C++0x thread)
 * @author Vu Ba Tien Dung
 *
 */
struct upnpserver_thread { // upnp server thread
	private:
		upnp_server &server;
		blocking_queue<event> &hbox_upnpserver;
		
	public:
		upnpserver_thread(upnp_server& server_, blocking_queue<event>& hbox_upnpserver_):server(server_), hbox_upnpserver(hbox_upnpserver_) { }
		void operator()() {	
			server.run(); 
			
			while (true) {
				event temp;
		
				if (hbox_upnpserver.pop(temp)) {
					server.onMessage(temp);
				}
				
				sleep(1);
			}
		}
};

/**
 * @class upnpclient_thread
 * @brief The thread struct in which the local-and-virtual UPnP control point process runs (The thread struct is neccessary for C++0x thread)
 * @author Vu Ba Tien Dung
 *
 */
struct upnpclient_thread { // upnp control point thread
	private:
		upnp_client &controlpoint;
		blocking_queue<event> &hbox_upnpclient;
		
	public:
		upnpclient_thread(upnp_client& controlpoint_, blocking_queue<event>& hbox_upnpclient_):controlpoint(controlpoint_), hbox_upnpclient(hbox_upnpclient_) { }
		
		void operator()() {	
			controlpoint.run(); 
			
			while (true) {
				event temp;
		
				if (hbox_upnpclient.pop(temp)) {
					controlpoint.onMessage(temp);
				}
		
				sleep(1);
			}
		}
};

/**
 * The hbox listens to all the incoming events and dispatches events to correct threads
 *
 */
void hbox::eventDispatching() {
	while (true /* !xmppclient_t.joinable() && !upnpserver_t.joinable() && !upnpclient_t.joinable() */) {
		event temp;
	
		if (upnpclient_hbox.pop(temp)) {
			if (temp.isUpnpInfo()) {
				if (temp.getCommand() == "NEW")
					newLocalUPnPDevice(temp);
				else if (temp.getCommand() == "NEW_MEDIA")
					newLocalMediaUPnPDevice(temp);
				else if (temp.getCommand() == "PORT")
					addNetworkInfoLocalUPnPDevice(temp); 
				else if (temp.getCommand() == "START")
					startLocalUPnPDevice(temp);
				else if (temp.getCommand() == "DEL")
					delLocalUPnPDevice(temp);
				else if (temp.getCommand() == "ACTION_RESPONSE")
					sendActionResponse(temp);
			}
			else {
				if (temp.getCommand() == "SERVICE") 
					newLocalUPnPService(temp);
			}
		}
	
		if (upnpserver_hbox.pop(temp)) {
			if (temp.isUpnpInfo()) {
				if (temp.getCommand() == "ACTION")
					sendAction(temp);
			}
		}
	
		if (xmpp_hbox.pop(temp)) {
			if (temp.isHboxInfo()) {
				if (temp.getCommand() == "NEW") 
					newNeighborHbox(temp);
				else if (temp.getCommand() == "DEL")
					delNeighborHbox(temp);
			}
			else {
				if (temp.getCommand() == "NEW")
					newRemoteUPnPDevice(temp);
				else if (temp.getCommand() == "PORT") 
					setRemoteUPnPDevicePort(temp);
				else if (temp.getCommand() == "SERVICE") 
					newRemoteUPnPService(temp);
				else if (temp.getCommand() == "START")
					startRemoteUPnPDevice(temp);
				else if (temp.getCommand() == "DEL") 
					delRemoteUPnPDevice(temp);
				else if (temp.getCommand() == "ACTION")
					actionControlReceived(temp);
				else if (temp.getCommand() == "ACTION_RESPONSE")
					actionResponseReceived(temp);	
			}
		}
	
		sleep(1);
	}
}

/**
 * The hbox will add the new available neighbor to its list
 * @param temp the information of the new available neighbor
 */
void hbox::newNeighborHbox(event& temp) {
	if (temp.getName() == temp.getDescription()) {
		event ev = event("NEW", false, temp.getName(), self_hbox.getCommInfo().toString());
		hbox_xmpp.push(ev);
	}
	else { 
		bool hbox_is_new = true;
		
		for (list<hbox_info*>::iterator it = remote_hbox_es.begin(); it != remote_hbox_es.end(); it++)
			if ((*it)->getName() == temp.getName()) {
				hbox_is_new = false;
				break;
			}						
		
		if (hbox_is_new) {
			hbox_info *new_hbox = new hbox_info();
			new_hbox->setName(temp.getName());
			new_hbox->setCommInfo(communication_info(temp.getDescription()));
			addHbox(new_hbox);
		
			event ev = event("NEW", false, temp.getName(), self_hbox.getCommInfo().toString());
			hbox_xmpp.push(ev);
			
			// HIP association
			self_hbox.getCommInfo().associateHip(new_hbox->getCommInfo());
			
			// send to this newly added neighbor all devices
			list<upnp_device*> device_list = self_hbox.getDeviceList();			
			for (list<upnp_device*>::iterator it = device_list.begin(); it != device_list.end(); it++)
				if ((*it)->getState() == "READY") {
					event ev = event("NEW", true, temp.getName(), (*it)->getDeviceName() + "|" + (*it)->getDeviceDescription()); // string C++ is mutable, it is fine to append strings this way
					hbox_xmpp.push(ev);					
					
					if ((*it)->getMediaServer()) {
						event ev = event("PORT", true, temp.getName(), (*it)->getDeviceName() + "|" + boost::lexical_cast<string>((*it)->getRemotePort()) + "|" + boost::lexical_cast<string>((*it)->getLocalPort()));
						hbox_xmpp.push(ev);			
					}
		
					list<upnp_service> service_list = (*it)->getServiceList();
					for (list<upnp_service>::iterator sit = service_list.begin(); sit != service_list.end(); sit++)
					{
						ev = event("SERVICE", true, temp.getName(), (*it)->getDeviceName() + "|" + sit->getServiceName() + "|" + sit->getServiceDescription()); 
						hbox_xmpp.push(ev);
					}
		
					ev = event("START", true, temp.getName(), (*it)->getDeviceName());
					hbox_xmpp.push(ev);
					sleep(1);
				}
		}				
	}
}

/**
 * The hbox will remove the neighbor from its list
 * @param temp the information of the removal neighbor
 */
void hbox::delNeighborHbox(event& temp) {
	for (list<hbox_info*>::iterator it = remote_hbox_es.begin(); it != remote_hbox_es.end(); it++)
		if ((*it)->getName() == temp.getName()) {
			remote_hbox_es.erase(it);
			delete (*it);
			break;
	}
}

/**
 * The hbox will add the new local UPnP device to its databases
 * @param temp the information of the new local device
 *
 */
void hbox::newLocalUPnPDevice(event& temp) {
	upnp_device *dev = new upnp_device(temp.getDescription());
	dev->setDeviceName(temp.getName());
	self_hbox.addUpnpDevice(dev);
}

/**
 * The hbox will add the new local media UPnP device to its databases
 * @param temp the information of the new local device
 *
 */
void hbox::newLocalMediaUPnPDevice(event& temp) {
	upnp_device *dev = new upnp_device(temp.getDescription());
	dev->setDeviceName(temp.getName());
	dev->setMediaServer(true);
	self_hbox.addUpnpDevice(dev);
}

void hbox::addNetworkInfoLocalUPnPDevice(event& temp) {
	// MediaServerIP|MediaServerPort

	int pos = temp.getDescription().find("|");	
	string serverIP, serverPort;
	serverIP = temp.getDescription().substr(0, pos);
	serverPort = temp.getDescription().substr(pos + 1);
	
	self_hbox.findUpnpDevice(temp.getName())->setIpAddress(serverIP);
	self_hbox.findUpnpDevice(temp.getName())->setRemotePort(atoi(serverPort.c_str()));
	self_hbox.findUpnpDevice(temp.getName())->setLocalPort(maxPort++);
	
	try {
		ios_deque io_services;
				
		for (int i = 0; i < THREAD_NUM; i++) {
			io_service_ptr ios(new ba::io_service);
			io_services.push_back(ios);
			io_service_work.push_back(ba::io_service::work(*ios));
			thr_grp.create_thread(boost::bind(&ba::io_service::run, ios));
		}
		
		tcp_proxy_server* server = new tcp_proxy_server(io_services, /* listenning port */ maxPort - 1, /* forwarding address */ serverIP, /* forwarding port */ atoi(serverPort.c_str()));
		self_hbox.findUpnpDevice(temp.getName())->setServer(server); // pass the pointer of server object to upnp device
	} 
	catch (exception& e) {
		HBOX_ERROR("Exception thrown " << e.what());
	}
}

/**
 * The hbox will find the local UPnP device which owns the service and add the service to its databases
 * @param temp the information of the service
 *
 */
void hbox::newLocalUPnPService(event& temp) {
	int pos = temp.getName().find("|");
	string deviceName, serviceName;
	deviceName = temp.getName().substr(0, pos);
	serviceName = temp.getName().substr(pos + 1);

	upnp_service service = upnp_service(serviceName, temp.getDescription());
	(self_hbox.findUpnpDevice(deviceName))->addUpnpService(service);
}

/**
 * The hbox will start the new local UPnP device 
 * @param temp the information of the local device
 *
 */
void hbox::startLocalUPnPDevice(event& temp) {
	(self_hbox.findUpnpDevice(temp.getName()))->start();
	// confirmLocalDatabases();
	
	// send this newly added device to all neighbors	
	for (list<hbox_info*>::const_iterator it = remote_hbox_es.begin(); it != remote_hbox_es.end(); it++)
	{
		event ev = event("NEW", true, (*it)->getName(), temp.getName() + "|" + (self_hbox.findUpnpDevice(temp.getName()))->getDeviceDescription());
		hbox_xmpp.push(ev);
		
		if ((self_hbox.findUpnpDevice(temp.getName()))->getMediaServer()) {
			event ev = event("PORT", true, (*it)->getName(), temp.getName() + "|" + boost::lexical_cast<string>((self_hbox.findUpnpDevice(temp.getName()))->getRemotePort()) + "|" + boost::lexical_cast<string>((self_hbox.findUpnpDevice(temp.getName()))->getLocalPort()));
			hbox_xmpp.push(ev);			
		}
				
		list<upnp_service> service_list = (self_hbox.findUpnpDevice(temp.getName()))->getServiceList();
		for (list<upnp_service>::iterator sit = service_list.begin(); sit != service_list.end(); sit++)
		{
			ev = event("SERVICE", true, (*it)->getName(), temp.getName() + "|" + sit->getServiceName() + "|" + sit->getServiceDescription()); 
			hbox_xmpp.push(ev);
		}
		
		ev = event("START", true, (*it)->getName(), temp.getName());
		hbox_xmpp.push(ev);
		sleep(1);
	}
}

/**
 * The hbox will remove the local UPnP device from its databases
 * @param temp the information of the removal local device
 *
 */
void hbox::delLocalUPnPDevice(event& temp) {
	self_hbox.removeUpnpDevice(temp.getName());
	// confirmLocalDatabases();
	
	for (list<hbox_info*>::const_iterator it = remote_hbox_es.begin(); it != remote_hbox_es.end(); it++)
	{
		event ev = event("DEL", true, (*it)->getName(), temp.getName());		
		hbox_xmpp.push(ev);
	}
}

void hbox::newRemoteUPnPDevice(event& temp) {
	hbox_info* hbox = getHbox(temp.getName());
	
	int pos = temp.getDescription().find("|");
	string deviceName, deviceDescription;
	deviceName = temp.getDescription().substr(0, pos);
	deviceDescription = temp.getDescription().substr(pos + 1);
	saveSourceHboxToDescription(deviceDescription, temp.getName());
	
	if (!hbox->contains(deviceName))
	{
		upnp_device *dev = new upnp_device(deviceDescription);
		dev->setDeviceName(deviceName);
		hbox->addUpnpDevice(dev);
	}
}

void hbox::setRemoteUPnPDevicePort(event& temp) {
	hbox_info* hbox = getHbox(temp.getName());
	
	int pos1 = temp.getDescription().find("|");
	int pos2 = temp.getDescription().find("|", pos1 + 1);
	string deviceName, serverPort, remotePort;
	deviceName = temp.getDescription().substr(0, pos1);
	serverPort = temp.getDescription().substr(pos1 + 1, pos2 - pos1 - 1);
	remotePort = temp.getDescription().substr(pos2 + 1);
	
	(hbox->findUpnpDevice(deviceName))->setLocalPort(maxPort++);
	(hbox->findUpnpDevice(deviceName))->setRemotePort(atoi(remotePort.c_str())); // remotePort
	
	try {
		ios_deque io_services;
				
		for (int i = 0; i < THREAD_NUM; i++) {
			io_service_ptr ios(new ba::io_service);
			io_services.push_back(ios);
			io_service_work.push_back(ba::io_service::work(*ios));
			thr_grp.create_thread(boost::bind(&ba::io_service::run, ios));
		}
		
		tcp_proxy_server* server;
		if ((hbox->getCommInfo()).getHip())
			server = new tcp_proxy_server(io_services, /* listenning port */ maxPort - 1, /* forwarding address */ (hbox->getCommInfo()).getLsiAddress(), /* forwarding port */ atoi(remotePort.c_str()));
		else
			server = new tcp_proxy_server(io_services, /* listenning port */ maxPort - 1, /* forwarding address */ (hbox->getCommInfo()).getIpAddress(), /* forwarding port */ atoi(remotePort.c_str()));
			
		(hbox->findUpnpDevice(deviceName))->setServer(server); // pass the pointer of server object to upnp device
	} 
	catch (exception& e) {
		HBOX_ERROR("Exception thrown " << e.what());
	}
}

void hbox::saveSourceHboxToDescription(string& deviceDescription, const string& remoteHboxJID) {
	int pos = deviceDescription.find("</UDN>");
	deviceDescription.insert(pos + 6, "<UPC>" + remoteHboxJID + "</UPC>");
}

void hbox::newRemoteUPnPService(event& temp) {
	hbox_info* hbox = getHbox(temp.getName());	
	
	int pos1 = temp.getDescription().find("|");
	int pos2 = temp.getDescription().find("|", pos1 + 1);
	string deviceName, serviceName, serviceDescription;
	deviceName = temp.getDescription().substr(0, pos1);
	serviceName = temp.getDescription().substr(pos1 + 1, pos2 - pos1 - 1);
	serviceDescription = temp.getDescription().substr(pos2 + 1);

	upnp_service service = upnp_service(serviceName, serviceDescription);
	(hbox->findUpnpDevice(deviceName))->addUpnpService(service);
}

void hbox::startRemoteUPnPDevice(event& temp) {
	hbox_info* hbox = getHbox(temp.getName());
	
	if ((hbox->findUpnpDevice(temp.getDescription()))->getState() != "READY")
	{
		(hbox->findUpnpDevice(temp.getDescription()))->start();
		
		// initiate the remote device as an embedded device of the virtual upnp server "HBOX Device"
		event ev = event("NEW", true, (hbox->findUpnpDevice(temp.getDescription()))->getDeviceName(), (hbox->findUpnpDevice(temp.getDescription()))->getDeviceDescription());
		hbox_upnpserver.push(ev);
		
		ev = event("START", true, (hbox->findUpnpDevice(temp.getDescription()))->getDeviceName(), (hbox->findUpnpDevice(temp.getDescription()))->getDeviceName());
		hbox_upnpserver.push(ev);
		
		list<upnp_service> service_list = (hbox->findUpnpDevice(temp.getDescription()))->getServiceList();
		for (list<upnp_service>::iterator sit = service_list.begin(); sit != service_list.end(); sit++)
		{
			ev = event("SERVICE", true, temp.getDescription() + "|" + sit->getServiceName(), sit->getServiceDescription()); 
			hbox_upnpserver.push(ev);
		}
		
		ev = event("RESTART", true, (hbox->findUpnpDevice(temp.getDescription()))->getDeviceName(), (hbox->findUpnpDevice(temp.getDescription()))->getDeviceName());
		hbox_upnpserver.push(ev);
		
	}
}

void hbox::delRemoteUPnPDevice(event& temp) {
	hbox_info* hbox = getHbox(temp.getName());	
	HBOX_DEBUG("hbox name: " << hbox->getName());
	bool success = hbox->removeUpnpDevice(temp.getDescription());
}

void hbox::sendAction(event& temp) {
	hbox_xmpp.push(temp);
}

void hbox::sendActionResponse(event& temp) {
	hbox_xmpp.push(temp);
}

void hbox::actionControlReceived(event& temp) {
	hbox_upnpclient.push(temp);
}

void hbox::actionResponseReceived(event& temp) {
	fixResourceURL(temp);
	hbox_upnpserver.push(temp);
}

void hbox::fixResourceURL(event& temp) {
	string new_resource = temp.getDescription();
	
	vector<string> action_response_related_data;
	boost::split(action_response_related_data, new_resource, boost::is_any_of("|"));	
	string actionResponseReceivedUpnpUDN = action_response_related_data[0];
	
	if (getHbox(temp.getName())->findUpnpDevice(actionResponseReceivedUpnpUDN)->getLocalPort() == 0)
		return;
	
	string::size_type remote_url_start_pointer, remote_url_end_pointer;
	remote_url_start_pointer = new_resource.find("<res");
	string final_address = "";
	
	while(remote_url_start_pointer != string::npos) {
		if (final_address == "" ) {
			string localIp = self_hbox.getCommInfo().getIpAddress();
			int localPort = getHbox(temp.getName())->findUpnpDevice(actionResponseReceivedUpnpUDN)->getLocalPort();
			
			stringstream localPortStringStream; 
			string localPortString;
			localPortStringStream << localPort; 
			localPortString = localPortStringStream.str();
			final_address = "http://" + localIp + ":" + localPortString;
		}

		remote_url_start_pointer = new_resource.find(">", remote_url_start_pointer);
		remote_url_start_pointer = new_resource.find("http://", remote_url_start_pointer);
		remote_url_end_pointer = new_resource.find("/", remote_url_start_pointer + 7);

		new_resource = new_resource.substr(0, remote_url_start_pointer) + final_address + new_resource.substr(remote_url_end_pointer);
		remote_url_start_pointer = new_resource.find("<res", remote_url_end_pointer);
	}
	
	//action_response_related_data_temp.replace(remote_url_start_pointer, remote_url_end_pointer - remote_url_start_pointer, final_address);
	HBOX_DEBUG("Action response related data: " << new_resource);
 	temp.setDescription(new_resource);
}

void hbox::confirmLocalDatabases() {
	list<upnp_device*> device_list = self_hbox.getDeviceList();

	HBOX_DEBUG("Number of devices: " << device_list.size());	
	for (list<upnp_device*>::iterator it = device_list.begin(); it != device_list.end(); it++)
	{
		HBOX_DEBUG("Device: " << (*it)->getDeviceName());
		list<upnp_service> service_list = (*it)->getServiceList();
		for (list<upnp_service>::iterator sit = service_list.begin(); sit != service_list.end(); sit++)
			HBOX_DEBUG("       Service: " << sit->getServiceName());
	}
}
 
/**
 * Add a hboxinfo into the remote hboxes list
 * @param other one remote hbox
 *
 */
void hbox::addHbox(hbox_info* other) {
	remote_hbox_es.insert(remote_hbox_es.begin(), other);
}

/**
 * Get an hboxinfo from the remote hboxes list based on its name
 * @param hboxName the XMPP identity of the remote hbox
 * @return the pointer to that hbox, NULL if not found
 *
 */
hbox_info* hbox::getHbox(string hboxName) {
	for (list<hbox_info*>::iterator it = remote_hbox_es.begin(); it != remote_hbox_es.end(); it++)
		if ((*it)->getName() == hboxName)
			return (*it);
	return NULL;
}

/**
 * Starts the hbox instance and all main components
 *
 */
void hbox::start() {
	HBOX_INFO("Running...");
	
	// Start XMPP in a separate thread
	thread xmppclient_t((xmppclient_thread(client, hbox_xmpp)));
	thread upnpserver_t((upnpserver_thread(virtualUpnpServer, hbox_upnpserver)));
	thread upnpclient_t((upnpclient_thread(virtualControlPoint, hbox_upnpclient)));
	
	// start the dispatching loop
	eventDispatching();
	
	// wait until all the threads stop
	xmppclient_t.join();
	upnpserver_t.join();
	upnpclient_t.join();
	thr_grp.join_all();
	exit(EXIT_SUCCESS);
}

/**
 * PROGRAM'S MAIN FUNCTION
 *
 */
int main(int argc, char *argv[]) {
	hbox hb;
	cout << "HBOX STARTING ... " << endl; // no logging yet
	
	// Initialize hbox
	hb.getArguments(argc, argv);
	hb.init();
	hb.start();

	return 0;
}
