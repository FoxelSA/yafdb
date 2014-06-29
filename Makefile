# Includes
INCLUDES := $(realpath include/)
INCLUDES += $(realpath libgnomonic/src/)

# Sources
SHARED_SOURCES := $(realpath $(filter-out src/blur.cpp src/detect.cpp src/preview.cpp src/test.cpp, $(wildcard src/*.c src/*.cpp)))
LIBGNOMONIC_SOURCES := $(realpath $(wildcard libgnomonic/src/*.c libgnomonic/src/*.cpp))

BLUR_SOURCES := $(realpath src/blur.cpp)
DETECT_SOURCES := $(realpath src/detect.cpp)
PREVIEW_SOURCES := $(realpath src/preview.cpp)
TEST_SOURCES := $(realpath src/test.cpp)

# Objects
SHARED_OBJECTS := $(addsuffix .o, $(basename $(SHARED_SOURCES)))
LIBGNOMONIC_OBJECTS := $(addsuffix .o, $(basename $(LIBGNOMONIC_SOURCES)))

BLUR_OBJECTS := $(addsuffix .o, $(basename $(BLUR_SOURCES)))
DETECT_OBJECTS := $(addsuffix .o, $(basename $(DETECT_SOURCES)))
PREVIEW_OBJECTS := $(addsuffix .o, $(basename $(PREVIEW_SOURCES)))
TEST_OBJECTS := $(addsuffix .o, $(basename $(TEST_SOURCES)))

# Compilation flags
#RELEASEFLAGS := -g -O0
RELEASEFLAGS := -O3
CPPFLAGS += $(foreach dir, $(INCLUDES), -I$(dir))
CFLAGS += -pipe -std=gnu99 -Wall -funsigned-char $(RELEASEFLAGS)
CXXFLAGS += -pipe -std=gnu++11 -Wall -funsigned-char $(RELEASEFLAGS)
LDFLAGS += -pipe
LIBRARIES := -lopencv_core -lopencv_imgproc -lopencv_features2d -lopencv_objdetect \
	-lopencv_highgui -lopencv_contrib -lpthread -lm -lstdc++

# System detection
BASE_DIR := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))/


all: yafdb-blur yafdb-detect yafdb-preview yafdb-test

clean:
	@rm -f yafdb-blur yafdb-detect yafdb-preview yafdb-test
	@rm -f $(SHARED_OBJECTS)
	@rm -f $(LIBGNOMONIC_OBJECTS)
	@rm -f $(BLUR_OBJECTS)
	@rm -f $(DETECT_OBJECTS)
	@rm -f $(PREVIEW_OBJECTS)
	@rm -f $(TEST_OBJECTS)


yafdb-blur: $(SHARED_OBJECTS) $(BLUR_OBJECTS)
	@echo "linking $(subst $(BASE_DIR),,$@)..."
	@$(LINK.o) -o $@ $^ $(LIBRARIES)

yafdb-detect: $(SHARED_OBJECTS) $(LIBGNOMONIC_OBJECTS) $(DETECT_OBJECTS)
	@echo "linking $(subst $(BASE_DIR),,$@)..."
	@$(LINK.o) -o $@ $^ $(LIBRARIES)

yafdb-preview: $(SHARED_OBJECTS) $(PREVIEW_OBJECTS)
	@echo "linking $(subst $(BASE_DIR),,$@)..."
	@$(LINK.o) -o $@ $^ $(LIBRARIES)

yafdb-test: $(SHARED_OBJECTS) $(TEST_OBJECTS)
	@echo "linking $(subst $(BASE_DIR),,$@)..."
	@$(LINK.o) -o $@ $^ $(LIBRARIES)


%.o: %.c
	@echo "compiling $(subst $(BASE_DIR),,$<)..."
	@$(COMPILE.c) -o $@ $<

%.o: %.cpp
	@echo "compiling $(subst $(BASE_DIR),,$<)..."
	@$(COMPILE.cpp) -o $@ $<


.PHONY:	all clean
