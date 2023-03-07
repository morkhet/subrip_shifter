CC=gcc
CCFLAGS=-Wall -Wextra -Werror
LIBS= -lm
DBG_CCFLAGS= -g
SRC= subrip_shifter.c
OUT= subrip_shifter

all: build
debug:
	@$(CC) $(CCFLAGS) $(LIBS) $(DBG_CCFLAGS) $(SRC) -o $(OUT)
build:
	@$(CC) $(CCFLAGS) $(LIBS) $(SRC) -o $(OUT)
clean:
	@[ -f $(OUT) ] && rm $(OUT) || true
