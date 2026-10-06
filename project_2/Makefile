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

# 參數可覆寫，例如 make bench SEEDS=1-10 ITERS=500000
SEEDS ?= 1-5
ITERS ?= 300000
bench: $(TARGET)
	python3 test/bench.py --seeds $(SEEDS) --iters $(ITERS) --jobs 4

clean:
	rm -f $(TARGET) output.cbi output.plt output.png

.PHONY: all run plot bench clean
