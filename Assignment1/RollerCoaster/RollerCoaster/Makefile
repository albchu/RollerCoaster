CC=g++

CFLAGS=-c -std=c++0x -O3 -Wall
LDFLAGS=-lGLEW -lGL -lglut -L/usr/X11R6/lib 
SOURCES=main.cpp ShaderTools.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=RollerCoaster

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm *.o $(EXECUTABLE)
