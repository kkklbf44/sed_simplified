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

test: $(TARGET)
	@echo "Running tests..."
	@echo "This is a test file" > test_input.txt
	./$(TARGET) test_input.txt 's/test/working/'
	@cat test_input.txt
	@rm test_input.txt
	@echo "Test finished."

clean:
	rm -f $(OBJECTS) $(TARGET)