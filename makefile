CC := g++
CFLAGS := -std=c++14 -g
TARGET := test
OBJS := $(wildcard *.cpp)

$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)