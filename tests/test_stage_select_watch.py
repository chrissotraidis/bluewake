"""The stage select's inline address filter must cover every address its bodies act on.

bluewake_stage_select_dispatch, _observes and _redirect test the address inline
before calling their out-of-line bodies (runtime/host/src/stage_select.h). That
is exact only while each body acts at no address outside the inline set; an
address added to a body but not to the filter would be silently skipped.
"""

import re
import unittest
from pathlib import Path

HEADER = Path(__file__).resolve().parents[1] / "runtime/host/src/stage_select.h"


def function_body(source: str, name: str) -> str:
    start = source.index(name + "(")
    brace = source.index("{", start)
    depth = 0
    for i in range(brace, len(source)):
        if source[i] == "{":
            depth += 1
        elif source[i] == "}":
            depth -= 1
            if depth == 0:
                return source[brace:i + 1]
    raise AssertionError(f"unterminated body: {name}")


def compared_addresses(body: str) -> set[str]:
    # Only the address-valued names: equality tests against `address`.
    return set(re.findall(r"address\s*[!=]=\s*(BLUEWAKE_STAGE_SELECT_[A-Z_]+)", body))


class StageSelectWatch(unittest.TestCase):
    def setUp(self):
        self.source = HEADER.read_text()

    def test_observes_body_addresses_are_watched(self):
        watched = compared_addresses(function_body(self.source, "bool bluewake_stage_select_watched"))
        observed = compared_addresses(function_body(self.source, "bool bluewake_stage_select_observes_body"))
        self.assertTrue(observed, "observes_body compares no addresses")
        self.assertLessEqual(observed, watched)

    def test_redirect_body_acts_only_at_play_execute(self):
        redirect = compared_addresses(function_body(self.source, "bool bluewake_stage_select_redirect_body"))
        self.assertEqual(redirect, {"BLUEWAKE_STAGE_SELECT_PLAY_EXECUTE"})
        wrapper = function_body(self.source, "bool bluewake_stage_select_redirect")
        self.assertIn("BLUEWAKE_STAGE_SELECT_PLAY_EXECUTE", wrapper)

    def test_dispatch_body_starts_from_observes(self):
        dispatch = function_body(self.source, "void bluewake_stage_select_dispatch_body")
        self.assertRegex(dispatch, r"^\{\s*if \(__builtin_expect\(!bluewake_stage_select_observes_body\(address\), 1\)\)\s*return;")
        wrapper = function_body(self.source, "void bluewake_stage_select_dispatch")
        self.assertIn("bluewake_stage_select_watched(address)", wrapper)


if __name__ == "__main__":
    unittest.main()
