CXX = c++
CXXFLAGS = -Wall -Wextra -std=c++17
TARGET = traceroute
SRC = main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@


clean:
	rm -f $(OBJ)

distclean: clean
	rm -f $(TARGET)