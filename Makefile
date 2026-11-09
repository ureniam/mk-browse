CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra $(shell pkg-config --cflags gtk+-3.0 libcurl)
LDFLAGS = $(shell pkg-config --libs gtk+-3.0 libcurl)

CORE_OBJ = src/html_parser.o \
           src/style.o \
           src/layout.o \
           src/renderer.o \
           src/network.o \
           src/gfx_canvas.o

OBJ = src/main.o \
      src/browser_tab.o \
      src/main_window.o \
      src/gfx_search_box.o \
      $(CORE_OBJ)

TARGET = mk-browse
TEST_TARGET = tests/stress_test

all: $(TARGET) $(TEST_TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $(TARGET) $(LDFLAGS)
	@echo "✓ Successfully built $(TARGET) with MCP-generated GFX & Search engine"

$(TEST_TARGET): tests/stress_test.cpp $(CORE_OBJ)
	$(CXX) $(CXXFLAGS) tests/stress_test.cpp $(CORE_OBJ) -o $(TEST_TARGET) $(LDFLAGS)
	@echo "✓ Successfully built $(TEST_TARGET)"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(TARGET) $(TEST_TARGET)
	@echo "→ Running Stress & Hardening Test Suite..."
	./$(TEST_TARGET)
	@echo "→ Running Headless Engine Tests..."
	./$(TARGET) --test about:home
	./$(TARGET) --test about:mkcorpo
	@echo "→ Verifying Real Web Fetching..."
	./$(TARGET) --test https://news.ycombinator.com
	@echo "✓ All tests verified successfully!"

clean:
	rm -f $(OBJ) $(TARGET) $(TEST_TARGET)

.PHONY: all test clean
