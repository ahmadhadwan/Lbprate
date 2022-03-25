CC=gcc
CFLAGS=
LFLAGS=-lcurl

BIN_DIR=./bin/

TARGET=lbprate

all: bin lbprate

bin:
	@echo "Creating $@ directory.." 
	@mkdir $@

lbprate: lbprate.c
	@echo "Compiling $@.."
	@$(CC) $(CFLAGS) $^ -o $(BIN_DIR)$@ $(LFLAGS)
