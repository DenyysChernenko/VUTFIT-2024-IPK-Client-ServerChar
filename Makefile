# Compiler
CC = gcc
# Compiler flags
CFLAGS = -Wall -Wextra -g
# Additional libraries
LIBS = -pthread
# Source files
SOURCES = main.c errorproc.c TCP.c supportTCP.c UDP.c
# Object files
OBJECTS = $(SOURCES:.c=.o)
# Executable name
EXECUTABLE = ipk24-chat.exe

# Default rule
all: $(EXECUTABLE)

# Rule to compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to link object files into the executable
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(LIBS) $^ -o $@

# Clean rule to remove object files and the executable
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
