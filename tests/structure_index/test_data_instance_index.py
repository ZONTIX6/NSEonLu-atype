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

    def write_macro_sources(self):
        self.write("modules/remote/remote_control.h", """#ifndef REMOTE_CONTROL_H
#define REMOTE_CONTROL_H
#define RC_SW_UP ((uint16_t)1)
#define RC_SW_MID ((uint16_t)3)
#define RC_SW_DOWN ((uint16_t)2)
#define switch_is_up(s) (s == RC_SW_UP)
#define switch_is_mid(s) (s == RC_SW_MID)
#define switch_is_down(s) (s == RC_SW_DOWN)
#define KEY_PRESS 0
#endif
""")
        self.write("Inc/board_config.h", """#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H
#define BOARD_REMOTE_UART (&huart1)
#define BOARD_VTX_UART (&huart8)
#ifndef REFEREE_UART_PORT
#define REFEREE_UART_PORT 6
#endif
#if REFEREE_UART_PORT == 7
#define BOARD_REFEREE_UART (&huart7)
#else
#define BOARD_REFEREE_UART (&huart6)
#endif
#define REFEREE_ENABLE 1
#define LOG_OUTPUT 1
#endif
""")
        self.write("modules/algorithm/chassis_power_config.h",
                   "#define POWER_METER_ENABLE 1\n#define CHASSIS_POWER_CONTROL_STAGE 3\n")
        self.write("modules/algorithm/chassis_energy_config.h",
                   "#define CHASSIS_POWER_SOURCE_MODE CHASSIS_POWER_SOURCE_DIRECT\n#define CHASSIS_BUFFER_POWER_ENABLE 1\n")
        self.write("modules/algorithm/chassis_spin_config.h", """#ifndef CHASSIS_SPIN_MODE
#if CHASSIS_SPIN_ADAPTIVE_ENABLE
#define CHASSIS_SPIN_MODE CHASSIS_SPIN_MODE_ADAPTIVE
#else
#define CHASSIS_SPIN_MODE CHASSIS_SPIN_MODE_FIXED
#endif
#endif
#define CHASSIS_SPIN_FIXED_WZ 8000.0f
""")
        self.write("application/robot_def.h",
                   "#define CHASSIS_FOLLOW_MODE 0\n#define YAW_CHASSIS_ALIGN_ECD 350\n#define PITCH_HORIZON_ECD 3412\n")
        self.write("modules/telemetry/usb_telemetry_config.h",
                   "#define USB_TELEMETRY_ENABLE 1\n#define USB_TELEMETRY_PERIOD_MS 20U\n")

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
        args = [sys.executable, str(SCRIPT), "--root", str(self.root), "--full"]
        result = subprocess.run(args, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        output = self.root / "docs/data-instance-index-full.md"
        old = output.read_bytes()
        self.assertEqual(subprocess.run(args + ["--check"], capture_output=True).returncode, 0)
        self.write("modules/sample.c", 'typedef struct { int new_value; } State;\nstatic State *data;')
        self.assertNotEqual(subprocess.run(args + ["--check"], capture_output=True).returncode, 0)
        self.assertEqual(output.read_bytes(), old)
        self.assertEqual(index.generate(self.root), index.generate(self.root))

    def test_common_index_keeps_useful_pointer_paths_and_omits_unrelated_instances(self):
        self.write_macro_sources()
        self.write("modules/power_meter/power_meter.c",
                   'typedef struct { float power_w, current_a, bus_voltage_v; bool online; } Status;\n'
                   'typedef struct { Status status; } Meter;\nstatic Meter *power_meter;\n'
                   'static Meter *unrelated_pointer;')
        md = index.generate_common(self.root)
        self.assertIn("功率计", md)
        self.assertIn("`power_meter->status.power_w`", md)
        self.assertIn("power_meter.c#L3", md)
        self.assertNotIn("unrelated_pointer", md)
        self.assertNotIn("局部临时实例", md)

    def test_common_index_reports_a_stale_curated_member_instead_of_printing_bad_paths(self):
        self.write_macro_sources()
        self.write("modules/power_meter/power_meter.c",
                   'typedef struct { int renamed; } Status;\n'
                   'typedef struct { Status status; } Meter;\nstatic Meter *power_meter;')
        with self.assertRaisesRegex(ValueError, "power_meter"):
            index.generate_common(self.root)

    def test_macro_table_includes_conditional_values_and_locations_without_switch_entries(self):
        self.write_macro_sources()
        self.write("application/cmd/robot_cmd.c",
                   'typedef union { unsigned keys; } Key;\n'
                   'typedef struct { struct { int rocker_l_, switch_left; } rc; Key key[3]; } Remote;\n'
                   'static Remote *rc_data;')
        md = index.generate_common(self.root)
        macros = md.split("## 常用宏", 1)[1]
        rows = [line for line in macros.splitlines() if line.startswith("| ")][2:]
        self.assertEqual(len(rows), 18)
        self.assertNotIn("拨杆", md)
        self.assertNotIn("RC_SW_", md)
        self.assertNotIn("switch_is_", md)
        self.assertNotIn("rc.switch_left", md)
        self.assertIn("rc_data->rc.rocker_l_", md)
        self.assertIn("rc_data->key[0].keys", md)
        uart = next(line for line in rows if "`BOARD_REFEREE_UART`" in line)
        self.assertIn("`(&huart7)`", uart)
        self.assertIn("`(&huart6)`", uart)
        self.assertIn("#if REFEREE_UART_PORT == 7", uart)
        self.assertIn("#else", uart)
        self.assertIn("board_config.h#L9", uart)
        self.assertIn("board_config.h#L11", uart)
        spin = next(line for line in rows if "`CHASSIS_SPIN_MODE`" in line)
        self.assertIn("CHASSIS_SPIN_MODE_ADAPTIVE", spin)
        self.assertIn("CHASSIS_SPIN_MODE_FIXED", spin)
        self.assertNotIn("#ifndef CHASSIS_SPIN_MODE", spin)
        self.assertNotIn("#ifndef BOARD_CONFIG_H", macros)
        self.assertIn("源码默认值", md)
        self.assertIn("当前固件", md)
        period = next(line for line in rows if "`USB_TELEMETRY_PERIOD_MS`" in line)
        self.assertIn("20U", period)
        self.assertIn("ms", period)

    def test_macro_extraction_ignores_commented_definitions_and_preserves_continuations(self):
        self.write_macro_sources()
        path = "modules/remote/remote_control.h"
        text = (self.root / path).read_text(encoding="utf-8")
        text = text.replace("#define KEY_PRESS 0", "// #define KEY_PRESS 999\n/* #define KEY_PRESS 888 */\n#define KEY_PRESS 0")
        text = text.replace("#define KEY_PRESS 0",
                            "  #define KEY_PRESS " + "\\" + "\n    (0) // actual body")
        self.write(path, text)
        md = index.generate_common(self.root)
        self.assertNotIn("999", md)
        self.assertNotIn("888", md)
        self.assertIn("`(0)`", md)

    def test_default_cli_uses_moved_document_and_checks_macro_changes(self):
        self.write_macro_sources()
        args = [sys.executable, str(SCRIPT), "--root", str(self.root)]
        first = subprocess.run(args, capture_output=True)
        self.assertEqual(first.returncode, 0, first.stderr)
        output = self.root / ".Doc/指针查找索引.md"
        self.assertTrue(output.exists())
        self.assertFalse((self.root / "docs/data-instance-index.md").exists())
        self.assertEqual(subprocess.run(args + ["--check"], capture_output=True).returncode, 0)
        old = output.read_bytes()
        self.write("modules/telemetry/usb_telemetry_config.h",
                   "#define USB_TELEMETRY_ENABLE 1\n#define USB_TELEMETRY_PERIOD_MS 50U\n")
        self.assertNotEqual(subprocess.run(args + ["--check"], capture_output=True).returncode, 0)
        self.assertEqual(output.read_bytes(), old)
        self.assertEqual(subprocess.run(args, capture_output=True).returncode, 0)
        self.assertIn("50U", output.read_text(encoding="utf-8"))

    def test_missing_macro_reports_name_and_keeps_old_document(self):
        self.write_macro_sources()
        args = [sys.executable, str(SCRIPT), "--root", str(self.root)]
        self.assertEqual(subprocess.run(args, capture_output=True).returncode, 0)
        output = self.root / ".Doc/指针查找索引.md"
        old = output.read_bytes()
        self.write("modules/telemetry/usb_telemetry_config.h", "#define USB_TELEMETRY_ENABLE 1\n")
        result = subprocess.run(args, capture_output=True, encoding="utf-8")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("USB_TELEMETRY_PERIOD_MS", result.stderr)
        self.assertIn("usb_telemetry_config.h", result.stderr)
        self.assertEqual(output.read_bytes(), old)


if __name__ == "__main__":
    unittest.main()
