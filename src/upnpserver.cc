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

#include "upnpserver.hh"
#include "hbox.hh"

using namespace std;
namespace fs = boost::filesystem;

const char* defaultDescriptionFile = "description.xml";
const string DESCRIPTION_DIR = "/tmp/hbox/remotedevices";

// conditional variable
mutex shared_mutex;
condition_variable shared_condition_variable;

virtual_upnp::virtual_upnp(const char *devName, int port) : Device(devName) {
	setNMPRMode(true);
	
	this->setHTTPPort(port);
	setQueryListener(this, true);
	setActionListener(this, true);
}

virtual_upnp::virtual_upnp(int port) : Device() {
	setNMPRMode(true);
	
	this->setHTTPPort(port);
	setQueryListener(this, true);
	setActionListener(this, true);
}

void virtual_upnp::resetListeners() {
	setQueryListener(this, true);
	setActionListener(this, true);
}

void virtual_upnp::setQueue(blocking_queue<event> *_hbox) {
	upnpserver_hbox = _hbox;
}

/**
 * This method will handle the http request to the upnpserver object and is reimplemented to store the httpRequest object.
 * @param httpReq HTTPRequest object
 *
 */
void virtual_upnp::httpRequestReceived(HTTPRequest* httpReq) {
	m_lastHTTPReq = httpReq;
	Device::httpRequestRecieved(httpReq);
}

/**
 * This method will handle the action controls on the local virtual upnp devices. Since these virtual upnp device are actually remote devices, xmpp message containing the details of the action is send to the remote hbox (where the actual device is present) and then response of the action is also received through xmpp.
 * @param action Action object containing the input arguments of the action.
 * @return boolean value true if the action was successfull, false otherwise
 *
 */ 
bool virtual_upnp::actionControlReceived(Action *action) {
	string remoteHboxJID(action->getService()->getDevice()->getUPC());
	HBOX_DEBUG("An action was received at device of " << remoteHboxJID);	
	
	// Making the message body. The seperator would be "|". The first field would be the UDN of the immediate device
	// Subsequent fields would have some action related data which are as follows
	// Second field would be action name by using that we can create the action object at the remote hbox using the method device->getAction(ActionName);
	// Third field would be the argument name and the fourth would be the argument value, fifth would be next argument is there is and sixth would be its value and so on.
	string actionDetailed = action->getService()->getDevice()->getUDN();
	actionDetailed += "|";
	actionDetailed += action->getName();
	ArgumentList *argList = action->getArgumentList();

	for (int i = 0; i < argList->size(); i++) {
		actionDetailed += "|";
		actionDetailed += argList->getArgument(i)->getName();
		actionDetailed += "|";
		actionDetailed += argList->getArgument(i)->getValue();
	}
	
	event ev = event("ACTION", true, remoteHboxJID, actionDetailed);
	upnpserver_hbox->push(ev);
	
	HBOX_DEBUG("Before acquiring the lock");
	unique_lock<std::mutex> lock(shared_mutex);
	HBOX_DEBUG("Acquire the lock successful");
	shared_condition_variable.wait(lock);
	HBOX_DEBUG("the condition variable is signaled");
	lock.unlock();
	
	return true;
}

/**
 * The method handles the query control on the local virtual upnp devices
 * @param stateVar StateVariable object on which the query is received
 * @return Currently we return only boolean value false since we are not implementing this function for remote query control.
 *
 */
bool virtual_upnp::queryControlReceived(StateVariable *stateVar) {
	HBOX_DEBUG("Query received: " << stateVar->getName());
	return true;
}
	
/**
 * Constructor of the class upnpserver. This constructor is overloaded to set few parameters to the default upnp device.
 * @param devname Name of the device begin made
 *
 */
upnp_server::upnp_server(const char *devName) {	
	startport = 15000;
	server = new virtual_upnp(devName, startport+=10);
		
	// read the default description into string
	ifstream descriptionFile(defaultDescriptionFile);
	totalDescription = string((std::istreambuf_iterator<char>(descriptionFile)), std::istreambuf_iterator<char>()); 
	descriptionFile.close();
}

/**
 * Constructor of the class upnpserver is overloaded to set few parameters to the default upnp device.
 *
 */
