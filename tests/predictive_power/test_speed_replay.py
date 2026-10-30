"""Behavioral tests for production-C observation replay, not vehicle calibration."""
import copy
import ctypes as ct
import importlib.util
import json
import math
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import chassis_speed_capture as capture

SPEC = importlib.util.spec_from_file_location('speed_replay', ROOT/'scripts/wheel-speed-replay.py')
replay = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(replay)


def config():
    return dict(version=1, max_gap_us=20000, max_age_us=20000, max_horizon_us=20000,
                q=10, r=.1, initial_p00=1, initial_p11=100, gate_squared=10000,
                max_rejections=3, units='rotor_rad_s', synthetic=True)


def records(count=24, capture_id=1, base=100000):
    meta = dict(dwt_hz=1000000, unit_schema=1, wheel_order=['LF','RF','LB','RB'],
                kf_config=config(), synthetic=True, checkpoints=[],
                clock=dict(source_epoch=1, cycles=base, tick_ms=base//1000,
                           now_us=base, fraction=0, flags=1))
    out = [dict(kind='metadata', capture_id=capture_id, record_index=0, payload=meta)]
    for n in range(count):
        now = base+n*2000
        wheels=[]
        for w in range(4):
            truth=5+w+2*n*.002
            rpm=round(truth*60/(2*math.pi))
            wheels.append(dict(sequence=n+1, rx_tick_ms=now//1000, rx_cycles=now,
                raw_rpm=rpm, filtered_rpm=rpm, raw_feedback_current=2,
                filtered_current=2, target_rpm=rpm, pid_candidate=100,
                limited_command=100, final_command=100, flags=1|8|16,
                sender_group=0, truth_omega=truth))
        payload=dict(observation_now_us=now, start_cycles=now, end_cycles=now+400,
            tick_ms=now//1000, source_epoch=1, config_version=1, model_version=0,
            kf_config_version=1, cycle_index=n, flags=3, period_us=2000,
            execution_us=400, online_mask=15, enabled_mask=15,
            submitted_groups=1, successful_groups=1, wheels=wheels,
            diagnostics=dict(observation_cycles=now,kf_us=5,allocator_us=20,
                fast_shadow_us=40,slow_shadow_us=50,control_to_submit_end_us=30,
                old_guard_rpm=[abs(w['filtered_rpm']) for w in wheels],
                input_flags=0,load_flags=0,input_power_w=None,load_power_w=None))
        out.append(dict(kind='cycle',capture_id=capture_id,record_index=n,payload=payload))
    out.append(dict(kind='end',capture_id=capture_id,record_index=0,
                    payload=dict(raw_count=0,cycle_count=count,rejected_records=0,flags=1)))
    return out


class Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lib=replay.load_library(ROOT/'build/speed-replay-tests')

    def manifest(self, **extra):
        return capture.build_manifest(dict(synthetic=True, trajectory_id='synthetic-ramp',
                                          **extra))

    def test_short_window_reports_horizons_without_inventing_power_evidence(self):
        result=replay.replay(self.lib,records(),self.manifest(),config(),include_predictions=True)
        self.assertTrue(result['synthetic'])
        self.assertFalse(result['power']['available'])
        self.assertEqual(result['power']['reason'],'unverified_measurement_boundary')
        self.assertEqual(result['hardware_gate'],'insufficient_evidence')
        self.assertFalse(result['windows']['stitched'])
        for horizon,want in ((2000,23),(8000,20),(20000,14)):
            metric=result['wheels']['LF']['horizons'][str(horizon)]['kf']
            self.assertEqual(metric['samples'],want)
            self.assertIsNotNone(metric['observed_future_rmse_rad_s'])
            self.assertIsNotNone(metric['synthetic_truth_rmse_rad_s'])
        self.assertEqual(result['timing']['execution_us']['max'],400)
        self.assertEqual(result['trajectory']['reason'],'not_observable_in_window')

    def test_future_measurements_do_not_change_existing_predictions(self):
        a=records(); b=copy.deepcopy(a)
        for row in b:
            if row['kind']=='cycle' and row['payload']['cycle_index']>=10:
                for wheel in row['payload']['wheels']:
                    wheel['raw_rpm']+=1000
                    wheel['filtered_rpm']+=1000
                    wheel['truth_omega']+=100
        first=replay.replay(self.lib,a,self.manifest(),config(),include_predictions=True)
        second=replay.replay(self.lib,b,self.manifest(),config(),include_predictions=True)
        prefix=lambda report:[p for p in report['predictions'] if p['cycle_index']<10]
        self.assertEqual(prefix(first),prefix(second))
        self.assertNotEqual(first['wheels']['LF']['horizons']['20000'],
                            second['wheels']['LF']['horizons']['20000'])

    def test_windows_and_version_boundaries_are_never_joined(self):
        rows=records(6)+records(6,2,112000)
        result=replay.replay(self.lib,rows,self.manifest(),config())
        self.assertEqual(result['wheels']['LF']['horizons']['20000']['kf']['samples'],0)
        rows=records()
        for row in rows:
            if row['kind']=='cycle' and row['payload']['cycle_index']>=12:
                row['payload']['config_version']=2
        result=replay.replay(self.lib,rows,self.manifest(),config())
        self.assertEqual(result['wheels']['LF']['horizons']['20000']['kf']['samples'],4)

    def test_manifest_tampering_invalid_units_and_no_samples_fail(self):
        manifest=self.manifest(); manifest['synthetic']=False
        with self.assertRaisesRegex(ValueError,'manifest'):
            replay.replay(self.lib,records(),manifest,config())
        bad=config(); bad['units']='wheel_rpm'
        with self.assertRaises(ValueError):replay.replay(self.lib,records(),self.manifest(),bad)
        with self.assertRaisesRegex(ValueError,'valid.*sample'):
            replay.replay(self.lib,records(0),self.manifest(),config())

    def test_duplicate_and_invalid_frames_do_not_refresh_freshness(self):
        rows=records()
        for row in rows:
            if row['kind']=='cycle' and row['payload']['cycle_index']>0:
                row['payload']['wheels'][0]['sequence']=1
                row['payload']['wheels'][0]['rx_cycles']=100000
                row['payload']['wheels'][0]['rx_tick_ms']=100
        result=replay.replay(self.lib,rows,self.manifest(),config())
        self.assertEqual(result['wheels']['LF']['rejected']['duplicate'],10)
        self.assertEqual(result['wheels']['LF']['accepted_events'],1)
        self.assertLess(result['wheels']['LF']['valid_fraction'],1)

    def test_hot_checkpoint_reproduces_production_state(self):
        cfg=replay.read_config(self.lib,config()); state=replay.Filter()
        for n in range(100):
            s=replay.Sample(100000+n*2000,n+1,1,10+.004*n,True)
            self.lib.PPWheelKFCorrect(ct.byref(state),ct.byref(cfg),ct.byref(s))
        rows=records(12,base=300000)
        rows[0]['payload']['clock']['flags']=7
        rows[0]['payload']['checkpoints']=[replay.filter_to_dict(state) for _ in range(4)]
        expected=[]
        for row in rows:
            if row['kind']!='cycle':continue
            p=row['payload']; w=p['wheels'][0]
            w['sequence']+=100
            s=replay.Sample(p['observation_now_us'],w['sequence'],1,w['raw_rpm']*2*math.pi/60,True)
            self.lib.PPWheelKFCorrect(ct.byref(state),ct.byref(cfg),ct.byref(s))
            expected.append(state.corrected.omega)
        result=replay.replay(self.lib,rows,self.manifest(),config(),include_predictions=True)
        actual=[p['corrected_omega'] for p in result['predictions']
                if p['wheel']=='LF' and p['horizon_us']==2000]
        self.assertEqual(actual,expected)
        self.assertEqual(result['windows']['checkpoint_restored'],1)

    def test_all_events_are_reported_separately_from_latest_cycle_samples(self):
        rows=records();raw=[]
        for n in range(48):
            for w in range(4):
                raw.append(dict(kind='raw',capture_id=1,record_index=n*4+w,
                    payload=dict(wheel=w,sequence=n+1,rx_cycles=100000+n*1000,
                        rx_tick_ms=100+n,raw_rpm=100,filtered_rpm=100)))
        rows[1:1]=raw
        rows[-1]['payload']['raw_count']=len(raw)
        result=replay.replay(self.lib,rows,self.manifest(),config())
        self.assertEqual(result['wheels']['LF']['accepted_events'],24)
        self.assertEqual(result['all_events']['wheels']['LF']['accepted_events'],48)
        self.assertEqual(result['all_events']['wheels']['LF']['horizons']['8000']['samples'],40)

    def test_verified_power_boundaries_use_same_c_model_and_separate_excess_energy(self):
        rows=records()
        for row in rows:
            if row['kind']=='cycle':
                p=row['payload'];p['diagnostics'].update(input_flags=15,load_flags=15,
                    input_sequence=p['cycle_index']+1,load_sequence=p['cycle_index']+1,
                    input_power_w=5,load_power_w=3)
        model=dict(k0=.5,k1=0,k2=0,k3=0,k4=0,k5=0,current_conversion=1000)
        manifest=self.manifest(wiring=dict(input_boundary_verified=True,load_boundary_verified=True),
            timing_alignment=dict(verified=True),model=dict(models=[model]*4,calibration_evidence='synthetic'),
            power_limits_w=dict(input=4,load=1))
        result=replay.replay(self.lib,rows,manifest,config())
        self.assertTrue(result['power']['available'])
        self.assertAlmostEqual(result['power']['underprediction_w']['kf']['max'],1)
        self.assertAlmostEqual(result['power']['excess_energy_j']['input'],.046)
        self.assertAlmostEqual(result['power']['excess_energy_j']['load'],.092)

    def test_complete_trajectory_annotation_enables_integrals_only_for_covered_interval(self):
        rows=records()
        for row in rows:
            if row['kind']=='cycle':
                for w in row['payload']['wheels']:
                    w['raw_rpm']=0;w['target_rpm']=60
        manifest=self.manifest(trajectory=dict(complete=True,start_us=100000,end_us=146000,
                                              steady_start_us=130000))
        result=replay.replay(self.lib,rows,manifest,config())
        self.assertTrue(result['trajectory']['available'])
        self.assertAlmostEqual(result['trajectory']['wheels']['LF']['speed_iae_rad'],2*math.pi*.046)
        self.assertEqual(result['trajectory']['wheels']['LF']['successful_command_total_variation'],0)
        manifest=self.manifest(trajectory=dict(complete=True,start_us=100000,end_us=200000))
        self.assertFalse(replay.replay(self.lib,rows,manifest,config())['trajectory']['available'])

    def test_out_of_order_measurement_is_not_used_as_a_scoring_reference(self):
        rows=records()
        damaged=rows[5]['payload']
        for wheel in damaged['wheels']:
            wheel['rx_cycles']=100000;wheel['rx_tick_ms']=100;wheel['raw_rpm']=30000
        result=replay.replay(self.lib,rows,self.manifest(),config(),include_predictions=True)
        self.assertFalse(any(p['cycle_index']==4 for p in result['predictions']))
        self.assertEqual(result['wheels']['LF']['rejected']['time'],1)

    def test_missing_records_cannot_be_labelled_a_complete_window(self):
        rows=records();rows.pop(5)
        result=replay.replay(self.lib,rows,self.manifest(),config())
        self.assertEqual(result['windows']['incomplete'],1)
        self.assertFalse(result['trials'][0]['complete'])


if __name__=='__main__':unittest.main()
