.PHONY: all 

EXE_DIR=../bin
SRC = $(wildcard *.cpp)
EXE = $(patsubst %.cpp,$(EXE_DIR)/%,$(SRC))
INC = -I ../src
LIB = -L ../lib -lcliop

all : $(EXE)

$(EXE) : $(EXE_DIR)/% : %.cpp ../lib/libcliop.a
	g++ -o $@ $^ $(INC) $(LIB)
