
CC=gcc
CFLAGS=


LIBS=-lcyaml

HFILES = $(wildcard src/*.h)
CFILES = $(wildcard src/*.c)
OFILES = $(patsubst src/%.c,obj/%.o,$(CFILES))


debug: $(OFILES)
	$(CC) -o vcd2svg $^ $(CFLAGS) $(LIBS) -g

fast: $(OFILES)
	$(CC) -o vcd2svg $^ $(CFLAGS) $(LIBS) -O3


obj/%.o: src/%.c | obj/
	$(CC) -c -o $@ $< $(CFLAGS) -g

obj/:
	mkdir obj

.PHONY: clean

clean:
	rm -f obj/*.o *~ core $(INCDIR)/*~ 