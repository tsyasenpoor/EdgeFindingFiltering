CXX      = g++
CXXFLAGS = -O2 -std=c++17 -Wall -Wextra
TARGET   = edge_finding
TEST     = tests
SRCS     = edge_finding.cpp main.cpp
TEST_SRCS = edge_finding.cpp tests.cpp

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST): $(TEST_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

test: $(TEST)
	./$(TEST)

clean:
	rm -f $(TARGET) $(TEST)

.PHONY: test clean