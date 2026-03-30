CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
SRC_DIR  := src
SRCS     := $(wildcard $(SRC_DIR)/*.cpp)
OBJS     := $(SRCS:.cpp=.o)
TARGET   := simplify

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(SRC_DIR)/*.o $(TARGET)
