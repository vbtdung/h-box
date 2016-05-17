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

#include "xmppclient.hh"
#include "hbox.hh"

using namespace std;
using namespace gloox;

/**
 * Constructor for xmppclient
 *
 */
xmpp_client::xmpp_client() {
	client = NULL;
}

/**
 * Destructor of xmppclient
 *
 */
xmpp_client::~xmpp_client(){
	delete(client);
}

/**
 * Makes settings for connection to xmpp server
 * @param username Username of the xmpp client
 * @param password Password of the xmpp client
 * @param server Xmpp server of the client
 * @return Boolean true of the initialization was successful, false otherwise
 *
 */
bool xmpp_client::init(const string& username, const string& password, const string& server) {
	if(!username.size()) {
		HBOX_INFO("User name was not supplied");
		return false;
	}
	else
		this->username = username;

	HBOX_INFO("Server : " << server);
	HBOX_INFO("Connecting with username : " << username);

	JID jid(this->username); 
	
	// setting up the client
	client = new Client(jid.full(), password, -1);
	if (!server.empty()) client->setServer(server);
	client->setForceNonSasl(server == "talk.google.com" ? true : false); // While using google talk server, put SASL auth off. While using ejabberd, SASL auth should be on.
	client->disco()->setVersion("hbox", GLOOX_VERSION, "Linux");
	client->disco()->setIdentity("client", "hbox");
	client->disco()->addFeature(XMLNS_CHAT_STATES);

	// register for handlers
	HBOX_DEBUG("Registering for handlers");	
	client->registerConnectionListener(this);
	client->registerMessageHandler(this);
	client->logInstance().registerLogHandler(LogLevelDebug, LogAreaAll, this);
	client->registerIqHandler(this, EXT_HBOX_STANZA);
	client->registerIqHandler(this, EXT_HIPL);
	client->registerMessageHandler(this);
	client->rosterManager()->registerRosterListener(this);

	return true;
}

/**
 * Connects client to xmpp server
 *
 */
void xmpp_client::run() {
	bool status = client->connect(false);
}

/**
 * This method is used to assign the shared communication channel between the hbox and the xmppclient threads
 * @param xmpp_hbox the message queue from the xmppclient to the hbox
 *
 */
void xmpp_client::setQueue(blocking_queue<event> *_hbox) {
	xmpp_hbox = _hbox;
}

/**
 * This method is executed whenever the hbox main thread sends some message to upnpserver thread
 * @param msg the message content
 *
 */
void xmpp_client::onMessage(event& msg) {
	// HBOX_DEBUG("Received something from the hbox: " << msg);
	
	if (msg.isHboxInfo()) {
		if (msg.getCommand() == "NEW")
			sendMessage(/* type */ Message(Message::Normal, /* to */ JID(msg.getName()), /* body */ msg.getDescription(), /* subject */ "HBOX_INFO", /* xmlLang */ ""));
	}
	else {
		if (msg.getCommand() == "NEW")
			sendMessage(/* type */ Message(Message::Normal, /* to */ JID(msg.getName()), /* body */ msg.getDescription(), /* subject */ "NEW_DEVICE", /* xmlLang */ ""));
		else if (msg.getCommand() == "PORT") 			
			sendMessage(/* type */ Message(Message::Normal, /* to */ JID(msg.getName()), /* body */ msg.getDescription(), /* subject */ "DEVICE_PORT", /* xmlLang */ ""));	
		else if (msg.getCommand() == "SERVICE") 
			sendMessage(/* type */ Message(Message::Normal, /* to */ JID(msg.getName()), /* body */ msg.getDescription(), /* subject */ "NEW_SERVICE", /* xmlLang */ ""));
		else if (msg.getCommand() == "START")
			sendMessage(/* type */ Message(Message::Normal, /* to */ JID(msg.getName()), /* body */ msg.getDescription(), /* subject */ "START_DEVICE", /* xmlLang */ ""));
		else if (msg.getCommand() == "DEL") 
			sendMessage(/* type */ Message(Message::Normal, /* to */ JID(msg.getName()), /* body */ msg.getDescription(), /* subject */ "DELETE_DEVICE", /* xmlLang */ ""));
		else if (msg.getCommand() == "ACTION")
			sendMessage(/* type */ Message(Message::Normal, /* to */ JID(msg.getName()), /* body */ msg.getDescription(), /* subject */ "ACTION", /* xmlLang */ ""));
		else if (msg.getCommand() == "ACTION_RESPONSE")
			sendMessage(/* type */ Message(Message::Normal, /* to */ JID(msg.getName()), /* body */ msg.getDescription(), /* subject */ "ACTION_RESPONSE", /* xmlLang */ ""));	
	}
		
}

