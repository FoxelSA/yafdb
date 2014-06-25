# Includes
INCLUDES := $(realpath include/)
INCLUDES += $(realpath libgnomonic/src/)

# Sources
SHARED_SOURCES := $(realpath $(filter-out src/blur.cpp src/detect.cpp src/preview.cpp src/test.cpp, $(wildcard src/*.c src/*.cpp)))
LIBGNOMONIC_SOURCES := $(realpath $(wildcard libgnomonic/src/*.c libgnomonic/src/*.cpp))

DETECT_SOURCES := $(realpath src/detect.cpp)
BLUR_SOURCES := $(realpath src/blur.cpp)
PREVIEW_SOURCES := $(realpath src/preview.cpp)
TEST_SOURCES := $(realpath src/test.cpp)

# Objects
SHARED_OBJECTS := $(addsuffix .o, $(basename $(SHARED_SOURCES)))
LIBGNOMONIC_OBJECTS := $(addsuffix .o, $(basename $(LIBGNOMONIC_SOURCES)))

DETECT_OBJECTS := $(addsuffix .o, $(basename $(DETECT_SOURCES)))
BLUR_OBJECTS := $(addsuffix .o, $(basename $(BLUR_SOURCES)))
PREVIEW_OBJECTS := $(addsuffix .o, $(basename $(PREVIEW_SOURCES)))
TEST_OBJECTS := $(addsuffix .o, $(basename $(TEST_SOURCES)))

# Compilation flags
RELEASEFLAGS := -g -O0
#RELEASEFLAGS := -O3
CPPFLAGS += -c -pipe $(foreach dir, $(INCLUDES), -I$(dir))
CFLAGS += -std=gnu99 -Wall -funsigned-char
CXXFLAGS += -std=gnu++11 -Wall -funsigned-char
LDFLAGS += -pipe -lstdc++ -lm -lopencv_core -lopencv_imgproc -lopencv_features2d -lopencv_objdetect \
	-lopencv_highgui -lopencv_contrib

# System detection
BASE_DIR := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))/


all: yafdb-detect yafdb-blur yafdb-test

clean:
	@rm -f yafdb-detect yafdb-blur yafdb-test
	@rm -f $(SHARED_OBJECTS)
	@rm -f $(LIBGNOMONIC_OBJECTS)
	@rm -f $(DETECT_OBJECTS)
	@rm -f $(BLUR_OBJECTS)
	@rm -f $(TEST_OBJECTS)


yafdb-detect: $(SHARED_OBJECTS) $(LIBGNOMONIC_OBJECTS) $(DETECT_OBJECTS)
	@echo "linking $(subst $(BASE_DIR),,$@)..."
	@$(LINK.o) $(RELEASEFLAGS) -o $@ $^

yafdb-blur: $(SHARED_OBJECTS) $(BLUR_OBJECTS)
	@echo "linking $(subst $(BASE_DIR),,$@)..."
	@$(LINK.o) $(RELEASEFLAGS) -o $@ $^

yafdb-preview: $(SHARED_OBJECTS) $(PREVIEW_OBJECTS)
	@echo "linking $(subst $(BASE_DIR),,$@)..."
	@$(LINK.o) $(RELEASEFLAGS) -o $@ $^

yafdb-test: $(SHARED_OBJECTS) $(TEST_OBJECTS)
	@echo "linking $(subst $(BASE_DIR),,$@)..."
	@$(LINK.o) $(RELEASEFLAGS) -o $@ $^


%.o: %.c
	@echo "compiling $(subst $(BASE_DIR),,$<)..."
	@$(COMPILE.c) $(RELEASEFLAGS) -o $@ $<

%.o: %.cpp
	@echo "compiling $(subst $(BASE_DIR),,$<)..."
	@$(COMPILE.cpp) $(RELEASEFLAGS) -o $@ $<


.PHONY:	all clean
