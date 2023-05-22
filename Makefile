FLAG_INCLUDE	:=	-I ./include
FLAG_CPP		:=	-std=gnu++17
FLAG_DEBUG		:=	-fsanitize=address -static-libasan -g
COMMON_FILES	:=	$(wildcard *.cpp ./common/*)
LLVM_FLAGS		:=	`llvm-config --cxxflags --ldflags --system-libs --libs core`

FLAGS			:= $(COMMON_FILES) $(FLAG_INCLUDE) $(FLAG_CPP)


#MAIN_FILE		:= ./learn_llvm/c_global_show.cpp
#MAIN_FILE		:= ./learn_llvm/c_fun_if_else.cpp

#MAIN_FILE		:= ./src/test_lexer.cpp
MAIN_FILE		:= ./src/auto.cpp

all: a.out run compile
	time ./code

a.out:	$(MAIN_FILE)
	g++ $< $(FLAGS)

debug: $(MAIN_FILE)
	g++ $< $(FLAGS) $(FLAG_DEBUG)
	gdb ./a.out -tui -x gdb.txt
	
.PHONY: clean run compile
clean:
	rm -f ./a.out code.ll code.s code log.txt

run: a.out
	./a.out testcode4.in rules.in code.ll <grammar.in >log.txt
#	./a.out 1.in rules.in <grammar2.in >log.txt

compile:
#	lli code.ll
	llc code.ll
	gcc code.s -o code
