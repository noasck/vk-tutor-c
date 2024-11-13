CFLAGS = -Wall -std=c11 -O2
LDFLAGS = -lSDL2 -lcglm -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

compile: main.c
	gcc $(CFLAGS) -o test.out main.c vkinit.c $(LDFLAGS)
