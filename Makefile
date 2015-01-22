SOURCE=main.cpp
CXXFLAGS=-std=c++11 -Wall -Wextra -Wfatal-errors -ggdb -pg

test: main.o
	$(CXX) $(CXXFLAGS) -o $@ $^ -lsfml-graphics -lsfml-window -lsfml-system
