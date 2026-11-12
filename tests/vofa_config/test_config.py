import importlib.util
from pathlib import Path
import struct
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('vofa_config', ROOT/'scripts/vofa_config.py')
config = importlib.util.module_from_spec(spec)
spec.loader.exec_module(config)


class ConfigurationManifestTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.compiled = config.build_power_manifest()

    def test_production_configuration_and_model_validate(self):
        self.assertEqual(len(self.compiled['models']), 4)
        self.assertTrue(config.validate_power_manifest(self.compiled))
        self.assertEqual(self.compiled['parameters']['outer.kp'], 1.0)
        self.assertEqual(self.compiled['tuning_source'], 0)

    def test_runtime_reply_changes_effective_values_and_identity(self):
        values = list(self.compiled['power_values'])
        values[0] = 2.0
        words = [struct.pack('>f', v).hex() for v in values]
        reply = '@T1,17,OK,power,42,3,' + ','.join(words) + ',saved'
        actual = config.build_power_manifest(tuning_reply=reply, source=2, config_version=7)
        self.assertNotEqual(actual['fingerprint'], self.compiled['fingerprint'])
        self.assertEqual(actual['parameters']['outer.kp'], 2.0)
        self.assertEqual(actual['config_version'], 7)
        self.assertEqual(actual['tuning_source'], 2)
        self.assertEqual(actual['tuning_revision'], 42)
        self.assertTrue(config.validate_power_manifest(actual))

    def test_missing_flash_evidence_and_wrong_mode_are_rejected(self):
        with self.assertRaises(ValueError):
            config.build_power_manifest(source=2)
        words = [struct.pack('>f', v).hex() for v in self.compiled['power_values']]
        with self.assertRaises(ValueError):
            config.build_power_manifest(tuning_reply='@T1,1,OK,power,1,2,' + ','.join(words) + ',saved', source=3)

    def test_changed_models_cannot_hide_behind_fingerprint_label(self):
        import copy
        changed = copy.deepcopy(self.compiled)
        changed['models'][0]['k0'] += 1
        self.assertFalse(config.validate_power_manifest(changed))
        changed = copy.deepcopy(self.compiled)
        changed['parameters']['outer.kp'] += 1
        self.assertFalse(config.validate_power_manifest(changed))

    def test_build_definitions_are_part_of_effective_configuration(self):
        actual = config.build_power_manifest(defines={'CHASSIS_POWER_SOURCE_MODE': '1'})
        self.assertEqual(actual['parameters']['use_supercap'], 1)
        self.assertNotEqual(actual['fingerprint'], self.compiled['fingerprint'])
        self.assertTrue(config.validate_power_manifest(actual))
        self.assertEqual(actual['provenance']['defines']['CHASSIS_POWER_SOURCE_MODE'], '1')

    def test_missing_parameters_and_changed_get_evidence_are_rejected(self):
        import copy
        changed = copy.deepcopy(self.compiled)
        del changed['parameters']['budget.buffer.recovery_enter_j']
        self.assertFalse(config.validate_power_manifest(changed))
        changed = copy.deepcopy(self.compiled)
        changed['tuning_source'] = 2
        self.assertFalse(config.validate_power_manifest(changed))


if __name__ == '__main__':
    unittest.main()
