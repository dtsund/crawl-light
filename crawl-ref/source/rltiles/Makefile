uname_S := $(shell uname -s)

ifneq (,$(findstring MINGW,$(uname_S)))
LDFLAGS += -lmingw32
endif

ifndef CONTRIB_SDL
SDL_CFLAGS := $(shell sdl-config --cflags 2> /dev/null || echo "-I../contrib/install/include/SDL")
SDL_LDFLAGS := $(shell sdl-config --libs 2> /dev/null && echo "-lSDL_image" || echo "../contrib/install/lib/libSDL.a ../contrib/install/lib/libSDLmain.a ../contrib/install/lib/libSDL_image.a")
else
SDL_CFLAGS := -I../contrib/install/include/SDL
SDL_LDFLAGS := ../contrib/install/lib/libSDL.a ../contrib/install/lib/libSDLmain.a ../contrib/install/lib/libSDL_image.a
endif

ifndef NO_PKGCONFIG
PNG_INCLUDE := $(shell pkg-config libpng --cflags 2> /dev/null || echo "-I../contrib/install/include")
PNG_LIB := $(shell pkg-config libpng --libs 2> /dev/null || echo "../contrib/install/lib/libpng.a ../contrib/install/lib/libz.a")
else
PNG_INCLUDE := -I../contrib/install/include
PNG_LIB := ../contrib/install/lib/libpng.a ../contrib/install/lib/libz.a
endif

CFLAGS := -O2 $(SDL_CFLAGS) $(PNG_INCLUDE)
LDFLAGS += $(SDL_LDFLAGS) $(PNG_LIB)

ifdef DEBUG
CFLAGS += -DDEBUG -ggdb
endif

ifneq (,$(findstring MINGW,$(uname_S)))
LDFLAGS += -lgdi32 -lwinmm
endif
ifeq ($(uname_S),Darwin)
LDFLAGS += -framework AppKit -framework AudioUnit -framework Carbon -framework IOKit -framework OpenGL
endif
ifeq ($(uname_S),Linux)
LDFLAGS += -ldl -lpthread
endif

# Attempt to use a full compiler name, to make
# distcc builds work nicely.
LMACH := $(shell gcc -dumpmachine)-
ifeq ($(LMACH),-)
LMACH :=
endif
ifeq ($(shell which $(LMACH)gcc > /dev/null 2> /dev/null && echo "Yes"),)
LMACH :=
endif
HOSTCXX ?= $(LMACH)g++
ifdef ARCH
CF_ARCHS := $(patsubst %,-arch %,$(ARCH))
HOSTCXX += $(CF_ARCHS)
endif

DELETE = rm -f

TOOLDIR := tool
TILEGEN := $(TOOLDIR)/tilegen.elf

INPUTS := main dngn floor wall feat player gui icons
INPUTFILES := $(INPUTS:%=dc-%.txt)
HEADERS := $(INPUTS:%=tiledef-%.h)
HTML := $(INPUTS:%=tile-%.html)
SOURCE := $(INPUTS:%=tiledef-%.cc)
IMAGES := $(INPUTS:%=%.png)

ifneq ($(findstring $(MAKEFLAGS),s),s)
ifndef V
        QUIET_HOSTCXX      = @echo '   ' HOSTCXX $@;
        QUIET_GEN      = @echo '   ' GEN $(lastword $(subst -, ,$*));
        QUIET_HOSTLINK     = @echo '   ' HOSTLINK $@;
        export V
endif
endif

BASE_OBJECTS := tile_colour.o tile.o tile_page.o tile_list_processor.o main.o

# MSVCRT's tmpfile() is broken, so use our own version instead.
ifneq (,$(findstring MINGW,$(uname_S)))
BASE_OBJECTS += tmpfile.o
endif

OBJECTS := $(BASE_OBJECTS:%=$(TOOLDIR)/%)

all: $(TILEGEN) $(HEADERS) $(SOURCE) $(IMAGES)

%.png tiledef-%.h tiledef-%.cc: dc-%.txt | $(TILEGEN)
	$(QUIET_GEN)$(TILEGEN) $<

# CFLAGS difference check
TRACK_CFLAGS = $(subst ','\'',$(HOSTCXX) $(CFLAGS))           # (stray ' for highlights)

.cflags: .force-cflags
       @FLAGS='$(TRACK_CFLAGS)'; \
    if test x"$$FLAGS" != x"`cat .cflags 2>/dev/null`" ; then \
        echo "    * rebuilding crawl: new build flags or prefix"; \
        echo "$$FLAGS" > .cflags; \
    fi

.PHONY: .force-cflags

##########################################################################
# Dependencies 

DEPS := $(OBJECTS:%.o=%.d) $(INPUTS:%=%.d)

-include $(DEPS)

clean:
	$(DELETE) $(HEADERS) $(OBJECTS) $(TILEGEN) $(SOURCE) $(IMAGES) $(HTML) $(DEPS)

distclean: clean

%.o: %.cc .cflags
	$(QUIET_HOSTCXX)$(HOSTCXX) $(CFLAGS) -Wp,-MD,$*.d,-MT,$@ -c $< -o $@

$(TILEGEN): $(OBJECTS)
	$(QUIET_HOSTLINK)$(HOSTCXX) $(OBJECTS) -o $@ $(LDFLAGS)

.PHONY: all clean distclean
