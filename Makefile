CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra $(shell pkg-config --cflags gtk+-3.0 libcurl)
LDFLAGS = $(shell pkg-config --libs gtk+-3.0 libcurl)

SRC = src/main.cpp \
      src/html_parser.cpp \
      src/style.cpp \
      src/layout.cpp \
      src/renderer.cpp \
      src/network.cpp \
      src/browser_tab.cpp \
      src/main_window.cpp \
      src/gfx_search_box.cpp \
      src/gfx_canvas.cpp

OBJ = $(SRC:.cpp=.o)
TARGET = mk-browse

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $(TARGET) $(LDFLAGS)
	@echo "✓ Successfully built $(TARGET) with MCP-generated GFX & Search engine"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(TARGET)
	@echo "→ Running Headless Engine Tests..."
	./$(TARGET) --test about:home
	./$(TARGET) --test about:mkcorpo
	@echo "→ Verifying Real Web Fetching..."
	./$(TARGET) --test https://news.ycombinator.com
	@echo "✓ All tests verified successfully!"

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all test clean
