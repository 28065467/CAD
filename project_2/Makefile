CXX      = g++
CXXFLAGS = -std=c++17 -Wall -O2
TARGET   = cbi

all: $(TARGET)

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

run: $(TARGET)
	./$(TARGET) Example/input.cbi output.cbi

plot: run
	gnuplot -e "set terminal pngcairo size 800,700; set output 'output.png'" output.plt

clean:
	rm -f $(TARGET) output.cbi output.plt output.png

.PHONY: all run plot clean
