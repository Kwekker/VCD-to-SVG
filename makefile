
CC=gcc
CFLAGS=


LIBS=-lcyaml

HFILES = $(wildcard src/*.h)
CFILES = $(wildcard src/*.c)
OFILES = $(patsubst src/%.c,obj/%.o,$(CFILES))


hellomake: $(OFILES)
	$(CC) -o vcd2svg $^ $(CFLAGS) $(LIBS)

obj/%.o: src/%.c | obj/
	$(CC) -c -o $@ $< $(CFLAGS)

obj/:
	mkdir obj

.PHONY: clean

clean:
	rm -f obj/*.o *~ core $(INCDIR)/*~ 