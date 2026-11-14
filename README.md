A self-hosting functional Lisp whose compiler is written in itself and emits ARM64 Linux executables
directly. See [DESIGN.md](DESIGN.md) for the objectives and the design, and [NAMING.md](NAMING.md)
for the choice of a name.

Nothing runs yet: the interpreter and the compiler are stubs.


Layout
------

* `interpreter/`: the bootstrap interpreter for Lisp-0, in strict C99
* `compiler/`: the compiler, in Lisp-0
* `check/`: the checks that `make test` runs
* `build/`: what is built, per C compiler (e.g., `build/tcc/interpreter`)
* `Makefile`: build rules
* `Dockerfile.dev`, `docker-compose.yaml`: the dev container, which is arm64 Linux whatever the host
  is


Building
--------

On arm64 Linux, with `make`, `gcc` and `tcc`:

```shell
make test
```

This builds the interpreter with every C compiler in `COMPILERS` (`gcc tcc` by default) and checks
each build.

Anywhere else (macOS, for one), the same runs in the dev container, through Docker Compose:

```shell
make docker-test
```

`make docker-shell` opens a shell in that container, with the checkout mounted at `/mnt/lisp`. The
container also has QEMU: `qemu-system-aarch64`, the reference platform, and `qemu-aarch64`, to run
arm64 binaries on a host of another architecture.
