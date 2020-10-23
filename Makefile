SOURCE_FILES := $(wildcard bin/*.cc)
OBJECT_FILES := $(patsubst %.cc, %.o, $(SOURCE_FILES))

all: compile

compile: $(OBJECT_FILES) 
	g++ -o Inflatecpp.exe $^ 

%.o: %.cc
	g++ -c -o $@ $<