IDF_PATH := /home/chris/.espressif/v6.0.1/esp-idf
PORT     := /dev/ttyUSB1

.PHONY: setup build flash monitor flash-monitor clean fullclean

setup:
	$(IDF_PATH)/install.sh

build:
	bash -c "source $(IDF_PATH)/export.sh && idf.py build"

flash:
	bash -c "source $(IDF_PATH)/export.sh && idf.py -p $(PORT) flash"

monitor:
	bash -c "source $(IDF_PATH)/export.sh && idf.py -p $(PORT) monitor"

flash-monitor:
	bash -c "source $(IDF_PATH)/export.sh && idf.py -p $(PORT) flash monitor"

clean:
	bash -c "source $(IDF_PATH)/export.sh && idf.py clean"

fullclean:
	bash -c "source $(IDF_PATH)/export.sh && idf.py fullclean"
