CC = gcc
CFLAGS = -O2 -Wall
LFLAGS = -pthread
OBJS = main.o lcd.o temp.o web.o
SRC = main.c lcd.c temp.c web.c
TARGET = milkshake

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LFLAGS) -o $(TARGET) $(OBJS)


clean:	
	rm -f *~ *.o $(TARGET)

