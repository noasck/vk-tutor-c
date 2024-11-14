CFLAGS = -Wall -std=c11 -O2
LDFLAGS = -lcglm -lvulkan -lglfw -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

compile: src/main.c src/vkinit.c
	gcc $(CFLAGS) -o test.out src/main.c src/vkinit.c -DLLVM_MESA $(LDFLAGS)

run: compile
	 VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json ./test.out
