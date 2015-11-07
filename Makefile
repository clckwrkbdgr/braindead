TARGET=braindead

all: $(TARGET)

$(TARGET): $(TARGET).cpp
	$(CXX) -o $@ -lncurses $^

brainfuck: brainfuck.c
	$(CC) -o $@ $^

.PHONY: run
run: brainfuck
	./brainfuck

clean:
	$(RM) $(TARGET)
