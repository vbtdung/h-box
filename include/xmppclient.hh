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

#ifndef XMPPCLIENT_HH
#define XMPPCLIENT_HH

#include <iostream>
#include <sstream>
#include <string>
#include <list>
#include <mutex>

#include <gloox/gloox.h>
#include <gloox/client.h>
#include <gloox/messagehandler.h>
#include <gloox/messagesessionhandler.h>
#include <gloox/messageeventhandler.h>
#include <gloox/connectionlistener.h>
#include <gloox/chatstatehandler.h>
#include <gloox/messageeventfilter.h>
#include <gloox/chatstatefilter.h>
#include <gloox/rostermanager.h>
#include <gloox/disco.h>
#include <gloox/pubsubresulthandler.h>
#include <gloox/pubsubmanager.h>
#include <gloox/error.h>
#include <gloox/pubsub.h>
#include <gloox/pubsubevent.h>
#include <gloox/pubsubitem.h>
#include <gloox/jid.h>
#include <gloox/clientbase.h>
#include <gloox/dataform.h>
#include <gloox/presence.h>
#include <gloox/message.h>

#include "threadsafe_queue.hh"
#include "event.hh"
#include "hboxinfo.hh"

using namespace gloox;
using namespace std;

const int EXT_HBOX_STANZA = 100;
const int EXT_HIPL= 200;
const string XMLNS_HBOX = "http://www.cse.tkk.fi/Datacommunications/Research";
const string XMLNS_HIPL = "http://infrahip.hiit.fi";

/**
 * @class xmpp_client
 * @brief This class provides a communication interface between the hbox and its associated XMPP server.
 * @author Matti
 * @author Dung
 *
 */
class xmpp_client : public ConnectionListener, public LogHandler, public IqHandler, public RosterListener, public MessageHandler {
private:
	// Client object
	Client* client;
	string username;
	communication_info self_hbox_comm;
	blocking_queue<event> *xmpp_hbox;
	
	mutable mutex roster_mutex, incoming_message_mutex, outgoing_message_mutex;
	
public:
	xmpp_client();
	virtual ~xmpp_client();
 
	// init and start XMPP 
	bool init(const string& username, const string& password, const string& server = "");
	void setQueue(blocking_queue<event> *_hbox);
	void run();
			
	// event listener
	void onMessage(event& msg);
	
	Client* getclient() { return client; }
	string getusername() { return username; }
	
	// overload methods for ConnectionListener
	virtual void onConnect();
	virtual void onDisconnect(ConnectionError e);
	virtual bool onTLSConnect(const CertInfo& info);

	// overload methods for LogHandler
	virtual void handleLog(LogLevel level, LogArea area, const string& message);
	
	// overload methods for IqHandler
	virtual bool handleIq(const IQ& iq );
	virtual void handleIqID(const IQ& iq, int context) {}

	// overload methods for RosterListener
	virtual void handleRoster(const Roster& roster);
	virtual void handleRosterError(const IQ& iq);
	virtual void handleRosterPresence(const RosterItem& item, const string& resource, Presence::PresenceType presence, const string& msg);
	virtual void handleSelfPresence(const RosterItem& item, const string& resource, Presence::PresenceType presence, const string& msg);
	virtual void handleNonrosterPresence(const Presence& presence);
	
	virtual void handleItemAdded(const JID&) {}
	virtual void handleItemSubscribed(const JID&) {}
	virtual void handleItemRemoved(const JID&) {}
	virtual void handleItemUpdated(const JID&) {}
	virtual void handleItemUnsubscribed(const JID&) {}
	virtual bool handleSubscriptionRequest(const JID&, const string&) {}
	virtual bool handleUnsubscriptionRequest(const JID&, const string&) {}
	
	// overload methods for MessageHandler
	void handleMessage(const Message& msg, MessageSession *session=0);
	bool sendMessage(const Message& msg);
		
	// extended stanza errors' meaning
	string StanzaErrorMeaning(int n);
	string PresenceMeaning(int n);

};

#endif
