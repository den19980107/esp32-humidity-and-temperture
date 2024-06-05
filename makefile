PROJECT_NAME := esp32

# Default target, executed when typing 'make' without arguments
all:
	@echo "Available targets:"
	@echo "  make build     - Build the project"
	@echo "  make upload    - Upload firmware to a connected board"
	@echo "  make monitor   - Monitor serial output from a connected board"
	@echo "  make clean     - Clean project (remove compiled files)"

build:
	pio run

upload:
	pio run -t upload

monitor:
	pio device monitor

clean:
	pio run -t clean

.PHONY: all build upload monitor clean
