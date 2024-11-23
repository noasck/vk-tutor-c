CFLAGS = -Wall -std=c11 -O2
LDFLAGS = -lcglm -lvulkan -lglfw -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

SRC = src/main.c src/vkinit.c src/shaderUtils.c
BUILD_DIR = build
SHADER_DIR = $(BUILD_DIR)/shaders
RES_DIR = src/resources
SHADER_SRC_DIR = src/shaders
SHADER_RES_DIR = ${RES_DIR}/shaders

SHADER_FILES = $(wildcard $(SHADER_SRC_DIR)/*.glsl)
SPV_FILES = $(patsubst $(SHADER_SRC_DIR)/%.glsl, $(SHADER_DIR)/%.spv, $(SHADER_FILES))
HEADER_FILES = $(patsubst $(SHADER_SRC_DIR)/%.glsl, $(SHADER_RES_DIR)/%.h, $(SHADER_FILES))
OUTPUT = $(BUILD_DIR)/test.out

all: $(BUILD_DIR) $(SHADER_DIR) $(RESOURCE_DIR) $(SPV_FILES) $(HEADER_FILES) $(OUTPUT)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(SHADER_DIR):
	mkdir -p $(SHADER_DIR)

$(SHADER_RES_DIR):
	mkdir -p $(SHADER_RES_DIR)

$(SHADER_DIR)/%.spv: $(SHADER_SRC_DIR)/%.glsl | $(SHADER_DIR)
	if echo $< | grep -q "_frag.glsl"; then \
		glslc -fshader-stage=frag $< -o $@; \
	elif echo $< | grep -q "_vert.glsl"; then \
		glslc -fshader-stage=vert $< -o $@; \
	else \
		echo "Error: Unrecognized shader type for $<"; \
		exit 1; \
	fi

$(SHADER_RES_DIR)/%.h: $(SHADER_DIR)/%.spv | $(SHADER_RES_DIR)
	xxd -i $< > $@

$(OUTPUT): $(SRC) | $(BUILD_DIR)
	gcc -g $(CFLAGS) -o $@ $(SRC) -DLLVM_MESA $(LDFLAGS)

run: $(OUTPUT)
	XLIB_SKIP_ARGB_VISUALS=1 \
	LIBGL_ALWAYS_SOFTWARE=1 \
	VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json \
	VK_LAYER_PATH=/usr/share/vulkan/explicit_layer.d:/usr/share/vulkan/implicit_layer.d \
	./$<

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(SHADER_RES_DIR)

.PHONY: all clean run
