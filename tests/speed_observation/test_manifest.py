"""Exercise experiment provenance and generated C parameter contract."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts'))
import chassis_speed_capture as csc

CONFIG={'version':1,'max_gap_us':20000,'max_age_us':20000,'max_horizon_us':20000,
        'q':1.0,'r':0.25,'initial_p00':1.0,'initial_p11':100.0,'gate_squared':25.0,
        'max_rejections':3,'synthetic':True,'units':'rotor_rad_s'}

class ManifestTests(unittest.TestCase):
    def test_missing_measured_fields_stay_unknown(self):
        manifest=csc.build_manifest({'synthetic':True},ROOT)
        self.assertEqual(manifest['hardware_gate'],'insufficient_evidence')
        self.assertEqual(manifest['pid_actual'],{'value':None,'status':'unmeasured'})
        self.assertEqual(len(manifest['source']['worktree_patch_sha256']),64)
        self.assertTrue(csc.validate_manifest(manifest))
        manifest['synthetic']=False
        self.assertFalse(csc.validate_manifest(manifest))

    def test_artifact_hash_rejects_a_missing_measurement_file(self):
        with tempfile.TemporaryDirectory() as directory:
            sample=Path(directory)/'capture.bin';sample.write_bytes(b'known capture')
            manifest=csc.build_manifest({'synthetic':True,'artifacts':{'capture':str(sample)}},ROOT)
            self.assertEqual(manifest['artifact_hashes']['capture']['bytes'],13)
            self.assertEqual(len(manifest['artifact_hashes']['capture']['sha256']),64)
            sample.unlink()
            with self.assertRaises(ValueError):
                csc.build_manifest({'artifacts':{'capture':str(sample)}},ROOT)

    def test_nonfinite_wrong_units_invalid_parameters_are_rejected(self):
        for update in ({'r':0},{'q':float('nan')},{'max_rejections':0},{'max_age_us':-1},
                       {'max_age_us':0x80000000},{'max_gap_us':0xffffffff},
                       {'max_horizon_us':0x80000000},{'version':0},{'units':'rpm'},{'synthetic':'true'}):
            with self.assertRaises(ValueError): csc.encode_kf_config(CONFIG|update)

    def test_generated_header_compiles_and_matches_canonical_values(self):
        with tempfile.TemporaryDirectory() as directory:
            d=Path(directory)
            config=d/'config.json';config.write_text(json.dumps(CONFIG),encoding='utf-8')
            header=d/'chassis_speed_kf_experiment.h'
            subprocess.run([sys.executable,str(ROOT/'scripts/chassis-speed-capture.py'),
                'emit-kf-header','--kf-config',str(config),'--output',str(header)],check=True)
            # Standalone ABI contract tests header values without including unimplemented glue.
            source=d/'check.c'
            source.write_text('''#include <assert.h>
#include "chassis_speed_kf_experiment.h"
int main(void) {
assert(chassis_speed_experiment_kf_config.version==1);
assert(chassis_speed_experiment_kf_config.r==0.25f);
assert(chassis_speed_experiment_kf_config.max_horizon_us==20000);
assert(chassis_speed_experiment_kf_synthetic);
return chassis_speed_experiment_kf_checksum==0;
}
''',encoding='utf-8')
            exe=d/'check.exe'
            subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror',
                            '-I'+str(ROOT/'modules/predictive_power'),'-I'+str(d),str(source),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True)

if __name__=='__main__':unittest.main()
