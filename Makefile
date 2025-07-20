
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
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
 
run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(TARGET) $(OBJ)


