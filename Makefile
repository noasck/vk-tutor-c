CFLAGS = -Wall -std=c11 -O2
LDFLAGS = -lcglm -lvulkan -lglfw -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

SRC = src/main.c src/vkinit.c
BUILD_DIR = build
SHADER_DIR = $(BUILD_DIR)/shaders
SHADER_SRC_DIR = src/shaders
SHADER_FILES = $(wildcard $(SHADER_SRC_DIR)/*.glsl)
SPV_FILES = $(patsubst $(SHADER_SRC_DIR)/%.glsl, $(SHADER_DIR)/%.spv, $(SHADER_FILES))
OUTPUT = $(BUILD_DIR)/test.out

all: $(BUILD_DIR) $(SHADER_DIR) $(SPV_FILES) $(OUTPUT)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(SHADER_DIR):
	mkdir -p $(SHADER_DIR)

$(SHADER_DIR)/%.spv: $(SHADER_SRC_DIR)/%.glsl | $(SHADER_DIR)
	if echo $< | grep -q "_frag.glsl"; then \
		glslc -fshader-stage=frag $< -o $@; \
	elif echo $< | grep -q "_vert.glsl"; then \
		glslc -fshader-stage=vert $< -o $@; \
	else \
		echo "Error: Unrecognized shader type for $<"; \
		exit 1; \
	fi

$(OUTPUT): $(SRC) | $(BUILD_DIR)
	gcc $(CFLAGS) -o $@ $(SRC) -DLLVM_MESA $(LDFLAGS)

run: $(OUTPUT)
	VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json ./$<

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean run
