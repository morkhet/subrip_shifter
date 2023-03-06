CC=gcc
LIBS= -lm
DBG_CCFLAGS= -g
SRC= subrip_shifter.c
OUT= subrip_shifter

all: build
debug:
	@$(CC) $(LIBS) $(DBG_CCFLAGS) $(SRC) -o $(OUT)
build:
	@$(CC) $(LIBS) $(SRC) -o $(OUT)
clean:
	@[ -f $(OUT) ] && rm $(OUT) || true
