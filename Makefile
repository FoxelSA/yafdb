# Includes
INCLUDES := include/

# Sources
SOURCES := $(realpath $(wildcard src/*.c src/*.cpp))

# Objects
OBJECTS := $(addsuffix .o, $(basename $(SOURCES)))

# Compilation flags
RELEASEFLAGS := -g -O0
#RELEASEFLAGS := -O3
CPPFLAGS += -c -pipe $(foreach dir, $(INCLUDES), -I$(dir))
CFLAGS += -std=gnu99
CXXFLAGS += -std=gnu++98
LDFLAGS += -pipe -lstdc++ -lm -lopencv_core -lopencv_imgproc -lopencv_contrib \
	-lopencv_features2d -lopencv_calib3d -lopencv_photo -lopencv_video -lopencv_videostab \
	-lopencv_highgui


all: yafdb-detect yafdb-blur yafdb-test

clean:
	rm -f yafdb-detect yafdb-blur yafdb-test $(OBJECTS)


yafdb-detect: $(filter-out $(realpath src/blur.o) $(realpath src/test.o),$(OBJECTS))
	@echo "linking program $@..."
	@$(LINK.o) $(RELEASEFLAGS) -o $@ $^

yafdb-blur: $(filter-out $(realpath src/detect.o) $(realpath src/test.o),$(OBJECTS))
	@echo "linking program $@..."
	@$(LINK.o) $(RELEASEFLAGS) -o $@ $^

yafdb-test: $(filter-out $(realpath src/blur.o) $(realpath src/detect.o),$(OBJECTS))
	@echo "linking program $@..."
	@$(LINK.o) $(RELEASEFLAGS) -o $@ $^


%.o: %.c
	@echo "compiling $<..."
	@$(COMPILE.c) $(RELEASEFLAGS) -o $@ $<

%.o: %.cpp
	@echo "compiling $<..."
	@$(COMPILE.cpp) $(RELEASEFLAGS) -o $@ $<


.PHONY:	all clean
