flags = -g -w -std=c++11

main:main.o
	g++ $(flags) -o main main.o

main.o:main.cpp
	g++ $(flags) -c main.cpp

clean:
	rm -rf *.o main