/**
 * Reimplemented from ConnectionListener
 *
 */
void xmpp_client::onConnect() {
	HBOX_INFO("connected!!!");
}

/**
 * Reimplemented from ConnectionListener
 * @param e ConnectionError object for error details
 *
 */
void xmpp_client::onDisconnect(ConnectionError e) {
	HBOX_INFO("Disconnected " << e);
	if(e == ConnAuthenticationFailed)
		HBOX_DEBUG("Authentication failed. Reason: " << client->authError());
}

/**
 * Reimplemented from ConnectionListener
 * @param info CertInfo object for certificate information
 * @return Boolean true
 *
 */
bool xmpp_client::onTLSConnect(const CertInfo& info){
	time_t from(info.date_from);
	time_t to(info.date_to);

	HBOX_DEBUG("status: " << info.status
			<< "; issuer: " << info.issuer
			<< "; peer: " << info.server
			<< "; protocol: " << info.protocol
			<< "; mac: " << info.mac
			<< "; cipher: " << info.cipher
			<< "; compression: " << info.compression
			<< "; from: " << ::ctime(&from)
			<< "; to: " << ::ctime(&to));

	return true;
}

/**
 * Reimplemented from LogHandler.
 * @param level Loglevel object
 * @param area LogArea object
 * @param message String log message
 *
 */
void xmpp_client::handleLog(LogLevel level, LogArea area, const string& message) {
	HBOX_DEBUG("Received: " << message);
}


/**
 * Reimplemented from RosterListener.
 * @param iq IQ error in the roster.
 * @return Boolean true
 *
 */
bool xmpp_client::handleIq(const gloox::IQ& iq) {
	return true;
}
	
/**
 * Reimplemented from RosterListener.
 * @param roster Roster object arrived.
 *
 */
void xmpp_client::handleRoster(const Roster& roster) {
	HBOX_DEBUG("Roster arriving");
	Roster::const_iterator it = roster.begin();
}

/**
 * Reimplemented from RosterListener.
 * @param iq IQ error in the roster.
 *
 */
void xmpp_client::handleRosterError(const IQ& iq) {
	HBOX_DEBUG("A roster-related error occured");
}

/**
 * Reimplemented from RosterListener, this function is called on every status change of an item in the roster. When the online presence of other hboxes is detected, 'this' hbox sends over it's HIT and device descriptions to them (as xml StanzaExtensions). When the offline presence of a remote hbox is detected then remote hbox along with its child device are removed. This is the only way we could have detected the offline presece of the remote hbox. This method doesn't take care of the individual device add and remove in the remote network. That would be handled in handleMessage() method.
 * TODO: More generic way to check the identity of the counterpart
 * TODO: How about a network of hboxes.
 * @param item RosterItem for which the presence is received.
 * @param resource Resource of the roster item.
 * @param presence PresenceType of the Roster item.
 * @param msg Message if any, corresponding the roster.
 *
 */
void xmpp_client::handleRosterPresence(const RosterItem& item, const string& resource, Presence::PresenceType presence, const string& msg) {
	lock_guard<mutex> lock(roster_mutex);
	HBOX_DEBUG("Received presence from entity which is in the roster: " << PresenceMeaning(presence));
	
	string remoteHboxJID = item.jid();
	string remoteHboxPresenceType = PresenceMeaning(presence);
	
	if (remoteHboxPresenceType == "Available") {
		event ev = event("NEW", false, remoteHboxJID, remoteHboxJID);
		xmpp_hbox->push(ev);
	}
	else {
		event ev = event("DEL", false, remoteHboxJID, remoteHboxJID);
		xmpp_hbox->push(ev);
	}
}

/**
 * Reimplemented from RosterListener.
 * @param presence Presence from a non roster identity.
 *
 */
void xmpp_client::handleNonrosterPresence(const Presence& presence) {
	HBOX_DEBUG("Received presence from entity not in the roster: " << presence.from().full());
}

/**
 * Reimplemented from RosterListener, this function is called on every status change of a JID that matches the Client's own JID. If the presence is of type Unavailable, then the resource has already been removed from the RosterItem.
 * @param item RosterItem for which the presence is received.
 * @param resource Resource of the roster item.
 * @param presence PresenceType of the Roster item.
 * @param msg Message if any, corresponding the roster.
 *
 */
void xmpp_client::handleSelfPresence(const RosterItem& item, const string& resource, Presence::PresenceType presence, const string& msg) {
	HBOX_DEBUG("Self presence received: " << item.jid() << "/" << resource << " presence type: " << PresenceMeaning(presence));
}

