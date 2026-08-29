#!/usr/bin/env python3
# AI pre-created code: https://github.com/LBJ-code/ahc-library
"""Expand local C/C++ includes into one deterministic AtCoder submission file.

Only ``#include "..."`` directives are expanded.  System includes such as
``#include <bits/stdc++.h>`` are left untouched.  The tool intentionally uses
simple textual rules: it does not run a C preprocessor and does not need a
compiler or third-party Python package.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import re
from pathlib import Path
import subprocess
import sys
from typing import Iterable, Sequence


LOCAL_INCLUDE_RE = re.compile(
    r"^[ \t]*#[ \t]*include[ \t]*\"(?P<name>[^\"\r\n]+)\"[ \t]*(?://.*|/\*.*\*/[ \t]*)?$"
)
PRAGMA_ONCE_RE = re.compile(
    r"^[ \t]*#[ \t]*pragma[ \t]+once[ \t]*(?://.*|/\*.*\*/[ \t]*)?$"
)
GITHUB_URL_RE = re.compile(
    r"https://github\.com/[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+(?:/[A-Za-z0-9_.\-./]*)?"
)
TRAILING_URL_PUNCTUATION = ".,;:)]}>\"'"


class AmalgamationError(RuntimeError):
    """Base class for errors that should be shown as concise CLI diagnostics."""


class IncludeNotFoundError(AmalgamationError):
    """A quoted include could not be resolved relative to the include stack."""


class IncludeCycleError(AmalgamationError):
    """A quoted include recursively referred to an active source file."""


@dataclass(frozen=True)
class SourceInfo:
    """Stable provenance information for one expanded source file."""

    path: Path
    display_path: str
    github_url: str | None
    commit: str | None


def _normalise_github_url(url: str) -> str | None:
    """Return a canonical GitHub repository URL, or ``None`` for other URLs."""

    match = GITHUB_URL_RE.search(url)
    if match is None:
        return None
    value = match.group(0).rstrip(TRAILING_URL_PUNCTUATION).rstrip("/")
    if value.endswith(".git"):
        value = value[:-4]
    return value


class Amalgamator:
    """Stateful include expander.

    ``include_dirs`` are searched after the directory containing the including
    file.  Repeated canonical paths are emitted once.  A path encountered
    while it is already active is an error even if it has an include guard;
    this catches accidental cycles before an AtCoder submission is made.
    """

    def __init__(
        self,
        include_dirs: Iterable[str | Path] = (),
        *,
        repo_root: str | Path | None = None,
        provenance: bool = True,
    ) -> None:
        self.include_dirs = tuple(self._canonical_path(Path(directory)) for directory in include_dirs)
        self.repo_root = self._canonical_path(Path(repo_root)) if repo_root is not None else None
        self.provenance = provenance
        self._visited: set[Path] = set()
        self._active: list[Path] = []
        self._sources: dict[Path, SourceInfo] = {}
        self._git_dir: Path | None = None
        self._git_remote: str | None = None

    @staticmethod
    def _canonical_path(path: Path) -> Path:
        return path.expanduser().resolve(strict=False)

    @staticmethod
    def _read(path: Path) -> str:
        try:
            return path.read_text(encoding="utf-8")
        except UnicodeDecodeError as error:
            raise AmalgamationError(f"source is not UTF-8: {path}") from error

    def amalgamate(self, input_path: str | Path) -> str:
        """Return one C++ translation unit for ``input_path``."""

        root = self._canonical_path(Path(input_path))
        if not root.is_file():
            raise AmalgamationError(f"input file does not exist: {input_path}")
        if self.repo_root is None:
            self.repo_root = self._discover_repo_root(root)
        self._prepare_git_metadata()
        self._visited.clear()
        self._active.clear()
        self._sources.clear()

        expanded = self._expand(root)
        result: list[str] = []
        if self.provenance:
            result.extend(self._header_comments(root))
        result.extend(expanded)
        # A single final newline makes output independent of whether the input
        # happened to have one and avoids concatenating with a shell prompt.
        return "".join(result).rstrip("\n") + "\n"

    def write(self, input_path: str | Path, output_path: str | Path) -> None:
        """Amalgamate and write UTF-8 text to ``output_path``."""

        output = self.amalgamate(input_path)
        destination = Path(output_path)
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text(output, encoding="utf-8", newline="\n")

    def sources(self) -> tuple[SourceInfo, ...]:
        """Return expanded files in deterministic traversal order."""

        return tuple(self._sources.values())

    def _discover_repo_root(self, input_path: Path) -> Path | None:
        for parent in (input_path.parent, *input_path.parents):
            if self._find_git_dir(parent) is not None:
                return parent
        return None

    @staticmethod
    def _find_git_dir(root: Path) -> Path | None:
        # .git-local is used by this repository's sandbox, while .git is the
        # normal layout.  Ignore an empty placeholder .git directory.
        for name in (".git", ".git-local"):
            candidate = root / name
            if candidate.is_file():
                return candidate
            if candidate.is_dir() and (candidate / "HEAD").is_file():
                return candidate
        return None

    def _prepare_git_metadata(self) -> None:
        if self.repo_root is None:
            return
        self._git_dir = self._find_git_dir(self.repo_root)
        if self._git_dir is None:
            return
        try:
            completed = subprocess.run(
                ["git", f"--git-dir={self._git_dir}", f"--work-tree={self.repo_root}", "config", "--get", "remote.origin.url"],
                check=False,
                capture_output=True,
                text=True,
            )
        except OSError:
            return
        if completed.returncode == 0:
            self._git_remote = _normalise_github_url(completed.stdout.strip())

    def _git_commit(self, path: Path) -> str | None:
        if self.repo_root is None or self._git_dir is None:
            return None
        try:
            relative = path.relative_to(self.repo_root).as_posix()
        except ValueError:
            return None
        try:
            completed = subprocess.run(
                [
                    "git",
                    f"--git-dir={self._git_dir}",
                    f"--work-tree={self.repo_root}",
                    "log",
                    "-1",
                    "--format=%H",
                    "--",
                    relative,
                ],
                check=False,
                capture_output=True,
                text=True,
            )
        except OSError:
            return None
        commit = completed.stdout.strip()
        if completed.returncode == 0 and re.fullmatch(r"[0-9a-fA-F]{7,64}", commit):
            return commit
        # A newly-created file has no path-specific commit.  A repository HEAD
        # is still useful provenance, if available.
        try:
            completed = subprocess.run(
                ["git", f"--git-dir={self._git_dir}", f"--work-tree={self.repo_root}", "rev-parse", "HEAD"],
                check=False,
                capture_output=True,
                text=True,
            )
        except OSError:
            return None
        commit = completed.stdout.strip()
        return commit if completed.returncode == 0 and re.fullmatch(r"[0-9a-fA-F]{7,64}", commit) else None

    def _display_path(self, path: Path) -> str:
        if self.repo_root is not None:
            try:
                return path.relative_to(self.repo_root).as_posix()
            except ValueError:
                pass
        return path.as_posix()

    def _source_info(self, path: Path, text: str) -> SourceInfo:
        explicit_url = None
        for match in GITHUB_URL_RE.finditer(text):
            explicit_url = _normalise_github_url(match.group(0))
            if explicit_url is not None:
                break
        return SourceInfo(
            path=path,
            display_path=self._display_path(path),
            github_url=explicit_url or self._git_remote,
            commit=self._git_commit(path),
        )

    def _resolve_include(self, including: Path, include_name: str) -> Path:
        name = Path(include_name)
        candidates = [including.parent / name]
        candidates.extend(directory / name for directory in self.include_dirs)
        for candidate in candidates:
            canonical = self._canonical_path(candidate)
            if canonical.is_file():
                return canonical
        stack = " -> ".join(self._display_path(path) for path in (*self._active, including))
        raise IncludeNotFoundError(
            f'cannot resolve local include "{include_name}" from {self._display_path(including)}'
            + (f" (include stack: {stack})" if stack else "")
        )

    @staticmethod
    def _line_with_newline(line: str) -> str:
        return line.rstrip("\r\n") + "\n"

    def _expand(self, path: Path) -> list[str]:
        if path in self._active:
            cycle_start = self._active.index(path)
            cycle = self._active[cycle_start:] + [path]
            rendered = " -> ".join(self._display_path(item) for item in cycle)
            raise IncludeCycleError(f"local include cycle detected: {rendered}")
        if path in self._visited:
            return []

        self._active.append(path)
        text = self._read(path)
        info = self._source_info(path, text)
        self._sources[path] = info
        output: list[str] = []
        if self.provenance:
            output.append(f"// BEGIN INCLUDED: {info.display_path}\n")
            if info.github_url:
                output.append(f"// Source: {info.github_url}\n")
            if info.commit:
                output.append(f"// Git commit: {info.commit}\n")

        try:
            for line in text.splitlines():
                if PRAGMA_ONCE_RE.match(line) is not None:
                    if self.provenance:
                        output.append("// #pragma once omitted by amalgamate.py\n")
                    continue
                match = LOCAL_INCLUDE_RE.match(line)
                if match is None:
                    output.append(self._line_with_newline(line))
                    continue
                included = self._resolve_include(path, match.group("name").strip())
                if included in self._active:
                    # Let _expand produce the complete, readable cycle trace.
                    output.extend(self._expand(included))
                elif included in self._visited:
                    if self.provenance:
                        output.append(f"// Duplicate local include omitted: {self._display_path(included)}\n")
                else:
                    output.extend(self._expand(included))
        finally:
            self._active.pop()
        self._visited.add(path)
        if self.provenance:
            output.append(f"// END INCLUDED: {info.display_path}\n")
        return output

    def _header_comments(self, root: Path) -> list[str]:
        # `_expand` has collected sources in DFS order.  Sorting the summary
        # makes it stable even if an implementation later changes traversal.
        root_display = self._display_path(root)
        lines = [
            "// Generated by tools/amalgamate.py; do not edit this file.",
            f"// Root source: {root_display}",
            "// Local quoted includes were recursively expanded.",
            "//",
            "// Libraries used (GitHub URL and source commit when available):",
        ]
        libraries: dict[str, str | None] = {}
        for info in self._sources.values():
            if info.github_url is not None:
                previous = libraries.get(info.github_url)
                if previous is None or (info.commit is not None and info.commit < previous):
                    libraries[info.github_url] = info.commit
        for url in sorted(libraries):
            commit = libraries[url]
            suffix = f" (commit {commit})" if commit else " (commit unavailable)"
            lines.append(f"// - {url}{suffix}")
        lines.append("")
        return [line + "\n" for line in lines]


def amalgamate(
    input_path: str | Path,
    *,
    include_dirs: Iterable[str | Path] = (),
    repo_root: str | Path | None = None,
    provenance: bool = True,
) -> str:
    """Convenience wrapper around :class:`Amalgamator`."""

    return Amalgamator(
        include_dirs=include_dirs,
        repo_root=repo_root,
        provenance=provenance,
    ).amalgamate(input_path)


def build_argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Recursively expand local C/C++ #include \"...\" directives."
    )
    parser.add_argument("input", type=Path, help="root .cpp/.cc/.hpp file")
    parser.add_argument("-o", "--output", type=Path, help="output file (default: stdout)")
    parser.add_argument(
        "-I",
        "--include-dir",
        dest="include_dirs",
        action="append",
        type=Path,
        default=[],
        help="additional local include directory; may be repeated",
    )
    parser.add_argument(
        "--repo-root",
        type=Path,
        help="repository root used for provenance (auto-detected when omitted)",
    )
    parser.add_argument(
        "--no-provenance",
        action="store_true",
        help="omit generated provenance and begin/end comments",
    )
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_argument_parser().parse_args(argv)
    try:
        amalgamator = Amalgamator(
            include_dirs=args.include_dirs,
            repo_root=args.repo_root,
            provenance=not args.no_provenance,
        )
        output = amalgamator.amalgamate(args.input)
        if args.output is None:
            sys.stdout.write(output)
        else:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(output, encoding="utf-8", newline="\n")
    except (AmalgamationError, OSError) as error:
        print(f"amalgamate.py: error: {error}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
