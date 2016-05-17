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

#include "hboxinfo.hh"
#include "hbox.hh"

using namespace std;

/**
 * Constructor of hboxinfo class
 *
 */
hbox_info::hbox_info() {
	STATE = "INIT";
}

/**
 * Destructor of hboxinfo class
 * @brief all pointers to upnp devices of this hbox will be removed
 *
 */
hbox_info::~hbox_info() {
	for (list<upnp_device*>::iterator it = localUpnpDevices.begin(); it != localUpnpDevices.end(); it++)	delete (*it);		
	localUpnpDevices.clear();
}

void hbox_info::addUpnpDevice(upnp_device* device) {
	localUpnpDevices.insert(localUpnpDevices.begin(), device);
}

/**
 * This method removes an individual upnp device from an hbox. It happens when deviceRemoved() is signaled.
 * @param deviceUDN the UDN of the removed device
 * @return indicate if the device was in the list and be removed or not
 *
 */
bool hbox_info::removeUpnpDevice(string deviceUDN) {
	list<upnp_device*>::iterator it;
	for (it = localUpnpDevices.begin(); it != localUpnpDevices.end(); it++)
		if ((*it)->getDeviceName() == deviceUDN) 
			break;
	
	if (it != localUpnpDevices.end())
	{
		delete (*it);
		localUpnpDevices.erase(it);
		return true;
	}
	
	return false;
}

/**
 * This method find an individual upnp device from an hbox. 
 * @param deviceUDN the UDN of the upnp device
 * @return a pointer to an upnp device, NULL if not found
 *
 */
upnp_device* hbox_info::findUpnpDevice(string deviceUDN) {
	list<upnp_device*>::iterator it;
	for (it = localUpnpDevices.begin(); it != localUpnpDevices.end(); it++)
		if ((*it)->getDeviceName() == deviceUDN) 
			return (*it);
	return NULL;
}

/**
 * Constructor of comminfo class
 *
 */
communication_info::communication_info() { 
	isBehindNAT = false;
	isBehindFirewall = false;
	isHipRelay = false;
	isHip = false;
	isTeredo = false;
	capability = 0;
}

/**
 * Constructor of comminfo class
 * @param communication_str the text representation of the communication information
 *
 */
communication_info::communication_info(const string& comm) {
	size_t pos = 0, pos_1, pos_2;

	pos = comm.find("ipAddress=", pos);
	if (pos != string::npos)
	{
		pos_1 = pos;
		pos = comm.find(";", pos);
		if (pos != string::npos) { 
			pos_2 = pos; 
			ipAddress = comm.substr(pos_1 + 10, pos_2 - pos_1 - 10);
		}
	}
	
	pos = comm.find("hipAddress=", pos);
	if (pos != string::npos)
	{
		pos_1 = pos;
		pos = comm.find(";", pos);
		if (pos != string::npos) { 
			pos_2 = pos; 
			hipAddress = comm.substr(pos_1 + 11, pos_2 - pos_1 - 11);
		}
	}
	
	if (hipAddress.length() > 0) isHip = true;
	else isHip = false;
}

/**
 * Copy constructor of comminfo class
 * @param other another communication information
 *
 */
communication_info::communication_info(const communication_info& other) {
	ipAddress = string(other.ipAddress);
	hipAddress = string(other.hipAddress);
	hipRelayAddress = string(other.hipRelayAddress);
	isBehindNAT = other.isBehindNAT;
	isBehindFirewall = other.isBehindFirewall;
	isHipRelay = other.isHipRelay;
	if (hipAddress.length() > 0) isHip = true;
	else isHip = false;
	isTeredo = other.isTeredo;
	capability = other.capability;
}

/**
 * The assignment operator overloading of comminfo class
 * @param other another communication information
 *
 */
communication_info& communication_info::operator=(const communication_info& other) {
	ipAddress = string(other.ipAddress);
	hipAddress = string(other.hipAddress);
	hipRelayAddress = string(other.hipRelayAddress);
	isBehindNAT = other.isBehindNAT;
	isBehindFirewall = other.isBehindFirewall;
	isHipRelay = other.isHipRelay;
	if (hipAddress.length() > 0) isHip = true;
	else isHip = false;
	isTeredo = other.isTeredo;
	capability = other.capability;
	
	return *this;
}

