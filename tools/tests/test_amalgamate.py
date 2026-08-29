from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest


MODULE_PATH = Path(__file__).resolve().parents[1] / "amalgamate.py"
SPEC = importlib.util.spec_from_file_location("amalgamate", MODULE_PATH)
assert SPEC is not None and SPEC.loader is not None
amalgamate = importlib.util.module_from_spec(SPEC)
sys.modules["amalgamate"] = amalgamate
SPEC.loader.exec_module(amalgamate)


class AmalgamateTest(unittest.TestCase):
    def test_recursive_duplicate_and_provenance(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "main.cpp").write_text(
                '#include "a.hpp"\n#include <vector>\n#include "a.hpp"\nint main() {}\n',
                encoding="utf-8",
            )
            (root / "a.hpp").write_text(
                '// Source: https://github.com/example/library\n  # pragma   once // generated guard\n#include "b.hpp"\nint a = 1;\n',
                encoding="utf-8",
            )
            (root / "b.hpp").write_text("int b = 2;\n", encoding="utf-8")

            result = amalgamate.Amalgamator(repo_root=root).amalgamate(root / "main.cpp")
            self.assertEqual(result, amalgamate.Amalgamator(repo_root=root).amalgamate(root / "main.cpp"))
            self.assertEqual(result.count("int a = 1;"), 1)
            self.assertEqual(result.count("int b = 2;"), 1)
            self.assertIn("#include <vector>", result)
            self.assertIn("https://github.com/example/library", result)
            self.assertIn("Duplicate local include omitted", result)
            self.assertNotRegex(result, r"(?m)^\s*#\s*pragma\s+once\b")

            plain = amalgamate.amalgamate(root / "main.cpp", repo_root=root, provenance=False)
            self.assertNotIn("BEGIN INCLUDED", plain)
            self.assertEqual(plain.count("int a = 1;"), 1)

    def test_cycle_and_missing_include_are_errors(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "main.cpp").write_text('#include "a.hpp"\n', encoding="utf-8")
            (root / "a.hpp").write_text('#include "b.hpp"\n', encoding="utf-8")
            (root / "b.hpp").write_text('#include "a.hpp"\n', encoding="utf-8")
            with self.assertRaises(amalgamate.IncludeCycleError):
                amalgamate.amalgamate(root / "main.cpp", provenance=False)

            (root / "b.hpp").write_text('#include "missing.hpp"\n', encoding="utf-8")
            with self.assertRaises(amalgamate.IncludeNotFoundError):
                amalgamate.amalgamate(root / "main.cpp", provenance=False)


if __name__ == "__main__":
    unittest.main()
