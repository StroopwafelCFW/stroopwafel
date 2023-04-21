OBJS = main.o utils.o sha1.o
LIBS = #-static-libstdc++ -static
CXXFLAGS = -I. 
CFLAGS = -std=c99 -Wall -Wno-unused-variable -Wno-unused-but-set-variable -I.
OUTPUT = salt-patch
CC = gcc

main: $(OBJS)
	$(CC) -o $(OUTPUT) $(LIBS) $(OBJS)


clean:
	rm -rf $(OUTPUT) $(OBJS) $(POLAR_OBJS) $(TINYXML_OBJS)
