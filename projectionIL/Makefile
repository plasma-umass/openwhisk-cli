all: 
	g++ src/ssaVisitor.cpp src/ast.cpp src/driver.cpp src/ssa.cpp -std=c++11 -Iinclude/ -Isrc/ -O0 -g -shared -fPIC -o libSPL.so

clean:
	rm -rf *.h.gch *.o src/*.h.gch src/*.o libSPL.so src/*.o
