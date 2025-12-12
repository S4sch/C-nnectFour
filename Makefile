CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -pedantic

TARGET=c_nnect_four
SRC=connect_four.c io_engine.c rl_agent.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) *.o
