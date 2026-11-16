A self-hosting functional Lisp whose compiler is written in itself and emits ARM64 Linux and macOS
executables directly. See [DESIGN.md](DESIGN.md) for the objectives and the design, and
[NAMING.md](NAMING.md) for the choice of a name.

Nothing runs yet: the interpreter and the compiler are stubs.


Layout
------

* `interpreter/`: the bootstrap interpreter for Lisp-0, in strict C99
* `compiler/`: the compiler, in Lisp-0
* `check/`: the checks that `make test` runs
* `build/`: what is built, per C compiler (e.g., `build/tcc/interpreter`)
* `Makefile`: build rules
* `Dockerfile.dev`, `docker-compose.yaml`: dev container


Building
--------

On arm64 Linux or arm64 macOS, with `make`, `gcc` and `tcc`:

```shell
make test
```

This builds the interpreter with every C compiler in `COMPILERS` (`gcc tcc` by default) and checks
each build. On macOS, `gcc` is Apple's clang and `tcc` is not installed by default, so
`make test COMPILERS=gcc` builds without it.

On any other host, the same runs in the dev container, through Docker Compose:

```shell
make docker-test
```

`make docker-shell` opens a shell in that container, with the checkout mounted at `/mnt/lisp`. The
container also has QEMU: `qemu-system-aarch64`, the reference platform, and `qemu-aarch64`, to run
arm64 binaries on a host of another architecture.
