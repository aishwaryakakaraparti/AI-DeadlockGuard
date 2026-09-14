.PHONY: all clean monitor harness

all: monitor harness

monitor:
	$(MAKE) -C monitor

harness: monitor
	$(MAKE) -C harness

clean:
	$(MAKE) -C monitor clean
	$(MAKE) -C harness clean
