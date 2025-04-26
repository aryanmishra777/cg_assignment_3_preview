# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -I$(SRC_DIR) -I$(IMGUI_DIR)
LDFLAGS = -lGL -lGLEW -lglfw -ldl

# Directories
SRC_DIR = src
IMGUI_DIR = imgui
BUILD_DIR = build
SHADER_DIR = shaders
MODEL_DIR = models

# Source files
SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)
IMGUI_FILES = $(wildcard $(IMGUI_DIR)/*.cpp)
IMGUI_BACKEND_FILES = $(wildcard $(IMGUI_DIR)/backends/*.cpp)

SRC_OBJ_FILES = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRC_FILES))
IMGUI_OBJ_FILES = $(patsubst $(IMGUI_DIR)/%.cpp,$(BUILD_DIR)/imgui_%.o,$(IMGUI_FILES))
IMGUI_BACKEND_OBJ_FILES = $(patsubst $(IMGUI_DIR)/backends/%.cpp,$(BUILD_DIR)/imgui_backends_%.o,$(IMGUI_BACKEND_FILES))

OBJ_FILES = $(SRC_OBJ_FILES) $(IMGUI_OBJ_FILES) $(IMGUI_BACKEND_OBJ_FILES)

# Targets
TARGET = graphics_app

# Rules
.PHONY: all clean

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD_DIR)/imgui_%.o: $(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD_DIR)/imgui_backends_%.o: $(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

run: $(TARGET)
	./$(TARGET)