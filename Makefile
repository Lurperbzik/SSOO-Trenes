CC = gcc
CFLAGS = -m32
LDFLAGS = -m32
LIBS = -L. -llomo

TARGET = lomo
SRC = lomo.c
OBJ = lomo.o

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) $(LIBS) -o $@

$(OBJ): $(SRC) lomo.h
	$(CC) $(CFLAGS) -c $(SRC) -o $(OBJ)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
