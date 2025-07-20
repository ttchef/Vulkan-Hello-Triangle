
CC = gcc
CFLAGS = -Wall -g -Iinclude
LDFLAGS =  -l:libdarray.a -l:libdrings.a -lglfw -lm -lglfw -lvulkan

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)

TARGET = main

all: $(TARGET)
	
%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS) 

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
 
clean:
	rm -rf $(TARGET) $(OBJ)


