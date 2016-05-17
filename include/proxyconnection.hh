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

#ifndef PROXYCONNECTION_HH
#define PROXYCONNECTION_HH

#include <deque>
#include <string>
#include <map>

#include <boost/lexical_cast.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/array.hpp>

using namespace std;

namespace ba=boost::asio;
namespace bs=boost::system;


/**
 * @class tcp_connection
 * @brief TCP connection implements an asynchronous socket of Boost::ASIO library.
 * @author Vu Ba Tien Dung
 *
 */
class tcp_connection : public boost::enable_shared_from_this<tcp_connection> {
public:
	typedef boost::shared_ptr<tcp_connection> pointer;

	static pointer create(ba::io_service& io_service, string forwardIP, int forwardPort) {
		return pointer(new tcp_connection(io_service, forwardIP, forwardPort));
	}

	ba::ip::tcp::socket& socket() {
		return csocket_;
	}

	void start();

private:
	tcp_connection(ba::io_service& io_service, string forwardIP, int forwardPort);
	void shutdown();
	void start_connect(size_t len);
	
	void handle_client_write(const bs::error_code& err, size_t len);
	void handle_client_read_data(const bs::error_code& err, size_t len);
	void handle_server_write(const bs::error_code& err, size_t len);
	void handle_server_read_data(const bs::error_code& err, size_t len);
	void start_forward_to_server(size_t len);
	void handle_resolve(const boost::system::error_code& err,
									ba::ip::tcp::resolver::iterator endpoint_iterator, size_t len);
	void handle_connect(const boost::system::error_code& err,
									ba::ip::tcp::resolver::iterator endpoint_iterator, size_t len);

	ba::io_service& io_service_;
	ba::ip::tcp::socket csocket_;	// socket to client
	ba::ip::tcp::socket ssocket_;	// socket to remote server
	ba::ip::tcp::resolver resolver_;
	bool proxy_closed;
	bool isOpened;
	bool isPersistent;
	int forwardPort;
	string forwardIP;

	boost::array<char, 8192> cbuffer;
	boost::array<char, 8192> sbuffer;
};

#endif
