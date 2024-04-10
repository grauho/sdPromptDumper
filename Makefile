.POSIX:
CC		= cc
CFLAGS		= -Wall -pedantic -Wno-unused-function -O2 
LDFLAGS		= 
PREFIX		= /usr/local
OBJFILES	= main.o stiTokenizer.o pngProcessing.o loadConfig.o
TARGET		= sdPromptDumper

ifeq ($(OS),Windows_NT)
TARGET = sdPromptDumper.exe
endif # Windows

all: $(TARGET)

$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)

rebuild: clean
rebuild: all

clean:
	rm -f $(OBJFILES) $(TARGET)

.PHONY: all rebuild clean
