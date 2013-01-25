TARGET=braindead

all: $(TARGET)

$(TARGET): $(TARGET).cpp
	$(CXX) -o $@ -lncurses $^

clean:
	$(RM) $(TARGET)
