CC = gcc
CXX = g++
override CFLAGS += -Wall -Wextra -Werror
override CXXFLAGS += -std=c++23 -Wall -Wextra -Werror -DCPPHTTPLIB_USE_POLL
override LDFLAGS += -static

ANALYZER = clang-tidy

CC_SRCS =  $(wildcard *.c)
CXX_SRCS = $(wildcard *.cpp)
OBJS = $(CXX_SRCS:.cpp=.o) $(CC_SRCS:.c=.o)

TARGET = server

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp $(wildcard *.h)
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c $(wildcard *.h)
	$(CC) $(CFLAGS) -c $< -o $@

analyze: $(CXX_SRCS)
	$(ANALYZER) $^ -- $(CXXFLAGS)

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
