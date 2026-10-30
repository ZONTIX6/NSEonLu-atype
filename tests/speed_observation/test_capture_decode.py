"""Exercise production CSC1 decoder against independent bytes and C encoder."""
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import chassis_speed_capture as csc


class DecoderTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.capture = Path(self.directory.name) / 'fixture.bin'
        subprocess.run(['make', '-C', str(ROOT / 'tests/speed_observation'), 'test_capture.exe'], check=True)
        subprocess.run([str(ROOT/'tests/speed_observation/test_capture.exe'), str(self.capture)], check=True)
        self.data = self.capture.read_bytes()

    def test_c_encoder_preserves_same_tick_events_and_signed_units(self):
        result = csc.decode_capture(b'log before\r\n' + self.data + b'log after\n')
        self.assertTrue(result['complete'], result['errors'])
        raw = [r['payload'] for r in result['records'] if r['kind'] == 'raw']
        self.assertEqual([r['sequence'] for r in raw], [0xffffffff, 0])
        self.assertEqual([r['rx_tick_ms'] for r in raw], [100, 100])
        self.assertEqual(raw[0]['raw_feedback_current'], -123)
        self.assertEqual(raw[0]['raw_rpm'], -600)
        self.assertEqual(raw[0]['filtered_rpm'], -599.5)
        self.assertIsNone(raw[0]['sample_time_us'])
        self.assertEqual(result['records'][0]['payload']['wheel_order'], ['LF', 'RF', 'LB', 'RB'])

    def test_bad_crc_truncation_missing_metadata_and_mixed_ids_are_incomplete(self):
        bad = bytearray(self.data); bad[1050] ^= 8
        self.assertFalse(csc.decode_capture(bad)['complete'])
        self.assertFalse(csc.decode_capture(self.data[:-1])['complete'])
        self.assertFalse(csc.decode_capture(self.data[1044:])['complete'])
        bad = bytearray(self.data)
        struct.pack_into('<I',bad,1044+10,99)
        struct.pack_into('<H',bad,1044+58,csc.crc16(bad[1044:1044+58]))
        self.assertFalse(csc.decode_capture(bad)['complete'])

    def test_duplicate_frame_and_unknown_schema_cannot_create_complete_window(self):
        duplicate = self.data[:1104]+self.data[1044:1104]+self.data[1104:]
        self.assertFalse(csc.decode_capture(duplicate)['complete'])
        bad = bytearray(self.data); struct.pack_into('<H',bad,4,2)
        struct.pack_into('<H',bad,1042,csc.crc16(bad[:1042]))
        self.assertFalse(csc.decode_capture(bad)['complete'])

    def test_cli_jsonl_records_and_summary(self):
        output = Path(self.directory.name)/'capture.jsonl'
        subprocess.run([sys.executable,str(ROOT/'scripts/chassis-speed-capture.py'),'decode',
                        '--input',str(self.capture),'--output',str(output)],check=True)
        rows = [json.loads(line) for line in output.read_text(encoding='utf-8').splitlines()]
        self.assertEqual(rows[0]['type'],'record')
        self.assertEqual(rows[-1]['type'],'summary')
        self.assertTrue(rows[-1]['complete'])

    def test_independent_wire_vector(self):
        self.assertEqual(csc.crc16(b'123456789'),0x4b37)
        payload=struct.pack('<BBBBIIIhhfhHQI',3,42,0,1,9,4,100,-50,-60,-49.5,-59,0,0,12)
        frame=csc.encode_frame(2,7,0,payload)
        self.assertEqual(frame[:18],b'CSC1\x01\x00\x02\x00\x28\x00\x07\x00\x00\x00\x00\x00\x00\x00')
        row=csc.decode_capture(frame)['records'][0]['payload']
        self.assertEqual(row['event_index'],12)
        self.assertEqual(row['raw_feedback_current'],-60)
        independent=Path(self.directory.name)/'python-raw.bin'
        independent.write_bytes(frame)
        subprocess.run([str(ROOT/'tests/speed_observation/test_capture.exe'),
                        '--check-raw',str(independent)],check=True)

    def test_unknown_timing_and_resources_are_missing_not_large_measurements(self):
        payload=bytearray(512)
        for offset in (12,16,80+68,448,452,456,460,468,472,476,480,484,488,492):
            struct.pack_into('<I',payload,offset,0xffffffff)
        row=csc.decode_cycle(payload)
        self.assertIsNone(row['period_us']);self.assertIsNone(row['execution_us'])
        self.assertIsNone(row['wheels'][0]['accepted_age_us'])
        for name in ('kf_us','allocator_us','fast_shadow_us','slow_shadow_us',
                     'execution_max_us','control_to_submit_end_us','motor_stack_free_words',
                     'robot_stack_free_words','observation_stack_free_words','heap_free_bytes','heap_min_bytes'):
            self.assertIsNone(row['diagnostics'][name],name)


if __name__ == '__main__':
    unittest.main()
