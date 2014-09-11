# Includes
INCLUDES := $(realpath include/)
INCLUDES += $(realpath libgnomonic/src/)
INCLUDES += $(realpath libgnomonic/lib/libinter/src/)

# Sources
SHARED_SOURCES := $(realpath $(shell find src/detectors/ -type f -iname "*.c" -o -iname "*.cpp"))
SHARED_SOURCES += $(realpath $(shell find libgnomonic/src/ -type f -iname "*.c" -o -iname "*.cpp"))
SHARED_SOURCES += $(realpath $(shell find libgnomonic/lib/libinter/src/ -type f -iname "*.c" -o -iname "*.cpp"))

APP_SOURCES += $(realpath $(wildcard src/*.c src/*.cpp))

# Objects
SHARED_OBJECTS := $(addsuffix .o, $(basename $(SHARED_SOURCES)))

APP_OBJECTS := $(addsuffix .o, $(basename $(APP_SOURCES)))

# Programs
APP_BINARIES := $(addprefix yafdb-, $(notdir $(basename $(APP_SOURCES))))

# Compilation flags
#RELEASEFLAGS := -g -O0
RELEASEFLAGS := -O3
CPPFLAGS += $(foreach dir, $(INCLUDES), -I$(dir))
CFLAGS += -pipe -std=gnu99 -Wall -funsigned-char $(RELEASEFLAGS)
CXXFLAGS += -pipe -std=gnu++11 -Wall -funsigned-char $(RELEASEFLAGS)
LDFLAGS += -pipe
LIBRARIES := -lopencv_core -lopencv_imgproc -lopencv_features2d -lopencv_objdetect \
	-lopencv_highgui -lopencv_calib3d -lopencv_contrib -lpthread -lm -lstdc++

# System detection
BASE_DIR := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))/


all: $(APP_BINARIES)

clean:
	@rm -f $(APP_BINARIES)
	@rm -f $(APP_OBJECTS)
	@rm -f $(SHARED_OBJECTS)


$(APP_BINARIES): $(SHARED_OBJECTS) $(APP_OBJECTS)
	@echo "linking $(subst $(BASE_DIR),,$@)..."
	@$(LINK.o) -o $@ $(SHARED_OBJECTS) $(realpath $(addprefix src/, $(addsuffix .o, $(subst yafdb-,,$(notdir $@))))) $(LIBRARIES)


%.o: %.c
	@echo "compiling $(subst $(BASE_DIR),,$<)..."
	@$(COMPILE.c) -o $@ $<

%.o: %.cpp
	@echo "compiling $(subst $(BASE_DIR),,$<)..."
	@$(COMPILE.cpp) -o $@ $<


.PHONY:	all clean
