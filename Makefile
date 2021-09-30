EXE = SDLggerat
OBJ_DIR := obj
BIN_DIR = bin
SRC_DIR := src
SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(subst $(SRC_DIR)/, $(OBJ_DIR)/, $(patsubst %.cpp, %.o, $(SOURCES)))
CPPFLAGS = 
CPPFLAGS += -g -Wall -Wformat
LIBS =

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS += -GL-ldl `sdl2-config --libs`
	CPPFLAGS += `sdl2-config --cflags`
endif

ifeq ($(OS), Windows_NT)
    ECHO_MESSAGE = "MinGW"
    LIBS += -lgdi32 -lmingw32 -lopengl32 -lmingw32 -limm32 `sdl2-config --libs`
    CPPFLAGS += `sdl2-config --cflags`
endif

all: $(EXE)

clean:
	rm -f $(EXE) $(OBJECTS)

$(EXE): $(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $(BIN_DIR)/$(EXE) $(LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $< 
