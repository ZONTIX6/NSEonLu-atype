"""Locate real data roots rather than type declarations or borrowed parameters."""
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[2] / "scripts/generate-data-instance-index.py"
spec = importlib.util.spec_from_file_location("data_instance_index", SCRIPT)
index = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = index
spec.loader.exec_module(index)


class InstanceTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def write(self, name, text):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")

    def roots(self, source):
        self.write("modules/sample.c", source)
        return index.inventory(self.root).variables

    def test_global_multi_pointer_array_object_and_extern_declarations(self):
        roots = self.roots('typedef struct { int value; } State;\n'
                           'static State *left, *right;\nState *motors[4];\n'
                           'static State data[2];\nextern State *external;')
        self.assertEqual([r.name for r in roots], ["left", "right", "motors", "data", "external"])
        self.assertEqual([r.pointers for r in roots], [1, 1, 1, 0, 1])
        self.assertEqual(roots[2].arrays, ["4"])
        self.assertEqual(roots[3].arrays, ["2"])
        self.assertEqual(roots[4].storage, "extern")

    def test_function_parameters_prototypes_members_and_casts_are_not_roots(self):
        roots = self.roots('typedef struct { int value; } State;\n'
                           'typedef struct { State *child; } Parent;\n'
                           'State *Create(State *input);\n'
                           'void Init(State *borrowed) { State *local = Create(borrowed); }\n'
                           'void Use(void) { (void)((State *)0); }')
        self.assertEqual([r.name for r in roots], ["local"])
        self.assertEqual(roots[0].function, "Init")
        self.assertEqual(roots[0].storage, "local")

    def test_anonymous_object_names_and_member_assignments_are_not_type_declarations(self):
        roots = self.roots('static struct { int id; int ready; } mailbox;\n'
                           'void Set(int id) { mailbox.id=id; mailbox.ready=1; }')
        self.assertEqual([r.name for r in roots], ["mailbox"])

    def test_local_static_and_nested_block_scopes(self):
        roots = self.roots('typedef struct { int value; } State;\n'
                           'void Work(void) { static State saved; State *temp;\n'
                           ' if (1) { State *temp; } }')
        self.assertEqual([r.name for r in roots], ["saved", "temp", "temp"])
        self.assertEqual(roots[0].storage, "local static")
        self.assertNotEqual(roots[1].scope_start, roots[2].scope_start)

    def test_equality_and_for_control_blocks_are_not_braced_initializers(self):
        roots = self.roots('typedef struct { int value; } State;\n'
                           'void Work(int mode) { if(mode==1) { static State saved; State *p; }\n'
                           ' for(int i=0;i<1;i++) { static State saved2; } State *after; }')
        self.assertEqual([r.name for r in roots], ["saved", "p", "saved2", "after"])

    def test_conditional_block_shadow_does_not_rebind_a_persistent_pointer(self):
        roots = self.roots('typedef struct { int value; } State;\nstatic State *data;\n'
                           'void Work(int mode) { if(mode==1) { State *data; data=Other(); } }')
        self.assertEqual(roots[0].assignments, [])
        self.assertEqual(roots[1].assignments[0].expression, "Other()")

    def test_conditional_call_prefixes_share_a_closing_parenthesis(self):
        roots = self.roots('typedef struct { int value; } State;\nstatic State *data;\n'
                           'void Init(void) { State *local =\n#if OPTION\n AllocateA(\n'
                           '#else\n AllocateB(\n#endif\n 1); data = local; }')
        self.assertEqual([r.name for r in roots], ["data", "local"])
        self.assertEqual(roots[0].assignments[0].expression, "local")

    def test_binding_evidence_ignores_shadowed_locals_and_comparisons(self):
        roots = self.roots('typedef struct { int value; } State;\nstatic State *data;\n'
                           'void Init(void) { State *instance = Allocate(); data = instance; }\n'
                           'void Other(void) { State *data; data = OtherValue(); }\n'
                           'void Read(void) { if (data == 0) return; data->value = 1; }')
        data = next(r for r in roots if r.name == "data" and not r.function)
        self.assertEqual([w.expression for w in data.assignments], ["instance"])
        self.assertIn("Allocate()", next(r for r in roots if r.name == "instance").initializer)

    def test_qualified_pointer_initializers_and_inline_anonymous_storage(self):
        roots = self.roots('typedef struct { int value; } State;\n'
                           'static State data;\n'
                           'const State *const volatile view = &data;\n'
                           'static struct { int ready; } private_data;')
        self.assertEqual([r.name for r in roots], ["data", "view", "private_data"])
        self.assertEqual(roots[1].initializer, "&data")
        self.assertEqual(roots[1].pointers, 1)

    def test_member_search_yields_full_real_pointer_paths(self):
        self.write("modules/motor/test.c", 'typedef struct { float speed_aps; } Measure;\n'
                   'typedef struct { Measure measure; } Motor;\nstatic Motor *motor_lf;')
        markdown = index.generate(self.root)
        self.assertIn("电机", markdown)
        self.assertIn("`motor_lf->measure.speed_aps`", markdown)
        self.assertIn("modules/motor/test.c#L3", markdown)

    def test_arrays_pointer_aliases_and_pointer_to_array_paths(self):
        self.write("modules/sample.c", 'typedef struct { int value; } State;\n'
                   'typedef State *Handle;\nstatic Handle handle;\n'
                   'static State states[2];\nstatic State *ptrs[4];\n'
                   'static State (*blocks)[2];')
        md = index.generate(self.root)
        for path in ['handle->value', 'states[0].value', 'ptrs[0]->value', '(*blocks)[0].value']:
            self.assertIn('`' + path + '`', md)

    def test_inline_pointer_typedef_and_array_aliases_preserve_access_shape(self):
        self.write("modules/sample.c", 'typedef struct { int value; } State, *Handle;\n'
                   'typedef State Items[2];\nstatic Handle handle;\nstatic Items items;\n'
                   'static Items *blocks;')
        md = index.generate(self.root)
        for path in ['handle->value', 'items[0].value', '(*blocks)[0].value']:
            self.assertIn('`' + path + '`', md)
        self.assertNotIn('`handle.value`', md)

    def test_pointer_only_inline_typedef_still_uses_arrow(self):
        self.write("modules/sample.c", 'typedef struct State { int value; } *Handle;\nstatic Handle handle;')
        self.assertIn('`handle->value`', index.generate(self.root))

    def test_each_nested_declarator_keeps_its_own_pointer_and_array_shape(self):
        self.write("modules/sample.c", 'typedef struct { int value; } Child;\n'
                   'typedef struct { Child first, *second; Child items[2], single; Child **indirect; } Parent;\n'
                   'static Parent *data;')
        md = index.generate(self.root)
        for path in ['data->first.value', 'data->second->value', 'data->items[0].value',
                     'data->single.value', '(*data->indirect)->value']:
            self.assertIn('`' + path + '`', md)
        self.assertNotIn('`data->first->value`', md)

    def test_vendor_data_types_are_not_replaced_by_test_stubs(self):
        self.write("Drivers/HAL/hal.h", 'typedef struct { int real; } Handle;')
        self.write("tests/stubs/hal.h", 'typedef struct { int fake; } Handle;')
        self.write("application/app.c", 'Handle *handle;')
        md = index.generate(self.root)
        self.assertIn('`handle->real`', md)
        self.assertNotIn('`handle->fake`', md)

    def test_primitive_local_shadow_does_not_rebind_a_global_pointer(self):
        roots = self.roots('typedef struct { int value; } State;\nstatic State *data;\n'
                           'void Work(void) { int data; data = 3; }')
        self.assertEqual(roots[0].assignments, [])

    def test_for_initializer_and_primitive_alias_shadows_are_not_global_bindings(self):
        roots = self.roots('typedef struct { int value; } State;\ntypedef int Count;\n'
                           'static State *data;\nvoid Work(void) {\n'
                           ' for(int data=0;data<2;data++) {}\n'
                           ' { Count data; data=3; }\n data=Real(); }')
        self.assertEqual([a.expression for a in roots[0].assignments], ["Real()"])

    def test_recursive_types_stop_and_duplicate_types_stay_explicit(self):
        self.write("modules/a.c", 'typedef struct Node { int value; struct Node *next; } Node;\nstatic Node *root;')
        md = index.generate(self.root)
        self.assertIn("`root->value`", md)
        self.assertIn("`root->next`", md)
        self.assertIn("递归", md)
        self.assertLess(md.count('root->next->next'), 2)

    def test_object_member_address_and_temporary_section(self):
        self.write("modules/sample.c", 'typedef struct { int value; } State;\n'
                   'static State stored;\nvoid Work(void) { State *temp = &stored; }')
        md = index.generate(self.root)
        self.assertIn("`stored.value`", md)
        self.assertIn("`&stored`", md)
        self.assertIn("## 局部临时实例", md)
        self.assertIn("Work", md)

    def test_cli_check_and_deterministic_generation(self):
        self.write("modules/sample.c", 'typedef struct { int value; } State;\nstatic State *data;')
        args = [sys.executable, str(SCRIPT), "--root", str(self.root)]
        result = subprocess.run(args, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        output = self.root / "docs/data-instance-index.md"
        old = output.read_bytes()
        self.assertEqual(subprocess.run(args + ["--check"], capture_output=True).returncode, 0)
        self.write("modules/sample.c", 'typedef struct { int new_value; } State;\nstatic State *data;')
        self.assertNotEqual(subprocess.run(args + ["--check"], capture_output=True).returncode, 0)
        self.assertEqual(output.read_bytes(), old)
        self.assertEqual(index.generate(self.root), index.generate(self.root))


if __name__ == "__main__":
    unittest.main()
