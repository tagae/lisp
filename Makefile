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
# every one of them must work: tcc is a requirement, not a convenience, because the bootstrap has to
# stay within reach of a C compiler far smaller than gcc.
COMPILERS ?= gcc tcc

# Strict C99 with no extensions. The flags only reject what the interpreter must not use: nothing in
# it relies on them.
CFLAGS ?= -std=c99 -pedantic -Wall -Wextra -Werror

interpreter_sources := $(wildcard interpreter/*.c)
interpreter_headers := $(wildcard interpreter/*.h)
interpreters := $(COMPILERS:%=build/%/interpreter)
lexer_checks := $(COMPILERS:%=build/%/check-lexer)

interpreter: $(interpreters)

build/%/interpreter: $(interpreter_sources) $(interpreter_headers)
	mkdir -p $(dir $@)
	$* $(CFLAGS) -o $@ $(interpreter_sources)

# The lexer's checks are a C program of their own, linked with the lexer.
build/%/check-lexer: check/lexer.c interpreter/lexer.c interpreter/lexer.h
	mkdir -p $(dir $@)
	$* $(CFLAGS) -o $@ check/lexer.c interpreter/lexer.c

test: interpreter $(lexer_checks)
	check/interpreter $(interpreters)
	set -e; for check in $(lexer_checks); do $$check; done

clean:
	rm -rf build/

#---===[ Docker ]===---

# Environment variables picked up automatically by Docker Compose.
.env:
	@echo HOST_UID=$$(id -u) >> $@
	@echo HOST_GID=$$(id -g) >> $@

docker-dev: .env
	docker compose run --build --rm dev bash

docker-clean: clean
	docker compose down --rmi local --remove-orphans
	rm -f .env
