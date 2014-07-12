BOOST=C:/path/to/boost
GLPKDEVEL=C:/path/to/usr/local
CCFLAGS=-DBOOST_NO_DEFAULTED_FUNCTIONS -I $(BOOST) -I $(GLPKDEVEL)/include -L $(GLPKDEVEL)/lib -std=c++0x -O3 -Wall
CC=g++

ChinesePostman.exe: ChinesePostman.o
	$(CC) $(CCFLAGS) $< -lglpk -o $@

.cpp.o:
	$(CC) $(CCFLAGS) -c $< -lglpk -o $@

clean:
	rm -f *.o
	rm -f *.exe
