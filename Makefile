
CC = gcc
CFLAGS = -Wall -g -Iinclude
LDFLAGS =  -l:libdarray.a -l:libdrings.a -lglfw -lm -lglfw -lvulkan

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)

TARGET = main

all: run
	
%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS) 

$(TARGET): $(OBJ) 
	./compile.sh
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
 
run: $(TARGET)
	./$(TARGET)

comp_shaders: 
	./compile.sh

clean:
	rm -rf $(TARGET) $(OBJ)


