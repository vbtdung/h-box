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

#include "upnpclient.hh"
#include "hbox.hh"

using namespace std;

/**
 * Constructor of the upnpclient class
 *
 */
upnp_client::upnp_client() {
	HBOX_DEBUG("Starting Control point");
	addDeviceChangeListener(this);
}

/**
 * Start the control point as upnp client
 *
 */
void upnp_client::run()
{
	bool ret = start();
	
	if (!ret) 
		HBOX_INFO("The upnp client thread is failed to start");
}

/**
 * This method is used to assign the shared communication channel between the hbox and the upnpclient threads
 * @param upnpclient_hbox the message queue from the upnpnclient to the hbox
 *
 */
void upnp_client::setQueue(blocking_queue<event> *_hbox) {
	upnpclient_hbox = _hbox;
}

/**
 * This method is executed whenever the hbox main thread sends some message to upnpclient thread
 * @param msg the message content
 *
 */
void upnp_client::onMessage(const event& msg) {
	HBOX_DEBUG("Received something from the hbox: " << msg);
	
	if (msg.isUpnpInfo()) {
		if (msg.getCommand() == "ACTION")
			invokeAction(msg);
	}
}

void upnp_client::invokeAction(const event& temp) {	
	vector<string> action_related_data;
	boost::split(action_related_data, temp.getDescription(), boost::is_any_of("|"));
	HBOX_DEBUG(action_related_data.size());
	
	string actionReceivedUpnpUDN = action_related_data[0];

	DeviceList* deviceList = this->getDeviceList();
	
	for (int i = 0; i < deviceList->size(); i++) {
		Device* dev = deviceList->getDevice(i);
		string deviceUDN = string(dev->getUDN());
		HBOX_DEBUG("Local device " << i << " has UDN " << deviceUDN);
		
		if (actionReceivedUpnpUDN == deviceUDN) {
			HBOX_DEBUG("Local device requested by the remote Hbox found with name: " << action_related_data[1]);
			Action* action = dev->getAction(action_related_data[1].c_str());
			// TODO: check if the action is null

			string actionResponse = action->getService()->getDevice()->getUDN();
			actionResponse += "|";
			actionResponse += action->getName();

			for (int j = 2; j < action_related_data.size(); j+=2) {
				action->setArgumentValue(action_related_data[j].c_str(), action_related_data[j+1].c_str());
			}
			
			if (action->postControlAction()) {
				actionResponse += "|true";
				
				ArgumentList *outArgList = action->getOutputArgumentList();
				int nOutArg = outArgList->size();
				HBOX_DEBUG("Action successfully executed with number of output arguments: " << nOutArg);

				for (int k = 0; k < nOutArg; k++){
					actionResponse += "|";
					actionResponse += outArgList->getArgument(k)->getName();
					actionResponse += "|";
					actionResponse += outArgList->getArgument(k)->getValue();
				}
				
				event ev = event("ACTION_RESPONSE", true, temp.getName(), actionResponse);
				(*upnpclient_hbox).push(ev);
			} 
			else {
				HBOX_DEBUG("Problem in executing action received from the remote HBOX");
				actionResponse += "|false";

				event ev = event("ACTION_RESPONSE", true, temp.getName(), actionResponse);
				(*upnpclient_hbox).push(ev);
			}
	
			break;
		}
	}
}

/**
 * This function is called whenever the control point receives an SSDP alive msg from a new upnp device in the network. This function is implemented fully for informing remote hbox about the newly added devices along with the accessories of the device but the only problem is that new device object are not created in the remote hbox. This is probably beacause of some bug in the cybergarage library. So this method almost commented. These comments can be removed when the bug in the cybergarage is handled.
 * @param dev Device which gets added
 *
 */	 
void upnp_client::deviceAdded(Device *dev) {
	string deviceFriendlyName = dev->getFriendlyName();
	string deviceUDN = string(dev->getUDN());
	HBOX_DEBUG("Added local device: " << deviceFriendlyName << "with UDN: " << deviceUDN);
	
	if (dev->isRootDevice() && deviceFriendlyName != "HBOX Device" && deviceFriendlyName.find("BubbleUPNP") == string::npos)
	{
		int size = rootDevices.size();
		rootDevices.insert(deviceUDN);
		if (size != rootDevices.size()) {
			HBOX_DEBUG("Sent device: " << deviceUDN);
			string deviceDescription = getHttpContent(dev);
			
			event ev;
			if (isMediaServer(dev))
			{
				ev = event("NEW_MEDIA", true, deviceUDN, deviceDescription);
				(*upnpclient_hbox).push(ev);
				
				string netInfo = findMediaServerNetInfo(dev); // MediaServerIP|MediaServerPort
				ev = event("PORT", true, deviceUDN, netInfo);
				(*upnpclient_hbox).push(ev);
			}
			else
			{
				ev = event("NEW", true, deviceUDN, deviceDescription);
				(*upnpclient_hbox).push(ev);
			}
			sleep(1);
		
			list<upnp_service> services = collectServices(dev);
			for (list<upnp_service>::const_iterator it = services.begin(); it != services.end(); it++)
			{
				event ev = event("SERVICE", false, (*it).getServiceName(), (*it).getServiceDescription());
				(*upnpclient_hbox).push(ev);
				sleep(1);
			}
			
			ev = event("START", true, deviceUDN, deviceUDN);
			(*upnpclient_hbox).push(ev);
			sleep(1);
		}
	}
}

