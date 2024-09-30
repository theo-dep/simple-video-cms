CXX = g++
override CXXFLAGS += -std=c++23 -Wall -Wextra -Werror -DCPPHTTPLIB_USE_POLL
override LDFLAGS +=

ANALYZER = clang-tidy

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)

TARGET = server

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp $(wildcard *.h)
	$(CXX) $(CXXFLAGS) -c $< -o $@

analyze: $(SRCS)
	$(ANALYZER) $^ -- $(CXXFLAGS)

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
