# Makefile for Arduino project

# Define the Arduino CLI command
ARDUINO_CLI = arduino-cli

# Define the board and port variables
BOARD = esp32:esp32:esp32s3
PORT = /dev/ttyACM1

# Default target with a placeholder for PORT
all: compile upload

# Compile target
compile:
	$(ARDUINO_CLI) compile -b $(BOARD)  --build-property 'compiler.cpp.extra_flags=-D MQTT_CLIENT_NAME="$(ID)"' 

# Upload target with a conditional check for PORT
upload:
	@echo "PORT is set to: $(PORT)"
	@echo "Board is set to: $(BOARD)"
	$(if $(PORT), $(ARDUINO_CLI) upload -b $(BOARD) -p $(PORT), $(error PORT is not set))

# Clean target
clean:
	$(ARDUINO_CLI) cache clean

# Set the port to a specific value
setport:
	$(eval PORT := $(filter-out $@,$(MAKECMDGOALS)))
	@echo "Id $(ID)"
	$(if $(ID),export MQTT_ID=$(ID), $(error ID is not set -> ID=*))
	@echo "Port set to: $(PORT)"
	$(MAKE) compile
	$(MAKE) upload PORT=$(PORT)
