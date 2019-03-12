SOLIB:=librtmpsdk


SRC_DIRS := $(shell find $(shell pwd) -maxdepth 10 -type d)
CXX_SRCS  = $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.cpp))
CXX_OBJS  = $(patsubst %.cpp, %.o, $(CXX_SRCS))
C_SRCS    = $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c))
C_OBJS    = $(patsubst %.c, %.o, $(C_SRCS))

all: $(SOLIB)

$(SOLIB): $(CXX_OBJS) $(C_OBJS)
ifeq ($(CXX_SRCS),)
	$(CC)  -shared $(CFLAGS) -o $@.so $^
else
	$(CXX)  -shared $(CFLAGS) -o $@.so $^	
endif

%.o:%.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

