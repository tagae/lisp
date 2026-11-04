# Naming

Choosing the language's name. The full list of candidates is in the
alphabetical index. This document narrows the ★ shortlist down to one name and
records the decision.

**Status:** open.

## What needs a name

The project has more than one thing to name, and the design's two axes suggest
a scheme:

| Thing | Today | Proposed |
|---|---|---|
| The language (the accepted, growing axis) | — | **⟨Name⟩** |
| The frozen subset the compiler is written in | Lisp-0 | **⟨Name⟩-0** |
| Command | — | `⟨name⟩`, lower case |
| Source file extension | `.lisp` | `.⟨name⟩`, or a 2–3 letter abbreviation |
| Bootstrap stages | lc1, lc2, lc3 | unchanged, or ⟨n⟩c1, ⟨n⟩c2, ⟨n⟩c3 |

The suffix `-0` keeps what "Lisp-0" already says well: the frozen language is
generation zero of this language, not a separate one. A second, independent
name for the core (Kern was a candidate for that) would suggest two languages
where there is one language and its frozen subset.

## Criteria

Hard filters. A name that fails any of these is out:

1. **No live clash in the same niche.** No existing language, Lisp, compiler
   or bootstrapping project uses the name. A clash in an unrelated field
   (music, CNC machining) is acceptable.
2. **Nothing unfortunate in English or Spanish.** Many of the candidates are
   Spanish words, so slang meanings in Spain and Latin America count.
3. **Works as a command.** Short, lower case, no clash with a common Linux
   command.

Soft criteria, used to rank what survives:

4. **Fits the project.** The best names describe what is unusual here: the
   compiler compiles itself, there is nothing underneath, and the language
   grows from a frozen core.
5. **Pleasant to type for years.** Four or five letters, ideally alternating
   hands.
6. **Searchable.** "⟨name⟩ lisp" and "⟨name⟩ language" should find this
   project once it exists.

Package registries such as crates.io and npm don't matter here: the toolchain
has no ecosystem dependencies, and the language will never be published
through one. Bare GitHub logins don't decide anything either, since every
shortlisted name is already taken as a GitHub user or organization (checked
November 2026). The repository can stay `tagae/lisp`, or move to something
like `⟨name⟩-lang`.

## The shortlist

