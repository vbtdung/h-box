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

#include "proxyconnection.hh"
#include "hbox.hh"

/** 
 * 
 * 
 * @param io_service 
 */
tcp_connection::tcp_connection(ba::io_service& io_service, string forwardIP, int forwardPort) : io_service_(io_service),
																						forwardIP(forwardIP),
																						forwardPort(forwardPort),
																						csocket_(io_service),
																						ssocket_(io_service),
																						resolver_(io_service),
																						proxy_closed(false),
																						isPersistent(false),
																						isOpened(false) {
}

/** 
 * 
 * 
 */
void tcp_connection::start() {
	async_read(csocket_, ba::buffer(cbuffer), ba::transfer_at_least(1),
			   boost::bind(&tcp_connection::handle_client_read_data,
						   shared_from_this(),
						   ba::placeholders::error,
						   ba::placeholders::bytes_transferred));
}

/** 
 * 
 * 
 * @param err 
 * @param len 
 */
void tcp_connection::handle_client_read_data(const bs::error_code& err, size_t len) {
	if(!err) 
	{
	    HBOX_DEBUG("Read something from client, with len: " << len);
		start_connect(len);
	}
	else 
		shutdown();	
}

/** 
 * 
 * 
 * @param err 
 * @param len 
 */
void tcp_connection::handle_server_write(const bs::error_code& err, size_t len) {
	if(!err) {
	    HBOX_DEBUG("Successfully write to the server, with len: " << len);
		async_read(ssocket_, ba::buffer(sbuffer), ba::transfer_at_least(1),
				   boost::bind(&tcp_connection::handle_server_read_data,
							   shared_from_this(),
							   ba::placeholders::error,
							   ba::placeholders::bytes_transferred));
	}
	else {
		shutdown();
	}
}

/** 
 * 
 * 
 * @param err 
 * @param len 
 */
void tcp_connection::handle_server_read_data(const bs::error_code& err, size_t len) {
	if(!err || err == ba::error::eof) {
		if(err == ba::error::eof)
			proxy_closed = true;
		ba::async_write(csocket_, ba::buffer(sbuffer,len),
						boost::bind(&tcp_connection::handle_client_write,
									shared_from_this(),
									ba::placeholders::error,
									ba::placeholders::bytes_transferred));
	} 
	else {
		shutdown();
	}
}

/** 
 * 
 * 
 * @param err 
 * @param len 
 */
void tcp_connection::handle_client_write(const bs::error_code& err, size_t len) {
	if(!err) {
		if(!proxy_closed)
			async_read(ssocket_, ba::buffer(sbuffer,len), ba::transfer_at_least(1),
					   boost::bind(&tcp_connection::handle_server_read_data,
								   shared_from_this(),
								   ba::placeholders::error,
								   ba::placeholders::bytes_transferred));
		else {
 			if(isPersistent && !proxy_closed) {  				
  				start();
 			}
		}
	} 
	else {
		shutdown();
	}
}

void tcp_connection::shutdown() {
	ssocket_.close();
	csocket_.close();
}

/** 
 * 
 * 
 */
void tcp_connection::start_connect(size_t len) {
	std::string server = forwardIP;
	std::string port =  boost::lexical_cast<string>(forwardPort);
	
	if(!isOpened) {
		HBOX_DEBUG("Trying to connect to remote server at: " << server << ":" << port);
		ba::ip::tcp::resolver::query query(server, port);
		resolver_.async_resolve(query, boost::bind(&tcp_connection::handle_resolve, shared_from_this(),
									   boost::asio::placeholders::error,
									   boost::asio::placeholders::iterator,
									   len));
	} 
	else {
	    start_forward_to_server(len);
	}
}

/** 
 * 
 * 
 */
void tcp_connection::start_forward_to_server(size_t len) {
	ba::async_write(ssocket_, ba::buffer(cbuffer,len),
					boost::bind(&tcp_connection::handle_server_write, shared_from_this(),
								ba::placeholders::error,
								ba::placeholders::bytes_transferred));
}

/** 
 * 
 * 
 * @param err 
 * @param endpoint_iterator 
 */
void tcp_connection::handle_resolve(const boost::system::error_code& err,
								ba::ip::tcp::resolver::iterator endpoint_iterator, size_t len) {
    if (!err) {
		ba::ip::tcp::endpoint endpoint = *endpoint_iterator;
		ssocket_.async_connect(endpoint,
							  boost::bind(&tcp_connection::handle_connect, shared_from_this(),
										  boost::asio::placeholders::error,
										  ++endpoint_iterator,
										  len));
    }
    else {
		shutdown();
	}
}

/** 
 * 
 * 
 * @param err 
 * @param endpoint_iterator 
 */
void tcp_connection::handle_connect(const boost::system::error_code& err,
								ba::ip::tcp::resolver::iterator endpoint_iterator, size_t len) {
    if (!err) {
        HBOX_DEBUG("Successfully open the connection to remote server");
		isOpened = true;
		start_forward_to_server(len);
    } 
    else if (endpoint_iterator != ba::ip::tcp::resolver::iterator()) {
		ssocket_.close();
		ba::ip::tcp::endpoint endpoint = *endpoint_iterator;
		ssocket_.async_connect(endpoint,
							  boost::bind(&tcp_connection::handle_connect, shared_from_this(),
										  boost::asio::placeholders::error,
										  ++endpoint_iterator,
										  len));
    } 
    else {
		shutdown();
	}
}
