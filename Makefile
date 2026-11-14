.PHONY: \
	interpreter \
	test \
	clean \
	docker-interpreter \
	docker-test \
	docker-shell \
	docker-clean

.DEFAULT_GOAL := test

# The C compilers that build the interpreter. Each gets a build of its own, build/<compiler>/, and
# every one of them must work: tcc is a requirement, not a convenience (DESIGN.md).
COMPILERS ?= gcc tcc

# Strict C99 with no extensions. The flags only reject what the interpreter must not use: nothing in
# it relies on them.
CFLAGS ?= -std=c99 -pedantic -Wall -Wextra -Werror

interpreter_sources := $(wildcard interpreter/*.c)
interpreter_headers := $(wildcard interpreter/*.h)
interpreters := $(COMPILERS:%=build/%/interpreter)

interpreter: $(interpreters)

build/%/interpreter: $(interpreter_sources) $(interpreter_headers)
	mkdir -p $(dir $@)
	$* $(CFLAGS) -o $@ $(interpreter_sources)

test: interpreter
	check/interpreter $(interpreters)

clean:
	rm -rf build/

#---===[ Docker ]===---

# Environment variables picked up automatically by Docker Compose.
.env:
	@echo HOST_UID=$$(id -u) >> $@
	@echo HOST_GID=$$(id -g) >> $@

# The same rules in the dev container, which is arm64 Linux whatever the host is.
docker-interpreter docker-test: docker-%: .env
	docker compose run --build --rm dev make $*

# A shell in the dev container.
docker-shell: .env
	docker compose run --build --rm dev bash

docker-clean: clean
	docker compose down --rmi local --remove-orphans
	rm -f .env
