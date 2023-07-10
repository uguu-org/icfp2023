target = solve.exe

CC = g++
LD = g++
ifeq ($(NDEBUG),1)
CFLAGS = -DNDEBUG -O3 -fexpensive-optimizations -finline-functions -Wall -Werror -pedantic
LFLAGS = -O3
else
CFLAGS = -g -O2 -Wall -Werror -pedantic
LFLAGS = -g -O2
endif


.SUFFIXES = .cc .o

.cc.o:
	$(CC) $(CFLAGS) -c $< -o $@


$(target): main.o problem.o solution.o grid.o
	$(LD) $(LFLAGS) $^ -o $@

main.o: main.cc problem.h solution.h

problem.o: problem.cc problem.h

solution.o: solution.cc solution.h problem.h grid.h

grid.o: grid.cc grid.h problem.h

grid.h: problem.h

solution.h: problem.h


verify_example.exe: verify_example.o problem.o solution.o
	$(LD) $(LFLAGS) $^ -o $@

verify_example.o: verify_example.cc problem.h solution.h

clean:
	-rm -f *.o *.exe
