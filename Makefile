
CFLAGS=-c -std=gnu99 -g -Wall
LDFLAGS=-lcec -ldl
EXECUTABLE=ceclaunchd

SOURCES=ceclaunchd.c
OBJECTS=$(SOURCES:.c=.o)

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@

.cpp.o:
	$(CXX) $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
