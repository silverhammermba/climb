SOURCE=main.cpp
EXE=climb
CXXFLAGS=-std=c++11 -Wall -Wextra -Wfatal-errors -O2

ifdef WINDOWS
EXE:=$(EXE).exe
CXX=x86_64-w64-mingw32-g++
CXXFLAGS+=-static
endif

$(EXE): main.o
	$(CXX) $(CXXFLAGS) -o $@ $^ -lsfml-audio -lsfml-graphics -lsfml-window -lsfml-system

clean:
	rm -f *.o $(EXE)
