# Multilingual case-encoding regression

The large regression uses FLORES+ 4.6 pinned to revision
`b3a5298db5721c8a682e7ef00a37fcc9ab522757`.

- Training: all 227 `dev` language-variety files, 997 lines each.
- Evaluation: all 221 `devtest` files, 1,012 lines each.
- Complete pairs: 218 varieties, 205 ISO languages, and 30 scripts.
- Cased scripts: Latin, Cyrillic, Greek, Armenian, and Georgian.
- Variants: original, uppercase, lowercase, title case, and alternating case.

The runner trains the legacy and candidate implementations on the balanced
`dev` corpus. It then requires IDs, pieces, and decoded strings produced by the
candidate with the legacy model to exactly match the legacy binaries. A freshly
trained candidate model is checked for UTF-8 validity and exact round trips.
The JSON report records hashes, token counts, unknown and byte-piece counts,
case-marker counts, timing, and per-language/per-script distributions.

FLORES+ is gated. Accept its terms and expose a read-only Hugging Face token as
`HF_TOKEN`; the token is never printed. The runner can also consume an existing
snapshot with `--flores-dir`.

```bash
python python/test/multilingual_case_regression.py \
  --baseline-bin-dir=/path/to/v0.1.94/bin \
  --candidate-bin-dir=/path/to/v0.2.2-case/bin
```

For a local subset smoke test, pass `--flores-dir` and `--allow-subset`. The
full pinned counts remain mandatory when `--allow-subset` is absent.

The large run belongs in credentialed merge or nightly CI. The checked-in C++
unit tests provide deterministic, credential-free PR coverage of the state
machine, legacy model, structured offsets, and mixed scripts.
