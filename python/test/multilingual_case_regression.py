#!/usr/bin/env python3
"""Large multilingual differential regression for Marian case encoding."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile
import time
from typing import Callable, Iterable

FLORES_REPO = "openlanguagedata/flores_plus"
FLORES_REVISION = "b3a5298db5721c8a682e7ef00a37fcc9ab522757"
EXPECTED_DEV_FILES = 227
EXPECTED_DEVTEST_FILES = 221
CASED_SCRIPTS = {"Armn", "Cyrl", "Geor", "Grek", "Latn"}


@dataclass(frozen=True)
class Segment:
  name: str
  script: str
  lines: int


@dataclass(frozen=True)
class Tools:
  train: Path
  encode: Path
  decode: Path

  @classmethod
  def from_directory(cls, directory: Path) -> "Tools":
    def find(name: str) -> Path:
      for candidate in (directory / name, directory / f"{name}.exe"):
        if candidate.is_file():
          return candidate.resolve()
      raise FileNotFoundError(f"Cannot find {name} in {directory}")

    return cls(find("spm_train"), find("spm_encode"), find("spm_decode"))

  def version(self) -> str:
    result = subprocess.run(
        [self.train, "--version"],
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    return result.stdout.strip()


def sha256_file(path: Path) -> str:
  digest = hashlib.sha256()
  with path.open("rb") as stream:
    for block in iter(lambda: stream.read(1024 * 1024), b""):
      digest.update(block)
  return digest.hexdigest()


def run(command: list[os.PathLike[str] | str]) -> float:
  printable = [os.fspath(part) for part in command]
  print(f"+ {shlex.join(printable)}", flush=True)
  start = time.perf_counter()
  subprocess.run(printable, check=True)
  return time.perf_counter() - start


def download_flores() -> Path:
  try:
    from huggingface_hub import snapshot_download
  except ImportError as exc:
    raise RuntimeError(
        "Install huggingface_hub or pass --flores-dir to a pinned snapshot"
    ) from exc

  token = os.environ.get("HF_TOKEN")
  if not token:
    raise RuntimeError(
        "FLORES+ is gated. Accept its terms and set HF_TOKEN, or pass "
        "--flores-dir."
    )

  return Path(
      snapshot_download(
          repo_id=FLORES_REPO,
          repo_type="dataset",
          revision=FLORES_REVISION,
          allow_patterns=["dev/*.jsonl", "devtest/*.jsonl"],
          token=token,
      )
  )


def jsonl_texts(path: Path) -> Iterable[str]:
  with path.open("r", encoding="utf-8") as stream:
    for line_number, line in enumerate(stream, 1):
      record = json.loads(line)
      text = record["text"]
      if not isinstance(text, str):
        raise TypeError(f"{path}:{line_number}: text is not a string")
      if "\n" in text or "\r" in text:
        raise ValueError(f"{path}:{line_number}: embedded newline")
      yield text


def language_name(path: Path) -> str:
  return path.stem


def script_for(name: str) -> str:
  fields = name.split("_")
  if len(fields) < 2:
    raise ValueError(f"Missing script tag in {name}")
  return fields[1]


def corpus_files(root: Path, split: str) -> list[Path]:
  files = sorted((root / split).glob("*.jsonl"))
  if not files:
    raise FileNotFoundError(f"No {split} JSONL files below {root}")
  return files


def write_corpus(
    files: list[Path], output: Path, transform: Callable[[str], str]
) -> list[Segment]:
  segments: list[Segment] = []
  with output.open("w", encoding="utf-8", newline="\n") as destination:
    for path in files:
      count = 0
      for text in jsonl_texts(path):
        transformed = transform(text)
        if "\n" in transformed or "\r" in transformed:
          raise ValueError(f"Transformation introduced newline for {path}")
        destination.write(transformed)
        destination.write("\n")
        count += 1
      segments.append(Segment(language_name(path), script_for(path.stem), count))
  return segments


def alternating_case(text: str) -> str:
  uppercase_next = True
  output: list[str] = []
  for character in text:
    lower = character.lower()
    upper = character.upper()
    if lower != upper:
      output.append(upper if uppercase_next else lower)
      uppercase_next = not uppercase_next
    else:
      output.append(character)
  return "".join(output)


TRANSFORMS: dict[str, Callable[[str], str]] = {
    "original": lambda text: text,
    "uppercase": str.upper,
    "lowercase": str.lower,
    "title": str.title,
    "alternating": alternating_case,
}


def train_model(
    tools: Tools, corpus: Path, prefix: Path, vocab_size: int
) -> tuple[Path, float]:
  elapsed = run(
      [
          tools.train,
          f"--input={corpus}",
          f"--model_prefix={prefix}",
          f"--vocab_size={vocab_size}",
          "--model_type=unigram",
          "--character_coverage=1.0",
          "--hard_vocab_limit=false",
          "--input_sentence_size=0",
          "--shuffle_input_sentence=false",
          "--num_threads=1",
          "--byte_fallback=true",
          "--encode_unicode_case=true",
          "--treat_whitespace_as_suffix=true",
          "--bos_id=-1",
          "--eos_id=0",
          "--unk_id=1",
      ]
  )
  model = prefix.with_suffix(".model")
  if not model.is_file():
    raise FileNotFoundError(f"Training did not create {model}")
  return model, elapsed


def encode(
    tools: Tools, model: Path, input_path: Path, output_path: Path, format_: str
) -> float:
  return run(
      [
          tools.encode,
          f"--model={model}",
          f"--input={input_path}",
          f"--output={output_path}",
          f"--output_format={format_}",
      ]
  )


def decode(
    tools: Tools, model: Path, input_path: Path, output_path: Path
) -> float:
  return run(
      [
          tools.decode,
          f"--model={model}",
          f"--input={input_path}",
          f"--output={output_path}",
          "--input_format=id",
          "--output_format=string",
      ]
  )


def normalized_line(raw: bytes) -> bytes:
  if raw.endswith(b"\n"):
    raw = raw[:-1]
  if raw.endswith(b"\r"):
    raw = raw[:-1]
  return raw


def scan_segments(
    path: Path, segments: list[Segment], format_: str
) -> dict[str, dict[str, int | str | dict[str, int]]]:
  output: dict[str, dict[str, int | str | dict[str, int]]] = {}
  with path.open("rb") as stream:
    for segment in segments:
      digest = hashlib.sha256()
      token_count = 0
      unknown_count = 0
      byte_piece_count = 0
      marker_counts = {marker: 0 for marker in "ATUL"}
      for _ in range(segment.lines):
        raw = stream.readline()
        if not raw:
          raise ValueError(f"{path} ended inside {segment.name}")
        raw.decode("utf-8", errors="strict")
        digest.update(raw)
        tokens = normalized_line(raw).split()
        token_count += len(tokens)
        if format_ == "id":
          unknown_count += sum(token == b"1" for token in tokens)
        elif format_ == "piece":
          for token in tokens:
            if token.startswith(b"<0x") and token.endswith(b">"):
              byte_piece_count += 1
            piece = token.decode("utf-8")
            if piece and piece[0] in marker_counts:
              marker_counts[piece[0]] += 1
      output[segment.name] = {
          "sha256": digest.hexdigest(),
          "lines": segment.lines,
          "tokens": token_count,
          "unknowns": unknown_count,
          "byte_pieces": byte_piece_count,
          "marker_prefixes": marker_counts,
      }
    if stream.readline():
      raise ValueError(f"{path} has more lines than the manifest")
  return output


def compare_text(
    expected_path: Path, actual_path: Path, segments: list[Segment]
) -> dict[str, dict[str, int | str]]:
  output: dict[str, dict[str, int | str]] = {}
  with expected_path.open("rb") as expected, actual_path.open("rb") as actual:
    for segment in segments:
      digest = hashlib.sha256()
      mismatches = 0
      for _ in range(segment.lines):
        expected_line = expected.readline()
        actual_line = actual.readline()
        if not expected_line or not actual_line:
          raise ValueError(f"Line count mismatch inside {segment.name}")
        actual_line.decode("utf-8", errors="strict")
        digest.update(actual_line)
        if normalized_line(expected_line) != normalized_line(actual_line):
          mismatches += 1
      output[segment.name] = {
          "sha256": digest.hexdigest(),
          "lines": segment.lines,
          "mismatches": mismatches,
      }
    if expected.readline() or actual.readline():
      raise ValueError("Decoded or expected output exceeds the manifest")
  return output


def aggregate_by_script(
    segments: list[Segment], language_metrics: dict[str, dict[str, object]]
) -> dict[str, dict[str, int | float]]:
  aggregate: dict[str, dict[str, int | float]] = {}
  for segment in segments:
    item = aggregate.setdefault(
        segment.script,
        {"varieties": 0, "lines": 0, "tokens": 0, "mismatches": 0},
    )
    item["varieties"] += 1
    item["lines"] += segment.lines
    item["tokens"] += int(language_metrics[segment.name]["tokens"])
    item["mismatches"] += int(language_metrics[segment.name]["mismatches"])
  for item in aggregate.values():
    lines = int(item["lines"])
    item["tokens_per_line"] = item["tokens"] / lines if lines else 0.0
  return dict(sorted(aggregate.items()))


def evaluate_variant(
    name: str,
    transform: Callable[[str], str],
    files: list[Path],
    run_dir: Path,
    baseline_tools: Tools,
    candidate_tools: Tools,
    baseline_model: Path,
    candidate_model: Path,
) -> tuple[dict[str, object], list[str]]:
  failures: list[str] = []
  input_path = run_dir / f"{name}.txt"
  segments = write_corpus(files, input_path, transform)

  paths = {
      "baseline_ids": run_dir / f"{name}.baseline.ids",
      "baseline_pieces": run_dir / f"{name}.baseline.pieces",
      "baseline_decoded": run_dir / f"{name}.baseline.decoded",
      "compat_ids": run_dir / f"{name}.compat.ids",
      "compat_pieces": run_dir / f"{name}.compat.pieces",
      "compat_decoded": run_dir / f"{name}.compat.decoded",
      "candidate_ids": run_dir / f"{name}.candidate.ids",
      "candidate_pieces": run_dir / f"{name}.candidate.pieces",
      "candidate_decoded": run_dir / f"{name}.candidate.decoded",
  }

  timings = {
      "baseline_encode_ids": encode(
          baseline_tools, baseline_model, input_path, paths["baseline_ids"], "id"
      ),
      "baseline_encode_pieces": encode(
          baseline_tools,
          baseline_model,
          input_path,
          paths["baseline_pieces"],
          "piece",
      ),
      "baseline_decode": decode(
          baseline_tools,
          baseline_model,
          paths["baseline_ids"],
          paths["baseline_decoded"],
      ),
      "compat_encode_ids": encode(
          candidate_tools, baseline_model, input_path, paths["compat_ids"], "id"
      ),
      "compat_encode_pieces": encode(
          candidate_tools,
          baseline_model,
          input_path,
          paths["compat_pieces"],
          "piece",
      ),
      "compat_decode": decode(
          candidate_tools,
          baseline_model,
          paths["compat_ids"],
          paths["compat_decoded"],
      ),
      "candidate_encode_ids": encode(
          candidate_tools, candidate_model, input_path, paths["candidate_ids"], "id"
      ),
      "candidate_encode_pieces": encode(
          candidate_tools,
          candidate_model,
          input_path,
          paths["candidate_pieces"],
          "piece",
      ),
      "candidate_decode": decode(
          candidate_tools,
          candidate_model,
          paths["candidate_ids"],
          paths["candidate_decoded"],
      ),
  }

  compatibility: dict[str, bool] = {}
  for metric in ("ids", "pieces", "decoded"):
    baseline_path = paths[f"baseline_{metric}"]
    candidate_path = paths[f"compat_{metric}"]
    equal = sha256_file(baseline_path) == sha256_file(candidate_path)
    compatibility[f"{metric}_equal"] = equal
    if not equal:
      failures.append(f"{name}: legacy-model {metric} differ")

  baseline_ids = scan_segments(paths["baseline_ids"], segments, "id")
  baseline_pieces = scan_segments(paths["baseline_pieces"], segments, "piece")
  baseline_text = compare_text(input_path, paths["baseline_decoded"], segments)
  candidate_ids = scan_segments(paths["candidate_ids"], segments, "id")
  candidate_pieces = scan_segments(paths["candidate_pieces"], segments, "piece")
  candidate_text = compare_text(input_path, paths["candidate_decoded"], segments)

  languages: dict[str, dict[str, object]] = {}
  for segment in segments:
    baseline_tokens = int(baseline_ids[segment.name]["tokens"])
    fresh_tokens = int(candidate_ids[segment.name]["tokens"])
    languages[segment.name] = {
        "script": segment.script,
        "cased_script": segment.script in CASED_SCRIPTS,
        "lines": segment.lines,
        "baseline_ids": baseline_ids[segment.name],
        "baseline_pieces": baseline_pieces[segment.name],
        "baseline_roundtrip": baseline_text[segment.name],
        "candidate_ids": candidate_ids[segment.name],
        "candidate_pieces": candidate_pieces[segment.name],
        "candidate_roundtrip": candidate_text[segment.name],
        "candidate_to_baseline_token_ratio": (
            fresh_tokens / baseline_tokens if baseline_tokens else 0.0
        ),
    }

  candidate_aggregate_input = {
      language: {
          "tokens": metrics["candidate_ids"]["tokens"],
          "mismatches": metrics["candidate_roundtrip"]["mismatches"],
      }
      for language, metrics in languages.items()
  }

  return (
      {
          "input_sha256": sha256_file(input_path),
          "line_count": sum(segment.lines for segment in segments),
          "compatibility": compatibility,
          "timings_seconds": timings,
          "languages": languages,
          "candidate_by_script": aggregate_by_script(
              segments, candidate_aggregate_input
          ),
      },
      failures,
  )


def parse_args() -> argparse.Namespace:
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument("--flores-dir", type=Path)
  parser.add_argument("--baseline-bin-dir", type=Path, required=True)
  parser.add_argument("--candidate-bin-dir", type=Path, required=True)
  parser.add_argument("--baseline-model", type=Path)
  parser.add_argument("--work-root", type=Path, default=Path(tempfile.gettempdir()))
  parser.add_argument("--report", type=Path)
  parser.add_argument("--vocab-size", type=int, default=32000)
  parser.add_argument(
      "--variants",
      nargs="+",
      choices=sorted(TRANSFORMS),
      default=list(TRANSFORMS),
  )
  parser.add_argument(
      "--allow-subset",
      action="store_true",
      help="Allow a local smoke corpus instead of enforcing the pinned file counts",
  )
  parser.add_argument(
      "--max-roundtrip-mismatches",
      type=int,
      default=0,
      help="Maximum total fresh-model mismatches across all variants",
  )
  return parser.parse_args()


def main() -> int:
  args = parse_args()
  flores_dir = args.flores_dir.resolve() if args.flores_dir else download_flores()
  baseline_tools = Tools.from_directory(args.baseline_bin_dir.resolve())
  candidate_tools = Tools.from_directory(args.candidate_bin_dir.resolve())
  dev_files = corpus_files(flores_dir, "dev")
  devtest_files = corpus_files(flores_dir, "devtest")

  if not args.allow_subset:
    if len(dev_files) != EXPECTED_DEV_FILES:
      raise ValueError(
          f"Pinned FLORES+ expected {EXPECTED_DEV_FILES} dev files, found "
          f"{len(dev_files)}"
      )
    if len(devtest_files) != EXPECTED_DEVTEST_FILES:
      raise ValueError(
          f"Pinned FLORES+ expected {EXPECTED_DEVTEST_FILES} devtest files, "
          f"found {len(devtest_files)}"
      )

  args.work_root.mkdir(parents=True, exist_ok=True)
  run_dir = Path(tempfile.mkdtemp(prefix="spm-flores-case-", dir=args.work_root))
  print(f"Run directory: {run_dir}")

  training_corpus = run_dir / "flores-dev.txt"
  training_segments = write_corpus(dev_files, training_corpus, TRANSFORMS["original"])

  if args.baseline_model:
    baseline_model = args.baseline_model.resolve()
    baseline_training_seconds = None
  else:
    baseline_model, baseline_training_seconds = train_model(
        baseline_tools,
        training_corpus,
        run_dir / "baseline-case",
        args.vocab_size,
    )

  candidate_model, candidate_training_seconds = train_model(
      candidate_tools,
      training_corpus,
      run_dir / "candidate-case",
      args.vocab_size,
  )

  report: dict[str, object] = {
      "dataset": {
          "repo": FLORES_REPO,
          "revision": FLORES_REVISION,
          "path": os.fspath(flores_dir),
          "dev_files": len(dev_files),
          "devtest_files": len(devtest_files),
          "training_lines": sum(segment.lines for segment in training_segments),
          "training_corpus_sha256": sha256_file(training_corpus),
      },
      "baseline": {
          "version": baseline_tools.version(),
          "model": os.fspath(baseline_model),
          "model_sha256": sha256_file(baseline_model),
          "training_seconds": baseline_training_seconds,
      },
      "candidate": {
          "version": candidate_tools.version(),
          "model": os.fspath(candidate_model),
          "model_sha256": sha256_file(candidate_model),
          "training_seconds": candidate_training_seconds,
      },
      "variants": {},
  }

  failures: list[str] = []
  total_roundtrip_mismatches = 0
  for variant in args.variants:
    variant_report, variant_failures = evaluate_variant(
        variant,
        TRANSFORMS[variant],
        devtest_files,
        run_dir,
        baseline_tools,
        candidate_tools,
        baseline_model,
        candidate_model,
    )
    report["variants"][variant] = variant_report
    failures.extend(variant_failures)
    for metrics in variant_report["languages"].values():
      total_roundtrip_mismatches += int(
          metrics["candidate_roundtrip"]["mismatches"]
      )

  report["total_candidate_roundtrip_mismatches"] = total_roundtrip_mismatches
  if total_roundtrip_mismatches > args.max_roundtrip_mismatches:
    failures.append(
        "fresh-model round-trip mismatches "
        f"{total_roundtrip_mismatches} > {args.max_roundtrip_mismatches}"
    )
  report["failures"] = failures

  report_path = args.report.resolve() if args.report else run_dir / "report.json"
  report_path.parent.mkdir(parents=True, exist_ok=True)
  with report_path.open("w", encoding="utf-8") as stream:
    json.dump(report, stream, ensure_ascii=False, indent=2, sort_keys=True)
    stream.write("\n")
  print(f"Report: {report_path}")

  if failures:
    for failure in failures:
      print(f"FAIL: {failure}", file=sys.stderr)
    return 1
  print("All multilingual case regression gates passed")
  return 0


if __name__ == "__main__":
  try:
    raise SystemExit(main())
  except (FileNotFoundError, RuntimeError, ValueError) as error:
    print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(2) from error