string communication_info::toString() {
	string temp = "comminfo;";
	if (isHipRelay) { temp += "ipAddress="; temp += hipRelayAddress; temp += ";"; }
	else { temp += "ipAddress="; temp += ipAddress; temp += ";"; }
		
	temp += "hipAddress="; temp += hipAddress; temp += ";";
	return temp;
}

/**
 * Adds HIP association between the local and the remote hbox. Currently this method takes care of the case when only one of the hbox is behind NAT. By this we mean to say that one hbox is able to ping the other hbox using ipv4 but not the other way round. This would be soon replaced by the full relay server association in which both the hboxes can be behind NAT.
 * @param remote the communication information of the remote hbox
 * @return Boolean true if the association was successfull, false otherwise.
 *
 */
bool communication_info::associateHip(communication_info& remote) {
	if (remote.getHipAddress().size() == 0 || remote.getIpAddress().size() == 0) {
		HBOX_DEBUG("Not enough information for HIP association");
		return false;
	}

	if (system(NULL)) HBOX_DEBUG("System command is available");
	else {HBOX_ERROR("System command is not available"); return false;}

	// Adding the map
	std::string command = "hipconf daemon add map " + remote.getHipAddress() + " " + remote.getIpAddress();
	if(system (command.c_str()) == 0)
		HBOX_DEBUG("System command: Successfully executed HIP mapping.");
	else
		HBOX_ERROR("System command: Problem in execution at HIP mapping.");

	// Finalizing the association by sending the ping6 packets
	command = "ping6 -c1 -w 1 " + remote.getHipAddress();
	for(int i=0; i<5; ++i) {
		int status = system (command.c_str());
		if(status == -1){
			HBOX_ERROR("System command: Problem in execution at HIP association.");
			return false;
		}
		else if(status == 0){
			HBOX_DEBUG("System command: Successfully executed HIP association");
			remote.setLsiAddress(findLsi(remote.getHipAddress()));
			return true;
		}
	}
	HBOX_ERROR("Problems in finalizing the association.");
	return false;
}

/**
 * This method retrieves all the communication capabilities of the system
 *
 */
void communication_info::init() {
	// get IP address
	ipAddress = findIpAddress();
	
	// get HIP address
	hipAddress = findHit();
	hipRelayAddress = findRelayAddress(); 
	if (hipAddress.length() > 0) isHip = true;
	if (hipRelayAddress.length() > 0) isHipRelay = true;
	
}

/**
 * This method return the capability level of the hbox system
 * @return the capability level
 *
 */
int communication_info::getCapability() {
	return capability;
}

/**
 * Finds IP address of local hbox. TODO: should use getifaddrs()
 * @return the IP address based on route and ifconfig commands
 * @author Amit
 * @author Matti
 *
 */
string communication_info::findIpAddress() {
	// Find the active device interface
	FILE *route = popen("route -n", "r");
	char s[1000];
	while (fgets(s, 1000, route) != NULL); // last line

	int i = 0; int j = 0;
	while (s[i++]!= '\0'){
		if(s[i] == ' ')
			j = i+1; // begining of new word
	}
	pclose(route);

	char ifconfig[30] = "ifconfig ";
	strncat(ifconfig, &s[j], (i-j-1));

	// Find the ip of this device interface
	FILE *f = popen(ifconfig, "r");
	char * str = fgets(s,1000,f);
	str = fgets(s,1000,f);
	string st=s;
	string raw_ip=st.substr(20,15);
	std::vector<std::string> filtered_ip;
	boost::split(filtered_ip, raw_ip, boost::is_any_of(" "));

	std::string ourIP = filtered_ip[0];
	pclose(f);
	HBOX_DEBUG("IP: " << ourIP);
	return ourIP;
}

/**
 * Finds HIT of local hbox.
 * @return the HIT of the local hbox by reading the hip config file
 * @author Amit
 * @author Matti
 *
 */
