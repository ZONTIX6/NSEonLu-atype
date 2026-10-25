"""Behavior tests for the source data dictionary (no firmware/toolchain needed)."""
import importlib.util
import contextlib
import io
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

SCRIPT = Path(__file__).resolve().parents[2] / "scripts/generate-structure-index.py"
spec = importlib.util.spec_from_file_location("structure_index", SCRIPT)
index = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = index
spec.loader.exec_module(index)


class ParseTests(unittest.TestCase):
    def parse(self, source, path="modules/motor/sample.h"):
        return index.parse_source(source, path)

    def test_comments_and_strings_do_not_create_fake_definitions(self):
        records = self.parse('// struct Fake { int bad; };\n'
                             'const char *s="struct Fake2 { int bad; };";\n'
                             '/* 电机反馈 */\ntypedef struct {\n'
                             ' float speed_aps; // 角速度，度/秒\n} Feedback;')
        self.assertEqual([r.name for r in records], ["Feedback"])
        self.assertEqual(records[0].line, 4)
        self.assertIn("电机反馈", records[0].comment)
        self.assertEqual(records[0].members[0].name, "speed_aps")
        self.assertEqual(records[0].members[0].line, 5)
        self.assertIn("角速度", records[0].members[0].comment)

    def test_multiple_declarators_arrays_callbacks_and_bitfields(self):
        record, = self.parse('typedef struct {\n'
                             ' float lo, *hi, gains[3];\n'
                             ' void (*callback)(int code, const char *text);\n'
                             ' unsigned ready:1, error:2;\n'
                             ' unsigned : 3;\n} State;')
        self.assertEqual([m.name for m in record.members],
                         ["lo", "hi", "gains", "callback", "ready", "error", "<未命名位域>"])
        self.assertIn("[3]", record.members[2].declaration)
        self.assertIn("const char", record.members[3].declaration)

    def test_nested_anonymous_and_named_union_are_searchable(self):
        records = self.parse('typedef struct {\n'
                             ' struct { int x; int y; } mouse;\n'
                             ' union { struct { unsigned ready:1; }; int raw; } status;\n'
                             '} Remote;')
        self.assertEqual(len(records), 4)
        root = records[0]
        self.assertEqual(root.name, "Remote")
        self.assertEqual([m.name for m in root.members], ["mouse", "status"])
        self.assertIsNotNone(root.members[0].nested)
        self.assertTrue(any(m.name == "ready" for r in records for m in r.members))
        self.assertEqual(sum(r.kind == "union" for r in records), 1)

    def test_tag_alias_local_definition_and_forward_declaration(self):
        records = self.parse('struct Node;\ntypedef struct Node NodeAlias;\n'
                             'struct Node { struct Node *next; };\n'
                             'typedef struct Tag { int code; } Alias, *AliasPtr;\n'
                             'void f(void) { struct { float lo, hi; } stack[48]; }')
        self.assertEqual(len(records), 3)
        self.assertIn("NodeAlias", records[0].aliases)
        self.assertEqual(records[1].name, "Alias")
        self.assertIn("AliasPtr", records[1].aliases)
        self.assertTrue(any("stack" in r.name for r in records))

    def test_packed_cmsis_and_attribute_forms(self):
        records = self.parse('typedef __PACKED_STRUCT { int a; } Packet;\n'
                             'struct __attribute__((packed, aligned(1))) Tag { int b; };\n'
                             '__PACKED_STRUCT T_UINT16_WRITE { unsigned short v; };')
        self.assertEqual([r.name for r in records], ["Packet", "Tag", "T_UINT16_WRITE"])

    def test_all_conditional_members_and_same_named_definitions_are_kept(self):
        records = self.parse('#if ENABLE_A\ntypedef struct {\n'
                             '#if EXTRA\n int enabled;\n#else\n int disabled;\n#endif\n'
                             '} State;\n#else\ntypedef struct { int fallback; } State;\n#endif\n')
        self.assertEqual([r.name for r in records], ["State", "State"])
        self.assertEqual([m.name for m in records[0].members], ["enabled", "disabled"])
        self.assertIn("EXTRA", records[0].members[0].condition)
        self.assertIn("else", records[0].members[1].condition)
        self.assertNotEqual(records[0].anchor, records[1].anchor)

    def test_macro_literal_body_is_kept_without_expanding_invocations(self):
        records = self.parse('#define RECORD(Name) typedef struct { int value; } Name\n'
                             'RECORD(Generated);\n')
        self.assertEqual(len(records), 1)
        self.assertEqual(records[0].name, "Name")
        self.assertIn("RECORD", records[0].condition)

    def test_conditional_type_headers_share_a_body_without_losing_tags(self):
        records = self.parse('#if CALLBACKS\ntypedef struct __Handle\n'
                             '#else\ntypedef struct\n#endif\n'
                             '{ int value; } Handle;')
        self.assertEqual(len(records), 2)
        self.assertEqual(records[0].tag, "__Handle")
        self.assertEqual(records[0].members[0].name, "value")
        self.assertEqual(records[1].members[0].name, "value")
        self.assertIn("CALLBACKS", records[0].condition)
        self.assertIn("else", records[1].condition)

    def test_preceding_member_comment_and_markdown_punctuation(self):
        record, = self.parse('typedef struct {\n'
                             ' /** 功率 | 电流 `说明` */\n float power_w;\n'
                             ' float current_a; /**< 电流 A */\n} Power;')
        self.assertIn("功率 | 电流", record.members[0].comment)
        self.assertIn("电流 A", record.members[1].comment)

    def test_unbalanced_definition_is_not_silently_omitted(self):
        with self.assertRaisesRegex(ValueError, "broken.h"):
            self.parse('typedef struct { int value;', "broken.h")

    def test_object_field_macros_keep_optional_members_and_separate_comments(self):
        records = self.parse('#if CHECK\n\t#define FIRST unsigned integrity;\n'
                             '#else\n  #define FIRST\n#endif\n'
                             'struct Item {\n FIRST /* macro comment */\n'
                             ' int value; /* actual value */\n};')
        item, = records
        self.assertEqual([m.name for m in item.members], ["integrity", "value"])
        self.assertIn("CHECK", item.members[0].condition)
        self.assertIn("FIRST", item.members[0].comment)
        self.assertNotIn("macro comment", item.members[1].comment)

    def test_xmacro_fields_are_individually_indexed_with_their_real_types(self):
        sample, = self.parse('#define FIELDS(U,I) U(version) I(power_cw)\n'
                             'typedef struct {\n#define FIELD_U(n) unsigned n;\n'
                             '#define FIELD_I(n) int n;\n'
                             ' FIELDS(FIELD_U, FIELD_I)\n} Sample;')
        self.assertEqual([m.name for m in sample.members], ["version", "power_cw"])
        self.assertEqual(sample.members[0].declaration, "unsigned version;")
        self.assertEqual(sample.members[1].declaration, "int power_cw;")
        self.assertEqual(sample.members[0].line, 5)

    def test_unknown_field_macro_is_an_explicit_failure(self):
        with self.assertRaisesRegex(ValueError, "UNKNOWN_FIELDS"):
            self.parse('struct Sample {\n UNKNOWN_FIELDS(X)\n};')

    def test_inline_field_macros_expand_at_declaration_boundaries(self):
        for source in ['#define FIELDS int value;\nstruct A { FIELDS };',
                       '#define FIELDS(X) X(value)\n#define FIELD(n) int n;\nstruct A { FIELDS(FIELD) };']:
            with self.subTest(source=source):
                record, = self.parse(source)
                self.assertEqual([m.name for m in record.members], ["value"])
        with self.assertRaisesRegex(ValueError, "UNKNOWN_FIELDS"):
            self.parse('struct A { UNKNOWN_FIELDS(X) };')


class GenerationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def write(self, name, text, encoding="utf-8"):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(text.encode(encoding))

    def test_scans_all_source_groups_and_legacy_encoding_but_not_build(self):
        for directory in ["modules/power_meter", "Drivers/HAL", "Middlewares/RTOS",
                          "portable_modules/copy", "tests/example/stubs", "new_module"]:
            self.write(directory + "/sample.h", '/* 功率数据 */\nstruct State { int value; };', "gb18030")
        self.write("build/generated.h", "struct ShouldNotAppear { int bad; };")
        self.write("node_modules/generated.h", "struct ShouldNotAppear { int bad; };")
        markdown = index.generate(self.root)
        self.assertIn("功率数据", markdown)
        self.assertNotIn("ShouldNotAppear", markdown)
        for directory in ["Drivers/HAL", "Middlewares/RTOS", "portable_modules/copy",
                          "tests/example/stubs", "new_module"]:
            self.assertIn(directory + "/sample.h", markdown)

    def test_chinese_keywords_reverse_member_lookup_and_nested_links(self):
        self.write("modules/motor/sample.h",
                   'typedef struct { float speed_aps; } Measure;\n'
                   'typedef struct { Measure measure; bool online; } Motor;\n')
        self.write("modules/power_meter/sample.h", 'struct Meter { bool online; float power_w; };')
        markdown = index.generate(self.root)
        self.assertIn("电机", markdown)
        self.assertIn("功率计", markdown)
        reverse = markdown.split("## 成员名反查", 1)[1].split("## 类型详情", 1)[0]
        self.assertIn("speed_aps", reverse)
        self.assertIn("Motor", reverse)
        self.assertIn("Meter", reverse)
        self.assertGreaterEqual(reverse.count("`online`"), 2)
        self.assertIn("嵌套关系", markdown)
        self.assertRegex(markdown, r'Measure\]\(#r-')
        self.assertEqual(index.generate(self.root), markdown)

    def test_duplicate_type_links_prefer_same_file_and_list_ambiguity(self):
        self.write("modules/motor/a.h", 'typedef struct { int real; } State;\n'
                   'typedef struct { State state; } Owner;')
        self.write("tests/stubs/a.h", 'typedef struct { int stub; } State;')
        markdown = index.generate(self.root)
        self.assertEqual(markdown.count('id="r-'), 3)
        self.assertIn("modules/motor/a.h#L2", markdown)
        self.assertIn("tests/stubs/a.h#L1", markdown)

    def test_primitive_member_names_are_not_mistaken_for_nested_types(self):
        self.write("modules/a.h", 'typedef struct { int value; } Measure;\n'
                   'typedef struct { int Measure; Measure (*callback)(void); } Owner;')
        markdown = index.generate(self.root)
        self.assertNotIn('`Owner.Measure` →', markdown)
        self.assertNotIn('`Owner.callback` →', markdown)

    def test_typedef_in_a_separate_header_resolves_to_its_definition(self):
        self.write("modules/a.h", 'typedef struct Node NodeAlias;\n'
                   'typedef struct { NodeAlias *head; } Owner;')
        self.write("modules/a.c", 'struct Node { int value; };')
        markdown = index.generate(self.root)
        self.assertIn('`Owner.head` → [Node]', markdown)

    def test_unused_typedef_alias_is_searchable_with_its_declaration_location(self):
        self.write("modules/a.h", 'typedef struct Node NodeAlias;')
        self.write("modules/a.c", 'struct Node { int value; };')
        markdown = index.generate(self.root)
        self.assertIn('`NodeAlias`', markdown)
        self.assertIn('modules/a.h#L1', markdown)

    def test_empty_source_files_are_in_inventory_without_nonexistent_line_links(self):
        self.write("modules/empty.h", "")
        markdown = index.generate(self.root)
        self.assertIn("modules/empty.h", markdown)
        self.assertNotIn("empty.h#L1", markdown)

    def run_cli(self, *args):
        return subprocess.run([sys.executable, str(SCRIPT), "--root", str(self.root), *args],
                              capture_output=True, text=True, encoding="utf-8")

    def test_cli_check_detects_stale_output_without_overwriting_it(self):
        self.write("modules/a.h", "struct A { int member; };")
        self.assertEqual(self.run_cli().returncode, 0)
        output = self.root / "docs/structure-index.md"
        old = output.read_bytes()
        self.assertEqual(self.run_cli("--check").returncode, 0)
        self.write("modules/a.h", "struct A { int new_member; };")
        self.assertNotEqual(self.run_cli("--check").returncode, 0)
        self.assertEqual(output.read_bytes(), old)
        self.assertEqual(self.run_cli().returncode, 0)
        self.assertIn("new_member", output.read_text(encoding="utf-8"))

    def test_failed_parse_keeps_existing_document(self):
        self.write("docs/structure-index.md", "keep me")
        self.write("modules/broken.h", "typedef struct { int value;")
        result = self.run_cli()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("broken.h", result.stderr)
        self.assertEqual((self.root / "docs/structure-index.md").read_text(), "keep me")

    def test_check_accepts_windows_checkout_line_endings_without_rewriting(self):
        self.write("modules/a.h", "struct A { int value; };")
        self.assertEqual(self.run_cli().returncode, 0)
        output = self.root / "docs/structure-index.md"
        windows = output.read_bytes().replace(b"\n", b"\r\n")
        output.write_bytes(windows)
        self.assertEqual(self.run_cli("--check").returncode, 0)
        self.assertEqual(output.read_bytes(), windows)

    def test_western_vendor_comments_are_preserved(self):
        self.write("Middlewares/vendor.c", "/* Copyright © vendor */\nstruct Vendor { int value; };", "cp1252")
        self.assertIn("© vendor", index.generate(self.root))

    def test_publish_failure_keeps_previous_index_and_cleans_temporary_output(self):
        self.write("modules/a.h", "struct A { int value; };")
        self.write("docs/structure-index.md", "previous complete index")
        with patch.object(sys, "argv", [str(SCRIPT), "--root", str(self.root)]), \
             patch.object(index.os, "replace", side_effect=OSError("simulated publish failure")), \
             contextlib.redirect_stderr(io.StringIO()):
            result = index.main()
        self.assertEqual(result, 1)
        self.assertEqual((self.root / "docs/structure-index.md").read_text(), "previous complete index")
        self.assertEqual([p.name for p in (self.root / "docs").iterdir()], ["structure-index.md"])


if __name__ == "__main__":
    unittest.main()