upnp_server::upnp_server() {	
	startport = 15000;
	server = new virtual_upnp(defaultDescriptionFile, startport+=10);
	
	// read the default description into string
	ifstream descriptionFile(defaultDescriptionFile);
	totalDescription = string((std::istreambuf_iterator<char>(descriptionFile)), std::istreambuf_iterator<char>()); 
	descriptionFile.close();
}

upnp_server::~upnp_server() {
}

/**
 * Start the upnp server
 *
 */
void upnp_server::run() {
	bool ret = start();
	
	if (!ret) 
		HBOX_INFO("The upnp server thread is failed to start");
	else
	{
		HBOX_INFO("still reading");
	}
}

/**
 * This method is used to start the upnpserver object as a local upnp device. This method is overloaded because Query and Action Listener need to be set after SCPD. Also we need to link the upnpDevice to its device manager in this method. Device manager is responsible of adding xmpp sending receiving capabilites to the upnpserver.
 * @param upnpDeviceManager Device manager object which will manage the upnpDevice object.
 *
 */
bool upnp_server::start() {
	bool ret = ((Device* )server)->start();
	return ret;
}

/**
 * This method is used to assign the shared communication channel between the hbox and the upnpserver threads
 * @param upnpserver_hbox the message queue from the upnpserver to the hbox
 *
 */
void upnp_server::setQueue(blocking_queue<event> *_hbox) {
	upnpserver_hbox = _hbox;
	server->setQueue(upnpserver_hbox);
}

/**
 * This method is executed whenever the hbox main thread sends some message to upnpserver thread
 * @param msg the message content
 *
 */
void upnp_server::onMessage(const event& msg) {
	// HBOX_DEBUG("Received something from the hbox: " << msg);
	
	if (msg.isUpnpInfo()) {
		if (msg.getCommand() == "NEW") 
			newEmbeddedDevice(msg);
		else if (msg.getCommand() == "SERVICE")
			newEmbeddedService(msg);
		else if (msg.getCommand() == "START")
			startEmbeddedDevice(msg);			
		else if (msg.getCommand() == "RESTART")
			restartEmbeddedDevice(msg);		
		else if (msg.getCommand() == "DEL")
			delEmbeddedDevice(msg);			
		else if (msg.getCommand() == "ACTION_RESPONSE")
			actionResponseReceived(msg);		
	}
}

void upnp_server::newEmbeddedDevice(const event& temp) {
	string copiedTotalDescription(totalDescription); 
	delete (server);
	
	string::size_type deviceDescBegin, deviceDescEnd, descDeviceList, deviceDescScpdUrl, deviceDescControlUrl, deviceDescEventSubUrl;
	string deviceDesc = temp.getDescription();
	//search_and_replace(deviceDesc, "&lt;", "<");
	//search_and_replace(deviceDesc, "&gt;", ">");
	
	if ((deviceDescScpdUrl = deviceDesc.find("<SCPDURL>")) == string::npos) {
		HBOX_DEBUG("SCPD url not found.");
		return;
	} 
	else {
		while (deviceDescScpdUrl != string::npos) {
			if (deviceDesc.substr(deviceDescScpdUrl + string("<SCPDURL>").length(), 7).compare("http://") && deviceDesc.substr(deviceDescScpdUrl + string("<SCPDURL>").length(), 1).compare("/") )
				deviceDesc.insert(deviceDescScpdUrl + std::string("<SCPDURL>").length(), "/");
				
			deviceDescScpdUrl = deviceDesc.find("<SCPDURL>", deviceDescScpdUrl + 1);
		}
	}
	
	deviceDescBegin = deviceDesc.find("<device>");
	deviceDescEnd = deviceDesc.find("</root>", deviceDescBegin);
	descDeviceList = copiedTotalDescription.find("<deviceList>");

	HBOX_DEBUG("Inserting the description into the agregate description file");
	copiedTotalDescription.insert(descDeviceList + string("<deviceList>").length(), deviceDesc.substr(deviceDescBegin, deviceDescEnd - deviceDescBegin));
	totalDescription = string(copiedTotalDescription);
	sleep(5);
}

