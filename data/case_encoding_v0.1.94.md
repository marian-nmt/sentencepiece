# Case encoding compatibility fixture

`case_encoding_v0.1.94.model` was generated from
`case_encoding_corpus.txt` with Marian SentencePiece commit
`aef9e2bd0dd1f18d9ac8555a5fef38669e0d3609`:

```sh
spm_train \
  --input=case_encoding_corpus.txt \
  --model_prefix=case_encoding_v0.1.94 \
  --vocab_size=128 \
  --model_type=unigram \
  --character_coverage=1.0 \
  --hard_vocab_limit=false \
  --input_sentence_size=0 \
  --shuffle_input_sentence=false \
  --num_threads=1 \
  --encode_unicode_case \
  --treat_whitespace_as_suffix \
  --bos_id=-1 \
  --eos_id=0 \
  --unk_id=1
```

The binary model is intentionally checked in. It protects the protobuf wire
fields, token IDs, case decoding, whitespace-suffix behavior, and structured
piece surface/offset ownership across SentencePiece upgrades.
