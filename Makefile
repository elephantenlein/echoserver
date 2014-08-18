# Simple makefile
# Kirill Kupriyanov: elephantenlein@gmail.com

# Compiler & linker
LD = g++
CPPFLAGS = -c -Wall -march=i686 -mtune=i686

# Files
SOURCES = servers echo_server
BINARY  = echod

.PHONY: all clean cleanup

# Targets
all: $(BINARY)

$(BINARY): $(addsuffix .o, $(SOURCES))
	$(LD) $(LD_FLAGS) -o $@ $^

clean:
	@rm -f *.o

cleanup: clean
	@rm $(BINARY)

# Rules
# defaults Ok here