void upnp_server::newEmbeddedService(const event& temp) {
	int pos = temp.getName().find("|");
	string deviceName, serviceName;
	deviceName = temp.getName().substr(0, pos);
	serviceName = temp.getName().substr(pos + 1);
	
	DeviceList *childDeviceList = server->getDeviceList();
	int numChild = childDeviceList->size();
	HBOX_DEBUG("Number of child devices: " << numChild);	
	for (int i = 0; i < numChild; i++) {
		Device *childDevice = childDeviceList->getDevice(i);
		
		if (string(childDevice->getUDN()) == deviceName) {
			HBOX_DEBUG("Local device for scpd found with name: " << childDevice->getFriendlyName());
			Service* service = childDevice->getServiceBySCPDURL(serviceName.c_str());
			if (service)
				HBOX_DEBUG("Remote device SCPD info set " << (service->loadSCPD(temp.getDescription().c_str()) ? "successfully" : "failed") << ".");
			else
				HBOX_DEBUG("Cannot find the service with SCPD info set");

			break;
		}
	}
}

void upnp_server::startEmbeddedDevice(const event& temp) {
	fs::create_directories(DESCRIPTION_DIR);
	std::string descriptionFileName = DESCRIPTION_DIR + "/description.xml";
	std::ofstream descriptionFile;
	descriptionFile.open(descriptionFileName.c_str());
		
	if(!descriptionFile.fail()) {		
		descriptionFile << totalDescription;
		descriptionFile.close();
		HBOX_DEBUG("Writing the description to file successfully");

		try {
			server = new virtual_upnp(descriptionFileName.c_str(), startport+=10);
			server->setQueue(upnpserver_hbox);
			((Device* )server)->start();
			HBOX_DEBUG("Making of remote devices was success");
		}
		catch (InvalidDescriptionException e) {
			const char *errMsg = e.getMessage();
			HBOX_ERROR("InvalidDescriptionException: " << string(errMsg));
		}
	}	
}
			
void upnp_server::delEmbeddedDevice(const event& temp) {
}

void upnp_server::restartEmbeddedDevice(const event& temp) {
	server->stop();
	server->resetListeners();
	((Device* )server)->start();
}

void upnp_server::actionResponseReceived(const event& temp) {
	HBOX_DEBUG("Response for the previous action is: " << temp.getDescription());
	
	vector<string> action_response_related_data;
	boost::split(action_response_related_data, temp.getDescription(), boost::is_any_of("|"));
	
	string actionResponseReceivedUpnpUDN = action_response_related_data[0];

	//Bug: #693033. Here we check the UDN of the remote media server and replace	the address with the IP:port or HIT:port of the local hbox	with the forwarding already in place
	//This would be essentially advertizing the whole network path	to the media rendrer.
	
	HBOX_DEBUG("Before acquiring the lock");
	lock_guard<mutex> lock(shared_mutex);
	HBOX_DEBUG("Acquire the lock successful");
	
	DeviceList *childDeviceList = server->getDeviceList();
	int numChild = childDeviceList->size();
	HBOX_DEBUG("Number of child devices: " << numChild);	
	for (int i = 0; i < numChild; i++) {
		Device *childDevice = childDeviceList->getDevice(i);
		
		if(actionResponseReceivedUpnpUDN == string(childDevice->getUDN())) {
			HBOX_DEBUG("Local device and action requested by the remote Hbox for the response found with name: " << actionResponseReceivedUpnpUDN << " and " << action_response_related_data[1].c_str());

			if(action_response_related_data[2] == "false") {
				HBOX_DEBUG("Return was false ...");
				
				HBOX_DEBUG("Prepare to signal the condition variable");
				shared_condition_variable.notify_one();
				HBOX_DEBUG("Condition variable is signaled");
				return;
			}

			Action* action = childDevice->getAction(action_response_related_data[1].c_str());
			// TODO: check if the action is null
			for (int j = 3; j < action_response_related_data.size(); j+=2)
				action->setArgumentValue(action_response_related_data[j].c_str(), action_response_related_data[j+1].c_str());
			
			HBOX_DEBUG("Prepare to signal the condition variable");
			shared_condition_variable.notify_one();
			HBOX_DEBUG("Condition variable is signaled");
			return;
		}
	}
}
	
void upnp_server::search_and_replace(string &str, const string &oldsubstr, const string &newsubstr) {
	string::size_type startidx = str.find(oldsubstr);
	while (startidx != string::npos) {
		str.replace(startidx, oldsubstr.size(), newsubstr);
		startidx = str.find(oldsubstr);
	}
}