/**
 * This function is called when a device leaves the network. This function sends the DEVICE_REMOVED message to all the remote hboxes. This message contains the details of the device and is send over xmpp.
 * @param dev Device which getrs removed in the local network.
 *
 */
void upnp_client::deviceRemoved(Device *dev) {
	string deviceFriendlyName = dev->getFriendlyName();
	string deviceUDN = string(dev->getUDN());
	HBOX_DEBUG("Removed local device: " << deviceFriendlyName << "with UDN: " << deviceUDN);
	
	if (dev->isRootDevice() && deviceFriendlyName != "HBOX Device")
		if (rootDevices.find(deviceUDN) != rootDevices.end())
		{
			rootDevices.erase(deviceUDN);
			event ev = event("DEL", true, deviceUDN, deviceUDN);
			(*upnpclient_hbox).push(ev);
			sleep(1);
		}
}

/**
 * This function is called to get the content of description file of a device. These description file are later send to the remote hboxes after some editing.
 * @param dev Device of which description file is fetched.
 *
 */
string upnp_client::getHttpContent(Device *dev) { 
	CyberNet::URL url(dev->getLocation());
	HTTPRequest httpReq;
	httpReq.setMethod(HTTP::GET);
	httpReq.setURI(url.getPath());
	HTTPResponse *httpRes = httpReq.post(url.getHost(), url.getPort());
	if (httpRes->isSuccessful() == true)
		return httpRes->getContent(); // device description
	else return "";
}

/**
 * This function checks if the device is a Media server. This fuction is used since we have special connection for the content serverd by the media server device.
 * @param dev Device which is being cheked if it is of type media server.
 *
 */
bool upnp_client::isMediaServer(Device *dev) {
	std::vector<std::string> splitDeviceTypeDetails;
	std::string devType =  dev->getDeviceType();
	int pos = devType.find("MediaServer");
	if (pos != string::npos)
	{
		HBOX_DEBUG("Media Server device is found ...");
		return true;
	}
	else return false;
}

string upnp_client::findMediaServerNetInfo(Device *dev) {
	// MediaServerIP|MediaServerPort
	string deviceAddress = dev->getLocation();
	vector<string> splitDeviceAddress;
	boost::split(splitDeviceAddress, deviceAddress, boost::is_any_of("/"));

	vector<string> splitIpPort;
	boost::split(splitIpPort, splitDeviceAddress[2], boost::is_any_of(":"));
	
	string mediaServerIP(splitIpPort[0]);
	string mediaServerPort(splitIpPort[1]);
	
	return mediaServerIP + "|" + mediaServerPort;
}

/**
 * This function is used to the fetch the description of all the services from a local Upnp device.
 * @param dev Device of which we are collecting the services details
 * @return A list is returned each element of which contains a pair of string. First first field of that pair is the device UDN concatenated with the scpd url. Device UDN is also returned to avoid the ambguitiy of the same scpd url of different devices in the local network. Second field of the pair contains the contents of the service description.
 *
 */
list<upnp_service> upnp_client::collectServices(Device *dev) {
	std::string deviceUDN = dev->getUDN();
	ServiceList *sl = dev->getServiceList();
	list<upnp_service> services; 
	for(int i=0; i < sl->size(); i++){
		CyberNet::URL url(dev->getLocation());
		HTTPRequest httpReq;
		httpReq.setMethod(HTTP::GET);
		const char *scpd_temp = sl->getService(i)->getSCPDURL();

		char scpd[strlen(scpd_temp)+1];
		if(scpd_temp[0]!='/' && scpd_temp[0]!='h' && scpd_temp[1]!='t' && scpd_temp[2]!='t' && scpd_temp[3]!='p' && scpd_temp[4]!=':'){
			scpd[0] = '/';
			strcpy(scpd+1, scpd_temp);
		}
		else
			strcpy(scpd, scpd_temp);

		HBOX_DEBUG("Host URL :" << url.getHost());
		HBOX_DEBUG("Host Port :" << url.getPort());
		HBOX_DEBUG("SCPD URI of the service is :" << (std::string)scpd);
		httpReq.setURI(scpd);
		HTTPResponse *httpRes = httpReq.post(url.getHost(), url.getPort());
		if (httpRes->isSuccessful() == true) {
			const char *contents = httpRes->getContent();
			services.push_back(upnp_service((deviceUDN + "|" + string(scpd)), string(contents)));
		}
		else
			HBOX_DEBUG("Service description of service " << i << " could not be collected");
	}
	return services;
}