string communication_info::findHit() {
	FILE *f = fopen("/etc/hip/hip_cert.cnf","r");
	if(!f)
		f = fopen("/usr/local/etc/hip/hip_cert.cnf","r");

	if(!f) {
		HBOX_ERROR("HIT: <file not found>");
		return "";
	}

	char s[100];
	while(!feof(f)) {
		char * str = fgets(s,100,f);

		char *t=strstr(s,"issuerhit");
		if(t && s[0] != '#') {
			char hit[50];
			strcpy(hit,&t[12]);
			hit[39] = '\0';
			fclose(f);
			HBOX_DEBUG("HIT: " << hit);
			return string(hit);
		}
	}
	HBOX_ERROR("HIT: <hit not found>");
	return "";
}

/**
 * Gets the IP Address of the HIP relay server
 * @return String containing the IP address of the relay
 * @author Vu Ba Tien Dung
 */
string communication_info::findRelayAddress() {
	if (system(NULL)) HBOX_DEBUG("System command is available");
	else {HBOX_ERROR("System command is not available"); return "";}

	// Getting and reading the associations
	const char* command = "hipconf";
	const char* const command_args[] = {
			"hipconf",
			"daemon", 
			"get",
			"ha",
			"all",
			NULL
	};

	int err[2];	int pid; int rc; int status;

	rc = pipe(err);
	if (rc<0){
		close(err[0]);
		close(err[1]);
	}

	pid = fork();
	if (pid > 0) { // parent
		close(err[1]);
		std::string line;
		bool relayFound = false;
		char c;
		while(read(err[0], &c, 1) != 0){
			line.append(1, c);
			if(c == '\n'){
				HBOX_DEBUG("read line from the pipe: " << line);
				if (relayFound || (line.find("HA is I2-SENT", 0) != std::string::npos)) {
					if (!relayFound) { relayFound = true; line = ""; continue; }
				}
				else { line = ""; continue; }
									
				size_t found = line.find("Peer  IP: ");
				if(found != std::string::npos){
					close(0); close(1); close(2); close(err[0]);
					rc = waitpid(pid, &status, 0);
					HBOX_DEBUG("Relay IP: " << line.substr(found + std::string("Peer  IP: ").length()));
					return line.substr(found + std::string("Peer  IP: ").length());
				}
				line = "";
			}
		}
		close(0); close(1); close(2); close(err[0]);
		rc = waitpid(pid, &status, 0);
		return "";
	} else if (pid == 0) { // child
		close(err[0]);
		close(2);
		status = dup(err[1]);

		execvp(command, (char **)command_args);
		exit(1);
	} else{
		close(err[0]);
		close(err[1]);
		return "";
	}
}

/**
 * Gets the LSI corresponding to a HIT.
 * @param remoteHit HIT of the remote hbox of which the LSI is returned.
 * @return String containing the LSI.
 *
 */
string communication_info::findLsi(string remoteHit){
	if (system(NULL)) HBOX_DEBUG("System command is available");
	else {HBOX_ERROR("System command is not available"); return "";}

	// Getting and reading the associations
	const char* command = "hipconf";
	const char* const command_args[] = {
			"hipconf",
			"daemon", 
			"get",
			"ha",
			remoteHit.c_str(),
			NULL
	};

	int err[2];	int pid; int rc; int status;

	rc = pipe(err);
	if (rc<0){
		close(err[0]);
		close(err[1]);
	}

	pid = fork();
	if (pid > 0) { // parent
		close(err[1]);
		std::string line;
		char c;
		while(read(err[0], &c, 1) != 0){
			line.append(1, c);
			if(c == '\n'){
				HBOX_DEBUG("read line from the pipe: " << line);
				size_t found = line.find("Peer  LSI: ", 0);
				if(found != std::string::npos){
					close(0); close(1); close(2); close(err[0]);
					rc = waitpid(pid, &status, 0);
					return line.substr(found + std::string("Peer  LSI: ").length());
				}
				line = "";
			}
		}
		close(0); close(1); close(2); close(err[0]);
		rc = waitpid(pid, &status, 0);
		return "";
	} else if (pid == 0) { // child
		close(err[0]);
		close(2);
		status = dup(err[1]);

		execvp(command, (char **)command_args);
		exit(1);
	} else{
		close(err[0]);
		close(err[1]);
		return "";
	}
}



