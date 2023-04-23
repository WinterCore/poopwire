CFLAGS = -std=c11 -Wall -Wextra `pkg-config --cflags libpipewire-0.3`
LDFLAGS = -lm `pkg-config --libs libpipewire-0.3`

.PHONY: all clean executable

all: exec

exec: Run
	./Run

clean:
	rm -rf Run

Run: src/main.c
	cc $(CFLAGS) -o Run src/main.c $(LDFLAGS)
