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

#ifndef EVENT_HH
#define EVENT_HH

#include <string>
#include <iostream>

using namespace std;

/**
 * @class event
 * @brief A common data structure for interprocess communication
 * @author Vu Ba Tien Dung
 *
 */
class event {
private:
	// When one thread wants another thread invokes an action, it will send a message to the destination thread. As we observe, in the system, there are 4 main threads:
	// XMPP client, UPnP server, UPnP client and main thread. Therefore, the action will be invoked on either an UPnP device or a hbox system.
	// To express all neccessary information, the exchanged message between threads needs to contain the information below:
	string command; 		// which action will be invoked?
	bool upnpInfo;			// is the action related to UPnP device?
	string name;			// what is the name of the device on which the action will be invoked?
	string description;		// detail information of the action
	
public:
	/**
	 * Constructor of event class
	 * By default, an event is not related to UPnP device
	 *
	 */
	event() {
		upnpInfo = false;
	}
	
	/**
	 * Constructor of event class
	 * @param command		the action name
	 * @param upnpInfo		is the command related to UPnP device
	 * @param name			the name of UPnP device
	 * @param description	the action description
	 *
	 */
	event(string command, bool upnpInfo, string name, string description) : command(command), upnpInfo(upnpInfo), name(name), description(description) {}
	
	/**
	 * Getter of command field
	 * @return action name
	 *
	 */
	const string& getCommand() const { return command; }

	/**
	 * Setter of command field
	 * @param command the action name
	 *
	 */
	void setCommand(const string& command) { this->command = command; }

	/**
	 * Getter of upnpInfo field
	 * @return is the action related to UPnP device
	 *
	 */
	bool isUpnpInfo() const { return upnpInfo; }

	/**
	 * Getter of upnpInfo field
	 * @return is the action related to hbox device
	 *
	 */
	bool isHboxInfo() const { return !upnpInfo; }

	/**
	 * Setter of upnpInfo field
	 * @param upnpInfo whether the command is related to UPnP device
	 *
	 */
	void setUpnpInfo(bool upnpInfo) { this->upnpInfo = upnpInfo; }

	/**
	 * Setter of upnpInfo field
	 * @param hboxInfo whether the action is related to hbox device
	 *
	 */
	void setHboxInfo(bool hboxInfo) { this->upnpInfo = !hboxInfo; }

	/**
	 * Getter of name field
	 * @return the device on which the action will be invoked
	 *
	 */
	const string& getName() const { return name; }

	/**
	 * Setter of name field
	 * @param name the name of the device on which the action will be invoked
	 *
	 */
	void setName(const string& name) { this->name = name; }

	/**
	 * Getter of description field
	 * @return the description of the action
	 *
	 */
	const string& getDescription() const { return description; }

	/**
	 * Setter of description field
	 * @param description the further detail of the action
	 *
	 */
	void setDescription(const string& description) { this->description = description; }

	/**
	 * Friend overloading of the output operator <<
	 * @param out an output stream
	 * @param an event object
	 * @return the output stream
	 *
	 */
	friend ostream& operator <<(ostream &out, const event& ev) { 
		out << ev.description;
		return out; 
	}	
};

#endif
