include $(top_srcdir)/src/Makefile.include

AM_CPPFLAGS += -I$(top_srcdir)/src 
  
bin_PROGRAMS = monitorScanner

MONITOR_SCANNER_H = \
  gen-cpp/MonitorsScaner.h \  
  gen-cpp/monitorsScaner_constants.h \
  gen-cpp/monitorsScaner_types.h \
  server/informationProvider.h 
  
MONITOR_SCANNER_CPP = \
  gen-cpp/MonitorsScaner.cpp \  
  gen-cpp/monitorsScaner_constants.cpp \
  gen-cpp/monitorsScaner_types.cpp \
  server/monitorsScaner_server.cpp \
  server/informationProvider.cpp 

THRIFT_DIR := /usr/local/include/thrift
GEN_DIR := gen-cpp

monitorScanner_CPPFLAGS = $(AM_CPPFLAGS) $(QT_INCLUDES) $(QT_DBUS_INCLUDES)

monitorScanner_SOURCES = -I$(THRIFT_DIR) -I$(GEN_DIR) $(MONITOR_SCANNER_H) $(MONITOR_SCANNER_CPP)  

monitorScanner_LDADD = $(LIBBITCOINQT) $(LIBDIMS_TRACKER) $(LIBBITCOIN_SERVER) $(LIBDIMS_TRACKER)

monitorScanner_LDADD += $(LIBBITCOIN_CLI) $(LIBBITCOIN_COMMON) $(LIBLEVELDB) $(LIBMEMENV) $(BDB_LIBS) $(PROTOBUF_LIBS) \
  $(BOOST_LIBS) $(QT_LIBS) $(QT_DBUS_LIBS)  -lthrift

monitorScanner_LDFLAGS = $(QT_LDFLAGS)

.PHONY: FORCE
.SECONDARY: $(QT_QM)

bitcoinstrings.cpp: FORCE
	$(MAKE) -C $(top_srcdir)/src 

CLEANFILES = $(BUILT_SOURCES) *.gcda *.gcno
