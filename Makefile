CFLAGS = -Wall -std=c11 -O2
LDFLAGS = -lSDL2 -lcglm -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

compile: src/main.c src/vkinit.c
	gcc $(CFLAGS) -o test.out src/main.c src/vkinit.c $(LDFLAGS)
