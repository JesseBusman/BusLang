ifeq (, $(shell which clang++))
	ifeq (, $(shell which g++))
		$(error "No clang++ or g++ in path $(PATH)")
	else
		CPPC=g++
	endif
else
	CPPC=clang++
endif

#-Weverything -Wno-c++98-compat -Wno-pre-c++14-compat -Wno-reserved-identifier
#  -fsanitize=memory

CPPFLAGS = -O3 -std=c++26 -g -Wall -Werror -Wunreachable-code
CPPFILES = $(wildcard *.cpp) $(wildcard */*.cpp)
CPPDEPENDS = $(patsubst %.cpp,build/%.cpp.d,$(CPPFILES))
CPPOBJFILES = $(addprefix build/,$(CPPFILES:.cpp=.cpp.o))

build/buslang: $(CPPOBJFILES)
	mkdir -p build
	$(CPPC) $(CPPFLAGS) $(OFILES) -o $@ $(CPPOBJFILES)

-include $(CPPDEPENDS)

build/%.cpp.o: %.cpp
	mkdir -p build
	$(CPPC) -MMD -MP $(CPPFLAGS) -c -o $@ $(word 1, $<)

run: build/buslang
	build/buslang stdlib.bus

test: build/buslang
	build/buslang tests.bus

valgrind: build/busc
	valgrind --exit-on-first-error=yes --error-exitcode=1 build/buslang stdlib.bus

clean:
	rm -r build/
	rm -f vgcore.*
