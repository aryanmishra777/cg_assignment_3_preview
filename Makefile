CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
INCLUDES = -Iinclude -Isrc
LDFLAGS = -lGL -lGLEW -lglfw -ldl

# List of source files
SOURCES = $(wildcard src/*.cpp) \
          $(wildcard src/mesh/*.cpp) \
          $(wildcard src/rasterization/*.cpp) \
          $(wildcard src/scan_conversion/*.cpp) \
          $(wildcard src/raytracing/*.cpp) \
          $(wildcard src/ui/*.cpp) \
		  $(wildcard imgui/*.cpp) \
		  $(wildcard imgui/backends/*.cpp) \

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Binary name
TARGET = final_countdown

# Rules
all: dirs $(TARGET)

dirs:
	@mkdir -p bin

$(TARGET): $(OBJECTS)
	$(CXX) $^ -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean dirs