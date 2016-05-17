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

#ifndef PROXYSERVER_HH
#define PROXYSERVER_HH

#include <deque>
#include <string>

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>

#include "proxyconnection.hh"

using namespace std;

namespace ba=boost::asio;
namespace bs=boost::system;

typedef boost::shared_ptr<ba::ip::tcp::socket> socket_ptr;
typedef boost::shared_ptr<ba::io_service> io_service_ptr;
typedef deque<io_service_ptr> ios_deque;

/**
 * @class tcp_proxy_server
 * @brief TCP proxy server forwards data between one hbox to another. The hbox creates proxy servers to forward data from local UPnP service devices to the remote UPnP renderers.
 * @author Vu Ba Tien Dung
 *
 */
class tcp_proxy_server {
public:
	tcp_proxy_server(const ios_deque& io_services, int listeningPort, string forwardIP, int forwardPort);

private:
	void start_accept();
	void handle_accept(tcp_connection::pointer new_connection, const bs::error_code& error);
	
	ios_deque io_services_;
	ba::ip::tcp::acceptor acceptor_;
	int listeningPort;
	string forwardIP;
	int forwardPort;

};

#endif
