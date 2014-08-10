BOOST=C:/path/to/boost
GLPKDEVEL=C:/path/to/usr/local
CCFLAGS=-DBOOST_NO_DEFAULTED_FUNCTIONS -I $(BOOST) -I $(GLPKDEVEL)/include -L $(GLPKDEVEL)/lib -std=c++0x -O3 -Wall
CC=g++

default: DivideByBridge.exe SolveChinesePostman.exe

DivideByBridge.exe: DivideByBridge.o
	$(CC) $(CCFLAGS) $< -o $@

SolveChinesePostman.exe: SolveChinesePostman.o
	$(CC) $(CCFLAGS) $< -lglpk -o $@

.cpp.o:
	$(CC) $(CCFLAGS) -c $< -o $@

SolveChinesePostman.o: ChinesePostman.hpp ChinesePostmanUtil.hpp masked_vector.hpp
DivideByBridge.o: ChinesePostman.hpp ChinesePostmanUtil.hpp

clean:
	rm -f *.o
	rm -f *.exe
