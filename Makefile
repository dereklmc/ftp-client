#--------------------------------------------------------------------------------------------------
# Paths
BIN = .

#--------------------------------------------------------------------------------------------------
# Build Tools
CXX = g++

#--------------------------------------------------------------------------------------------------
# Build Flags
CFLAGS = -c -Wall -fstack-check -g
LINKFLAGS = -g

#--------------------------------------------------------------------------------------------------
# Object Files

FTPOBJS = \
	$(BIN)/ftp.o \
	$(BIN)/FTPClient.o \
	$(BIN)/Context.o

COMMONOBJS = \
	$(BIN)/Socket.o

#--------------------------------------------------------------------------------------------------
# Templates

TEMPLATES = Socket.h

#--------------------------------------------------------------------------------------------------
# Interfaces

INTERFACES = Command.h

#--------------------------------------------------------------------------------------------------
# Targets

ftp: $(FTPOBJS) $(COMMONOBJS) $(TEMPLATES) $(INTERFACES)
	$(CXX) $(LINKFLAGS) -o ftp.run $(FTPOBJS) $(COMMONOBJS)

clean:
	rm -f *.o $(BIN)/*.o *.run

#--------------------------------------------------------------------------------------------------
# Source Rules

$(BIN)/ftp.o: ftp.cpp
	$(CXX) $(CFLAGS) ftp.cpp

$(BIN)/Socket.o: Socket.cpp Socket.h
	$(CXX) $(CFLAGS) Socket.cpp

$(BIN)/Context.o: Context.cpp Context.h
	$(CXX) $(CFLAGS) Context.cpp

$(BIN)/FTPClient.o: FTPClient.cpp FTPClient.h Socket.h
	$(CXX) $(CFLAGS) FTPClient.cpp
