#--------------------------------------------------------------------------------------------------
# Paths
BIN = .

#--------------------------------------------------------------------------------------------------
# Build Tools
CXX = g++-4.7

#--------------------------------------------------------------------------------------------------
# Build Flags
CFLAGS = -c -g -Wall -fstack-check

#--------------------------------------------------------------------------------------------------
# Object Files

FTPOBJS = \
	$(BIN)/ftp.o \
	$(BIN)/FTPClient.o \
	$(BIN)/Context.o

COMMONOBJS = \
	$(BIN)/ArgParse.o \
	$(BIN)/CommandParser.o \
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
	$(CXX) -g -o ftp.run $(FTPOBJS) $(COMMONOBJS)

clean:
	rm -f *.o $(BIN)/*.o *.run

#--------------------------------------------------------------------------------------------------
# Source Rules

$(BIN)/ftp.o: ftp.cpp
	$(CXX) $(CFLAGS) ftp.cpp

$(BIN)/Socket.o: Socket.cpp Socket.h
	$(CXX) $(CFLAGS) Socket.cpp

$(BIN)/ArgParse.o: ArgParse.cpp ArgParse.h
	$(CXX) $(CFLAGS) ArgParse.cpp

$(BIN)/CommandParser.o: CommandParser.cpp CommandParser.h Command.h Context.h
	$(CXX) $(CFLAGS) CommandParser.cpp

$(BIN)/Context.o: Context.cpp Context.h
	$(CXX) $(CFLAGS) Context.cpp

$(BIN)/FTPClient.o: FTPClient.cpp FTPClient.h Socket.h
	$(CXX) $(CFLAGS) FTPClient.cpp