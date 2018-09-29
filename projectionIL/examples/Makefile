all: sequence-build test0-build

sequence-build:
	g++ sequence.cpp -std=c++11 -I../include/ -O0 -L../ -lSPL -o sequence

test0-build:
	g++ test0.cpp -std=c++11 -I../include/ -O0 -L../ -lSPL -o test0

test0-run:
	LD_LIBRARY_PATH="`pwd`/../" ./test0
	
sequence-run:
	LD_LIBRARY_PATH="`pwd`/../" ./sequence
	
clean:
	rm -rf *.h.gch *.o src/*.h.gch src/*.o  src/*.o sequence
