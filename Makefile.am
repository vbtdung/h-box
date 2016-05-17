
bin_PROGRAMS = hbox

hbox_SOURCES = 	src/configfile.cc \
				src/xmppclient.cc \
				src/upnpclient.cc \
				src/upnpserver.cc \
				src/hboxinfo.cc \
				src/proxyserver.cc \
				src/proxyconnection.cc \
				src/hbox.cc 

INCLUDES = -I@top_srcdir@/include

ACLOCAL_AMFLAGS = -I m4

AM_CXXFLAGS = -std=c++0x
AM_CPPFLAGS = $(log4cpp_CFLAGS) $(gloox_CFLAGS) $(cybergarage_CFLAGS) $(BOOST_CPPFLAGS)
AM_LDFLAGS = $(BOOST_LDFLAGS)
 
LIBS += $(log4cpp_LIBS) $(gloox_LIBS) $(cybergarage_LIBS) $(BOOST_FILESYSTEM_LIB) -lboost_thread-mt -lpthread -lclink -lxerces-c -lboost_system -lssl -lboost_system-mt -lcrypto -w


doxygen: Doxyfile
	doxygen $<
