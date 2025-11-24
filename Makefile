CXX = g++
CXXFLAGS = -Wall -pthread -std=c++17

OBJS = cse4001_sync.o

all: cse4001_sync

cse4001_sync: $(OBJS)
	$(CXX) $(CXXFLAGS) -o cse4001_sync $(OBJS)

cse4001_sync.o: cse4001_sync.cpp semaphore_class.h
	$(CXX) $(CXXFLAGS) -c cse4001_sync.cpp

clean:
	rm -f *.o cse4001_sync
