CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
TARGET = breakout
SRCDIR = src
SOURCES = $(SRCDIR)/main.cpp $(SRCDIR)/Game.cpp $(SRCDIR)/Config.cpp $(SRCDIR)/EndGame.cpp $(SRCDIR)/Utils.cpp $(SRCDIR)/Brick.cpp
OBJECTS = $(SOURCES:.cpp=.o)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

$(SRCDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJECTS)
	rm -rf config endgames

.PHONY: clean