CXX = g++
CXXFLAGS = -std=c++23 -static -O2 -Wall -Wextra -Werror -I/sources -DCPPHTTPLIB_USE_POLL
override LDFLAGS +=

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)

TARGET = server

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
