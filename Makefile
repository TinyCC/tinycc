SRC_DIR = src

.PHONY: default
default:
    $(MAKE) -C $(SRC_DIR)

clean:
    $(MAKE) -C $(SRC_DIR) clean
