# Paths
SHELL = powershell.exe
.SHELLFLAGS = -NoProfile -ExecutionPolicy Bypass -Command

WESL = wesl
WESL_DIR = src/wgpu/shader
OUTPUT_DIR = src/wgpu/shader

# Recursive wildcard function
rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

# Shader files
WESL_FILES = $(wildcard $(WESL_DIR)/*.wesl)
WESL_DEPS = $(call rwildcard,$(WESL_DIR),*.wesl)
WGSL_FILES = $(patsubst $(WESL_DIR)/%.wesl, $(OUTPUT_DIR)/%.wgsl, $(WESL_FILES))

# Default target
all: shaders

# Rule to compile shaders
shaders: $(WGSL_FILES)

# WESL to WGSL
$(OUTPUT_DIR)/%.wgsl: $(WESL_DIR)/%.wesl $(WESL_DEPS)
	& '$(WESL)' compile '$<' | Set-Content -Path '$@'

# Clean up
clean:
	Remove-Item -Path '$(OUTPUT_DIR)/*.wgsl' -Force -ErrorAction SilentlyContinue

.PHONY: all shaders clean
