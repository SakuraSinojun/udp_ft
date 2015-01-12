

CXX	:= g++
LD	:= g++
AR	:= ar
CXXFLAGS := -Wall -O2 -Werror -g
INCLUDES = -I../ -I../tools/serialize -I../../ministun

TARGET	= libUft.a
UNITEST = a.out
SFTTEST = s.out
# LIBSTUN = ../../ministun/libministun.a

LINKS	= -L.
LIBS	= -lpthread

# SOURCES := main.cpp
# SOURCES += Uft.cpp
SOURCES += UftClient.cpp
SOURCES += UftServer.cpp
SOURCES += UfServerNode.cpp
SOURCES += UfPacket.cpp
# SOURCES += UfBuffer.cpp
# SOURCES += UfTimeout.cpp
SOURCES += UfFile.cpp
SOURCES += UfTime.cpp
SOURCES += UfPartitions.cpp
SOURCES += UfConvert.cpp
SOURCES += UfPieceBuffer.cpp
SOURCES += UfWorkThread.cpp
SOURCES += UfThreadPool.cpp
SOURCES += UfMessenger.cpp
SOURCES += md5.cpp
SOURCES += Sft.cpp
# SOURCES += SfPacket.cpp

SOURCES += ../tools/socket.cpp
SOURCES += ../tools/thread.cpp
SOURCES += ../tools/lock.cpp
SOURCES += ../tools/dump.cpp
SOURCES += ../tools/serialize/archive.cpp
SOURCES += ../tools/logging.cpp
SOURCES += ../tools/timeout.cpp
SOURCES += ../tools/buffer.cpp
SOURCES += ../tools/reliable_udp_socket.cpp

OBJS := $(SOURCES:.cpp=.o)
DEPS := $(SOURCES:.cpp=.d)

all: prebuild $(TARGET) $(UNITEST) $(SFTTEST)

$(TARGET): $(OBJS) $(DEPS)
	@echo Linking $@ ...
	@# $(LD) $(OBJS) $(LINKS) $(LIBS) -o$@
	$(AR) crs $(TARGET) $(OBJS)
	@echo -------------------------------------------
	@echo done.

$(UNITEST): main.o $(TARGET)
	@echo Linking $@ ...
	$(LD) main.o $(TARGET) $(LINKS) $(LIBS) -o$@
	@echo -------------------------------------------
	@echo done.

$(SFTTEST): main_sft.o $(TARGET)
	@echo Linking $@ ...
	$(LD) main_sft.o $(TARGET) $(LINKS) $(LIBS) -o$@
	@echo -------------------------------------------
	@echo done.

.cpp.o:
	@echo Compling $@ ...
	$(CXX) -c $< $(INCLUDES) $(CXXFLAGS)  -o $@
	@echo -------------------------------------------

%.d:%.cpp
	$(CXX) -MM $< $(INCLUDES) $(CXXFLAGS) -o $@

prebuild:
	#echo -------

include $(DEPS)

clean:
	rm -fr $(OBJS) $(DEPS) $(TARGET) $(UNITEST) main.o aaa.tmp *.7252.o *.7252.a main_sft.o