Checked November 2026. "Verified" findings come from the web and GitHub
searches cited under [Sources](#sources). Findings marked *(unverified)* are
from memory and need confirming before they decide anything.

| Name | Length | Clashes found | Verdict |
|---|---|---|---|
| **Ouro** | 4 | An early-stage "OuroLang" graphics language exists, with little visible activity. Portuguese and Galician for *gold*. | **Keep.** |
| **Filo** | 4 | A small Go scripting language named Filo (`crgimenes/filo`). Also filo pastry. | **Keep**, weakly. |
| **Fragua** | 6 | Nothing found. | **Keep.** |
| **Lumbre** | 6 | Nothing found. | **Keep.** |
| Yesca | 5 | Nothing in software, but *yesca* is common slang for marijuana in Mexico and parts of Latin America *(unverified)*. | Out by criterion 2, unless you're fine with that reading. |
| Kern | 4 | Humdrum `**kern` is a well-known music notation format. John Shutt's *Kernel* is a fexpr-based Lisp *(unverified)*, which is exactly the design this project rejected, and "Kern" reads as short for it. | Out by criterion 1. |
| Klein | 5 | Sun Labs' Klein was a metacircular, self-hosting VM for Self: the same niche and the same idea. A `klein-language` GitHub organization also exists. Twisted's Klein web framework *(unverified)*. | Out by criterion 1. |
| Nacre | 5 | `aerkiaga/nacre` is a programming language for verified software, written in Rust and described as being in an early bootstrapping phase. | Out by criterion 1. |
| Weft | 4 | WeaveMindAI's Weft is an active language for AI systems, with a rebuild planned for release in August 2026. | Out by criterion 1. |
| Graft | 5 | Several developer tools: an AI coding-context tool from NanoNets (2026), an OCaml package, and the long-standing `graft` package-management utility *(unverified)*. | Out by criterion 3: a likely command clash. |
| Lathe | 5 | Several developer tools named Lathe (tutorial generator, CLI generator, genome tooling). Ross Angle's Lathe utility libraries for Arc, JavaScript and Racket *(unverified)* would be a Lisp-niche clash. | Out, mainly by criterion 1. |
| Rime | 4 | RIME (librime) is a widely used Chinese input method engine. Searching "rime language" finds its input-schema repositories. | Out by criterion 6. |
| Cambium | 7 | Cambium Networks, a networking company *(unverified)*, plus a UCI research modelling language. Long to type. | Out by criteria 5 and 6. |

### The four survivors

**Ouro.** Ouroboros, the snake eating its own tail: a compiler that compiles
itself, and a fixed point `lc1 == lc2` that closes the loop. It names the
project's central property rather than a mood. It is four letters and easy to
type (`ouro build compiler.ouro`), `.ouro` has no known use, and *gold* in
Portuguese is a pleasant second reading. The existing OuroLang project is the
one thing to watch.

**Fragua.** Spanish for *forge*: the place where tools are made, which is what
a compiler written in itself does. `fragua build` reads well. The weaknesses
are length (six letters, and `gu` is awkward to type) and that English
speakers may not know how to say it.

**Lumbre.** The small fire you light other fires from, which is the bootstrap
chain exactly: the interpreter lights lc1 once and is never needed again. It
is unusual in software. Like Fragua, it is six letters long and opaque to
non-Spanish speakers.

**Filo.** *Edge* or *blade* in Spanish, *thread* in Italian. It is the nicest
of the four to type, but it says the least about the project, and an existing
small scripting language already uses it.

## Recommendation

**Ouro**, with Lumbre as the runner-up.

Ouro is the only survivor that describes the project itself rather than a
metaphor around it, and it is the shortest. It also works well with the
naming scheme: Ouro-0 for the frozen core reads naturally as "the loop at
generation zero".

Lumbre is the better choice if you would rather the name describe the
bootstrap chain than self-hosting, and don't mind six letters.

## How to decide

1. Confirm the *(unverified)* findings for any name you still want to keep.
2. Search for "⟨name⟩ language" and "⟨name⟩ lisp" one more time, in case
   something new has appeared.
3. On the Pi, check that nothing else is installed under the command name:
   `command -v ⟨name⟩` and `apt-cache policy ⟨name⟩`.
4. Type `⟨name⟩ build compiler.⟨ext⟩` a few times, and keep whichever you
   don't mind typing on a bad day.
5. Record the decision below. Then rename Lisp-0 to ⟨Name⟩-0 in
   [DESIGN.md](DESIGN.md), and decide whether the stage binaries keep the
   `lc` prefix.

## Decision

- **Name:** —
- **Frozen subset:** —
- **Command:** —
- **File extension:** —
- **Decided on:** —
- **Why:** —

## Sources

- OuroLang: [OuroLang syntax documentation](https://donalddarkness.github.io/lava)
- Filo: [github.com/crgimenes/filo on pkg.go.dev](https://pkg.go.dev/github.com/crgimenes/filo)
- Kern: [Humdrum \*\*kern](https://kernpy.pages.dev/concepts/humdrum-kern), [KernScores](https://github.com/hleveillegauvin/KernScores)
- Klein: [OOPSLA 2005, "Constructing a Metacircular Virtual Machine in an Exploratory Programming Environment"](https://www.doi.org/10.1145/1094855.1094865), [Live Objects All The Way Down (2023)](https://arxiv.org/pdf/2312.16973)
- Nacre: [aerkiaga/nacre](https://awesome.ecosyste.ms/projects/github.com%2Faerkiaga%2Fnacre)
- Weft: [WeaveMindAI/weft](https://github.com/WeaveMindAI/weft)
- Graft: [NanoNets/Graft](https://github.com/NanoNets/Graft), [graft on opam](https://opam-4.ocaml.org/packages/graft)
- Lathe: [lathe-cli/lathe](https://github.com/lathe-cli/lathe), [devenjarvis/lathe](https://github.com/devenjarvis/lathe)
- Rime: [librime](https://github.com/biaoyinzi/librime), [librime in Debian](https://packages.debian.org/sid/ppc64el/librime1t64)
- Cambium: [Cambium-Plenum Translator (UCI)](https://computableplant.ics.uci.edu/sw/CambiumPlenum/)
