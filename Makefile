# Includes
INCLUDES := $(realpath include/)
INCLUDES += $(realpath libgnomonic/src/)

# Sources
SHARED_SOURCES := $(realpath $(shell find src/detectors/ -type f -iname "*.c" -o -iname "*.cpp"))
SHARED_SOURCES += $(realpath $(shell find libgnomonic/src/ -type f -iname "*.c" -o -iname "*.cpp"))

BLUR_SOURCES := $(SHARED_SOURCES) $(realpath src/blur.cpp)
DETECT_SOURCES := $(SHARED_SOURCES) $(realpath src/detect.cpp)
PREVIEW_SOURCES := $(SHARED_SOURCES) $(realpath src/preview.cpp)
TEST_SOURCES := $(SHARED_SOURCES) $(realpath src/test.cpp)
VALIDATE_SOURCES := $(SHARED_SOURCES) $(realpath src/validate.cpp)

# Objects
BLUR_OBJECTS := $(addsuffix .o, $(basename $(BLUR_SOURCES)))
DETECT_OBJECTS := $(addsuffix .o, $(basename $(DETECT_SOURCES)))
PREVIEW_OBJECTS := $(addsuffix .o, $(basename $(PREVIEW_SOURCES)))
TEST_OBJECTS := $(addsuffix .o, $(basename $(TEST_SOURCES)))
VALIDATE_OBJECTS := $(addsuffix .o, $(basename $(VALIDATE_SOURCES)))

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


all: yafdb-blur yafdb-detect yafdb-preview yafdb-test yafdb-validate

clean:
	@rm -f yafdb-blur yafdb-detect yafdb-preview yafdb-test yafdb-validate
	@rm -f $(BLUR_OBJECTS)
	@rm -f $(DETECT_OBJECTS)
	@rm -f $(PREVIEW_OBJECTS)
	@rm -f $(TEST_OBJECTS)
	@rm -f $(VALIDATE_OBJECTS)


yafdb-blur: $(BLUR_OBJECTS)
	@echo "linking $(subst $(BASE_DIR),,$@)..."
	@$(LINK.o) -o $@ $^ $(LIBRARIES)

yafdb-detect: $(DETECT_OBJECTS)
	@echo "linking $(subst $(BASE_DIR),,$@)..."
	@$(LINK.o) -o $@ $^ $(LIBRARIES)

yafdb-preview: $(PREVIEW_OBJECTS)
	@echo "linking $(subst $(BASE_DIR),,$@)..."
	@$(LINK.o) -o $@ $^ $(LIBRARIES)

yafdb-test: $(TEST_OBJECTS)
	@echo "linking $(subst $(BASE_DIR),,$@)..."
	@$(LINK.o) -o $@ $^ $(LIBRARIES)

yafdb-validate: $(VALIDATE_OBJECTS)
	@echo "linking $(subst $(BASE_DIR),,$@)..."
	@$(LINK.o) -o $@ $^ $(LIBRARIES)


%.o: %.c
	@echo "compiling $(subst $(BASE_DIR),,$<)..."
	@$(COMPILE.c) -o $@ $<

%.o: %.cpp
	@echo "compiling $(subst $(BASE_DIR),,$<)..."
	@$(COMPILE.cpp) -o $@ $<


.PHONY:	all clean
