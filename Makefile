CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_GNU_SOURCE
TARGET = sed_simplified
SOURCES = main.c operations.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)