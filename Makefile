CXX := g++
CC := gcc
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wpedantic
CFLAGS := -O2 -Wall -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION
SRC_DIR := src
LIB_DIR := lib
OBJ_DIR := build/obj
BIN_DIR := build/bin
TARGET := $(BIN_DIR)/rogue_depths

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))
SQLITE_OBJ := $(OBJ_DIR)/sqlite3.o
INCLUDES := -I$(SRC_DIR) -I$(LIB_DIR)

.PHONY: all run clean dirs

all: dirs $(TARGET)

dirs:
	mkdir -p $(OBJ_DIR) $(BIN_DIR) assets/ascii saves config

$(TARGET): $(OBJS) $(SQLITE_OBJ)
	$(CXX) $(CXXFLAGS) $(OBJS) $(SQLITE_OBJ) -o $@ -lpthread -ldl

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(SQLITE_OBJ): $(LIB_DIR)/sqlite3.c
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	$(TARGET)

clean:
	$(RM) -r $(OBJ_DIR) $(BIN_DIR)

