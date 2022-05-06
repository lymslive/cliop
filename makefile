CXX=g++
AR = ar
ARFLAGS = crsv

SRC_DIR=src
LIB_DIR=lib
OBJ_DIR=build
EXE_DIR=bin
TEST_DIR = utest

vpath %.cpp $(SRC_DIR) $(TEST_DIR)

EXTENSION=cpp

# source file *.cpp
SRC=$(wildcard $(SRC_DIR)/*.$(EXTENSION))
OBJS=$(patsubst $(SRC_DIR)/%.$(EXTENSION), $(OBJ_DIR)/%.o, $(SRC))

# source file *.cpp for unit test
TEST_SRC  := $(wildcard $(TEST_DIR)/*.$(EXTENSION))
TEST_OBJS := $(patsubst $(TEST_DIR)/%.$(EXTENSION), $(OBJ_DIR)/%.o, $(TEST_SRC))
TEST_OBJS += $(OBJS)
DEP_ALL := $(patsubst %.o,%.d,$(TEST_OBJS))
TEST_OBJS :=$(filter-out $(OBJ_DIR)/main.o,$(TEST_OBJS))

CXXFLAGS += -std=c++11
ifeq ($(_debug),1)
CXXFLAGS += -g -D_DEBUG
else
CXXFLAGS += -O2
endif

LDFLAGS = 

INCLUDE =
ifeq ($(MAKECMDGOALS),test)
INCLUDE += -I src/
endif
ifeq ($(MAKECMDGOALS),utest)
INCLUDE += -I src/
endif

TARGET=$(LIB_DIR)/libcliop.a
TEST_TARGET=$(EXE_DIR)/utest-cliop

.PHONY: all clean rebuild dir test utest sample docs
all: dir $(TARGET)

ifneq ($(MAKECMDGOALS),clean)
-include $(DEP_ALL)
endif

dir:
	@mkdir -p $(EXE_DIR)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(LIB_DIR)

#$(OBJ_DIR)/%.o:$(SRC_DIR)/%.$(EXTENSION)
$(OBJ_DIR)/%.o:%.$(EXTENSION)
	$(CXX) -c -o $@ -MMD -MT $@ $(CXXFLAGS) $(INCLUDE) $<

$(TARGET):$(OBJS)
	$(AR) $(ARFLAGS) -o $@ $^

$(TEST_TARGET):$(TEST_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

TINYTAST = utest/tinytast.hpp
utest : $(TINYTAST) dir $(TEST_TARGET)
test : utest
	$(TEST_TARGET) --cout=silent

TINYTAST_REMOTE = https://raw.githubusercontent.com/lymslive/couttast/main/include/tinytast.hpp
$(TINYTAST) :
	@wget $(TINYTAST_REMOTE) -O $@

sample :
	make -C sample/

docs :
	cd docs/ && doxygen

rebuild: clean all

clean:
	rm -rf $(OBJ_DIR)/*.o $(TARGET)

echo:
	@echo TARGET = $(TARGET)
	@echo OBJS = $(OBJS)