/**
 * Reimplemented from MessageHandler, this method is responsible for all the signaling and control message reception between the hbox peers.
 * @param msg Message object received.
 * @param session MessageSession object in which the message is received.
 *
 */
void xmpp_client::handleMessage(const Message& msg, MessageSession *session) {
	lock_guard<mutex> lock(incoming_message_mutex);
	HBOX_DEBUG("Receive message from " << msg.from().bare() << " in thread \"" << msg.thread() << "\" with subject: " << msg.subject());
	
	// TODO: check if the message is from the correct peer
	
	if (msg.subject() == "HBOX_INFO") {
		event ev = event("NEW", false, msg.from().bare(), msg.body());
		xmpp_hbox->push(ev);
	}
	else if (msg.subject() == "NEW_DEVICE") {
		event ev = event("NEW", true, msg.from().bare(), msg.body());
		xmpp_hbox->push(ev);
	}
	else if (msg.subject() == "DEVICE_PORT") {
		event ev = event("PORT", true, msg.from().bare(), msg.body());
		xmpp_hbox->push(ev);
	}
	else if (msg.subject() == "NEW_SERVICE") {
		event ev = event("SERVICE", true, msg.from().bare(), msg.body());
		xmpp_hbox->push(ev);
	}
	else if (msg.subject() == "START_DEVICE") {
		event ev = event("START", true, msg.from().bare(), msg.body());
		xmpp_hbox->push(ev);
	}
	else if (msg.subject() == "DELETE_DEVICE") {
		event ev = event("DEL", true, msg.from().bare(), msg.body());
		xmpp_hbox->push(ev);
	}
	else if (msg.subject() == "ACTION") {
		event ev = event("ACTION", true, msg.from().bare(), msg.body());
		xmpp_hbox->push(ev);
	}
	else if (msg.subject() == "ACTION_RESPONSE") {
		event ev = event("ACTION_RESPONSE", true, msg.from().bare(), msg.body());
		xmpp_hbox->push(ev);
	}
}

/**
 * Sends messages to the peer XMPP client mutually exclusively.
 * @param msg Message object to be send to the peer xmpp client.
 * @return Returns boolean true.
 *
 */
bool xmpp_client::sendMessage(const Message& msg) {
	lock_guard<mutex> lock(outgoing_message_mutex);
	client->send(msg);
	return true;
}

/**
 * Returns a string corresponding to the stanza error number
 * @param n Stanza error number
 * @return String corresponding to the stanza error number
 *
 */
string xmpp_client::StanzaErrorMeaning(int n) {
	switch (n) {
		case 0: return "StanzaErrorBadRequest ";
		case 1: return "StanzaErrorConflict- Node by this name already exists ";
		case 2: return "StanzaErrorFeatureNotImplemented ";
		case 3: return "StanzaErrorForbidden ";
		case 4: return "StanzaErrorGone ";
		case 5: return "StanzaErrorInternalServerError ";
		case 6: return "StanzaErrorItemNotFound ";
		case 7: return "StanzaErrorJidMalformed ";
		case 8: return "StanzaErrorNotAcceptable ";
		case 9: return "StanzaErrorNotAllowed ";
		case 10: return "StanzaErrorNotAuthorized ";
		case 11: return "StanzaErrorNotModified ";
		case 12: return "StanzaErrorPaymentRequired ";
		case 13: return "StanzaErrorRecipientUnavailable ";
		case 14: return "StanzaErrorRedirect ";
		case 15: return "StanzaErrorRegistrationRequired";
		case 16: return "StanzaErrorRemoteServerNotFound ";
		case 17: return "StanzaErrorRemoteServerTimeout ";
		case 18: return "StanzaErrorResourceConstraint";
		case 19: return "StanzaErrorServiceUnavailable ";
		case 20: return "StanzaErrorSubscribtionRequired ";
		case 21: return "StanzaErrorUndefinedCondition ";
		case 22: return "StanzaErrorUnexpectedRequest ";
		case 23: return "associated error type SHOULD be ";
		case 24: return "StanzaErrorUndefined";
	}
}

/**
 * Gets a string corresponding to the status of the XMPP item presence number.
 * @param n Presence number in the roster presence.
 * @return String corresponding to the roster presence number.
 *
 */
string xmpp_client::PresenceMeaning(int n) {
	switch (n) {
		case 0: return "Available";
		case 1: return "Chat";
		case 2: return "Away";
		case 3: return "DND";
		case 4: return "XA";
		case 5: return "Offline";
		case 6: return "Probe";
		case 7: return "Error";
		case 8: return "Invalid";
	}
	return "Something is wrong";
}
