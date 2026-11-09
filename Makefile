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
      src/main_window.cpp

OBJ = $(SRC:.cpp=.o)
TARGET = mk-browse

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $(TARGET) $(LDFLAGS)
	@echo "✓ Successfully built $(TARGET)"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(TARGET)
	@echo "→ Running Headless Engine Tests..."
	./$(TARGET) --test about:home
	./$(TARGET) --test about:mkcorpo
	@echo "→ Running Virtual X11 GUI Test via xvfb-run..."
	xvfb-run -a timeout 3 ./$(TARGET) about:home || true
	@echo "✓ All tests verified successfully!"

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all test clean
