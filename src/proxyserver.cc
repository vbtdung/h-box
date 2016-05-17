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

#include "proxyserver.hh"
#include "hbox.hh"

tcp_proxy_server::tcp_proxy_server(const ios_deque& io_services, int listeningPort, string forwardIP, int forwardPort) : io_services_(io_services),
	  acceptor_(*io_services.front(), ba::ip::tcp::endpoint(ba::ip::tcp::v4(), listeningPort)) {
	this->listeningPort = listeningPort;
	this->forwardIP = forwardIP;
	this->forwardPort = forwardPort;
	
	start_accept();
}

void tcp_proxy_server::start_accept() {
	// Round robin.
	io_services_.push_back(io_services_.front());
	io_services_.pop_front();
	tcp_connection::pointer new_connection = tcp_connection::create(*io_services_.front(), forwardIP, forwardPort);

	acceptor_.async_accept(new_connection->socket(), boost::bind(&tcp_proxy_server::handle_accept, this, new_connection, ba::placeholders::error));
}

void tcp_proxy_server::handle_accept(tcp_connection::pointer new_connection, const bs::error_code& error) {
	if (!error) {
        HBOX_DEBUG("A client connection is created");
		new_connection->start();
		start_accept();
	}
}
