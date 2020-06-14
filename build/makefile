CXXFLAGS=--std=c++11 -g -Wall
OBJS=build.o opcodes.o textutil.o dump.o parse.o parsestate.o generate.o program.o

build: $(OBJS)
	$(CXX) $(OBJS) -o build

clean:
	$(RM) *.o build

.PHONY: clean
