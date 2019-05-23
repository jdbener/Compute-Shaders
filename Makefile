CC=g++
CFLAGS=-g -I/usr/include
DIR=src

LIBS=/usr/lib/libglfw.so.3.3 /usr/lib/libGLEW.so.2.1.0 -lGL

# .cpp.ex and .h/pp.ex files are excluded from these lists
_SRC := $(shell find $(DIR) -name '*.cpp')
SRC := $(subst $(DIR)/,,$(_SRC))

DEP := $(shell find $(DIR) -name '*.h') $(shell find $(DIR) -name '*.hpp')

OBJ := $(addprefix $(DIR)/,$(SRC:%.cpp=%.o))

%.o: %.cpp $(DEP)
	$(CC) -c -o $@ $< $(CFLAGS)

%.o: %.c $(DEP)
	$(CC) -c -o $@ $< $(CFLAGS)

Run: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
	chmod o+x $@;

.PHONY: clean run

clean:
	rm -f $(OBJ)

run:
	'./Run';
