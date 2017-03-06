FILE=rasteriser

########
#   Directories
S_DIR=Source
B_DIR=Build

########
#   Output
EXEC=$(B_DIR)/$(FILE)

# default build settings
CC_OPTS=-c -pipe -Wall -Wno-switch -ggdb -g3 -O3
LN_OPTS=-lX11
CC=c++

########
#       SDL options
SDL_CFLAGS := $(shell sdl-config --cflags)
GLM_CFLAGS := -I$(GLMDIR)
SDL_LDFLAGS := $(shell sdl-config --libs)

CPP = /usr/local/opt/llvm/bin/clang++
CPPFLAGS = -I/usr/local/opt/llvm/include -fopenmp
LDFLAGS = -L/usr/local/opt/llvm/lib
########
#   This is the default action
all:Build


########
#   Object list
#
OBJ = $(B_DIR)/$(FILE).o


########
#   Objects
$(B_DIR)/$(FILE).o : $(S_DIR)/$(FILE).cpp $(S_DIR)/SDLauxiliary.h $(S_DIR)/TestModel.h
	$(CPP) $(CC_OPTS) -o $(B_DIR)/$(FILE).o $(S_DIR)/$(FILE).cpp $(SDL_CFLAGS) $(GLM_CFLAGS)


########
#   Main build rule
Build : $(OBJ) Makefile
	$(CC)  $(LN_OPTS) -o $(EXEC) $(OBJ) $(SDL_LDFLAGS)


clean:
	rm -f $(B_DIR)/*
