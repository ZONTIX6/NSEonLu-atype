"""CSC1 wire decoding and reproducible experiment metadata (stdlib only).

Wire integers are little endian; binary32 measurements retain their units.
Reception timestamps never become sensor sample timestamps.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
LENGTHS = {1:1024, 2:40, 3:512, 4:48}
KINDS = {1:'metadata', 2:'raw', 3:'cycle', 4:'end'}
WHEELS = ['LF','RF','LB','RB']
KF_INTEGER_FIELDS = ('version','max_gap_us','max_age_us','max_horizon_us')
KF_FLOAT_FIELDS = ('q','r','initial_p00','initial_p11','gate_squared')


def crc16(data):
    crc = 0xffff
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ (0xa001 if crc & 1 else 0)
    return crc


def encode_frame(kind, capture_id, record_index, payload):
    if kind not in LENGTHS or len(payload) != LENGTHS[kind]:
        raise ValueError('CSC1 payload length or kind')
    header = struct.pack('<4sHBBHII',b'CSC1',1,kind,0,len(payload),capture_id,record_index)
    frame = header + payload
    return frame + struct.pack('<H',crc16(frame))


def _unpack_named(fmt, data, offset, names):
    return dict(zip(names.split(), struct.unpack_from('<'+fmt,data,offset)))


def decode_kf_config(data, offset=0):
    out = _unpack_named('4I5fB',data,offset,' '.join(KF_INTEGER_FIELDS+KF_FLOAT_FIELDS+('max_rejections',)))
    out['units']='rotor_rad_s'
    return out


def _checkpoint(data, offset):
    corrected = _unpack_named('7f2Q3I', data, offset,
        'omega alpha p00 p01 p11 innovation innovation_variance corrected_time_us output_time_us sequence source_epoch config_version')
    valid,reason,initialized,seen=struct.unpack_from('<4B',data,offset+56)
    corrected.update(valid=bool(valid),reason=reason)
    result=_unpack_named('Q2IB',data,offset+60,'seen_time_us seen_sequence seen_source_epoch rejected_consecutive')
    result.update(corrected=corrected,initialized=bool(initialized),seen=bool(seen))
    return result


def decode_metadata(data):
    out=_unpack_named('3I4H',data,0,'metadata_version capture_id dwt_hz raw_capacity cycle_capacity raw_bytes cycle_bytes')
    if (out['metadata_version'],out['raw_capacity'],out['cycle_capacity'],out['raw_bytes'],out['cycle_bytes']) != (1,192,32,40,512):
        raise ValueError('unsupported metadata version or dimensions')
    order=list(data[20:24]);directions=list(struct.unpack_from('<4b',data,24))
    if order != [0,1,2,3] or any(d not in (-1,1) for d in directions):
        raise ValueError('invalid wheel order/directions')
    unit_schema,=struct.unpack_from('<I',data,28)
    if unit_schema!=1:raise ValueError('unknown unit schema')
    out.update(wheel_order=WHEELS.copy(),directions=directions,unit_schema=unit_schema,
        units={'speed':'rotor_rpm','kf_speed':'rotor_rad_s','kf_acceleration':'rotor_rad_s2',
               'current':'raw_equivalent','power':'W','time':'us'},sample_time_source='unknown')
    out['clock']=_unpack_named('4IQI',data,32,'source_epoch cycles tick_ms flags now_us fraction')
    out['kf_config']=decode_kf_config(data,64)
    out['checkpoints']=[_checkpoint(data,104+80*w) for w in range(4)]
    out['pid_actual']=[_unpack_named('7fI',data,424+32*w,
        'Kp Ki Kd MaxOut IntegralLimit Output_LPF_RC Derivative_LPF_RC Improve') for w in range(4)]
    out.update(_unpack_named('HHI',data,552,'kf_checksum pid_source dynamic_model_checksum'))
    out['build_identity_sha256']=data[560:592].hex() if out['clock']['flags'] & 8 else None
    out['worktree_patch_sha256']=data[592:624].hex() if out['clock']['flags'] & 16 else None
    out['pid_config_version'],=struct.unpack_from('<I',data,624)
    out['synthetic']=bool(out['clock']['flags'] & 32)
    if out['clock']['flags'] & 2 and crc16(data[64:104])!=out['kf_checksum']:
        raise ValueError('KF parameter checksum mismatch')
    if any(data[628:]) or any(data[60:64]):
        raise ValueError('nonzero reserved metadata bytes')
    return out


def decode_raw(data):
    out=_unpack_named('BBBBIIIhhfhHQI',data,0,
        'wheel temperature sample_source flags sequence rx_tick_ms rx_cycles raw_rpm raw_feedback_current filtered_rpm filtered_current reserved sample_time_us event_index')
    if out['wheel']>=4 or out['sample_source']!=0 or out['reserved']:
        raise ValueError('invalid raw wheel, sample source or reserved field')
    out.pop('reserved')
    out['sample_time_us']=None
    out['sample_time_source']='unknown'
    out['wheel_name']=WHEELS[out['wheel']]
    return out


def decode_cycle(data):
    out=_unpack_named('12IQ4I4BI',data,0,
        'tick_ms start_cycles end_cycles period_us execution_us config_version control_version model_version strategy_version kf_config_version source_epoch cycle_index observation_now_us first_submit_cycles last_submit_cycles flags skipped_event_total enabled_mask online_mask submitted_groups successful_groups capture_rejected_records')
    wheels=[]
    for w in range(4):
        row=_unpack_named('3I2hf2h3f4B7f3I',data,80+80*w,
            'sequence rx_tick_ms rx_cycles raw_rpm raw_feedback_current filtered_rpm filtered_current final_command target_rpm pid_candidate limited_command temperature sender_group kf_reason flags omega alpha p00 p01 p11 innovation innovation_variance accepted_age_us corrected_count skipped_events')
        row['wheel']=w;row['wheel_name']=WHEELS[w]
        row['sample_time_source']='unknown';row['sample_time_us']=None
        row['kf_valid']=bool(row['flags']&2)
        row['raw_time_valid']=bool(row['flags']&1)
        wheels.append(row)
    out['wheels']=wheels
    diagnostics=_unpack_named('f3If3I4f4I3I5I3fI',data,400,
        'input_power_w input_sequence input_tick_ms input_flags load_power_w load_sequence load_tick_ms load_flags old_guard_rpm0 old_guard_rpm1 old_guard_rpm2 old_guard_rpm3 kf_us allocator_us fast_shadow_us slow_shadow_us observation_cycles execution_max_us control_to_submit_end_us motor_stack_free_words robot_stack_free_words observation_stack_free_words heap_free_bytes heap_min_bytes old_candidate_power_w old_guard_power_w physical_budget_w shadow_reason')
    diagnostics['old_guard_rpm']=[diagnostics.pop('old_guard_rpm'+str(w)) for w in range(4)]
    for source in ('input','load'):
        if diagnostics[source+'_flags']&3!=3:diagnostics[source+'_power_w']=None
    out['diagnostics']=diagnostics
    return out


def decode_end(data):
    return _unpack_named('12I',data,0,'raw_count cycle_count freeze_reason rejected_records outside_window_at_drain transport_retries_at_drain start_tick_ms end_tick_ms start_cycles end_cycles source_epoch flags')


def _finite(value):
    if isinstance(value,float):return math.isfinite(value)
    if isinstance(value,dict):return all(_finite(v) for v in value.values())
    if isinstance(value,list):return all(_finite(v) for v in value)
    return True


def decode_capture(data):
    """Return records, windows, errors and completeness; never synthesize samples."""
    data=bytes(data);records=[];errors=[];cursor=0
    decoders={1:decode_metadata,2:decode_raw,3:decode_cycle,4:decode_end}
    while True:
        at=data.find(b'CSC1',cursor)
        if at<0:break
        if len(data)-at<18:
            errors.append({'offset':at,'reason':'truncated_header'});break
        _,schema,kind,flags,size,capture_id,index=struct.unpack_from('<4sHBBHII',data,at)
        if schema!=1 or flags or kind not in LENGTHS or size!=LENGTHS.get(kind):
            errors.append({'offset':at,'reason':'invalid_header'});cursor=at+4;continue
        end=at+20+size
        if end>len(data):errors.append({'offset':at,'reason':'truncated_frame'});break
        if crc16(data[at:end-2])!=struct.unpack_from('<H',data,end-2)[0]:
            errors.append({'offset':at,'reason':'crc'});cursor=at+4;continue
        try:
            payload=decoders[kind](data[at+18:end-2])
            if not _finite(payload):raise ValueError('nonfinite wire value')
            records.append({'type':'record','kind':KINDS[kind],'capture_id':capture_id,
                            'record_index':index,'payload':payload})
        except (ValueError,struct.error) as exc:
            errors.append({'offset':at,'reason':str(exc)})
        cursor=end
    windows=[]
    for capture_id in dict.fromkeys(r['capture_id'] for r in records):
        group=[r for r in records if r['capture_id']==capture_id]
        problems=[]
        by_kind={k:[r for r in group if r['kind']==k] for k in KINDS.values()}
        if len(by_kind['metadata'])!=1:problems.append('metadata_count')
        if len(by_kind['end'])!=1:problems.append('end_count')
        if group[0]['kind']!='metadata' or group[-1]['kind']!='end':problems.append('frame_order')
        ranks=[list(KINDS.values()).index(r['kind']) for r in group]
        if ranks!=sorted(ranks):problems.append('frame_order')
        for kind,limit in (('raw',192),('cycle',32)):
            rows=by_kind[kind]
            if [r['record_index'] for r in rows]!=list(range(len(rows))) or len(rows)>limit:
                problems.append(kind+'_indices')
        if len(by_kind['metadata'])==1:
            m=by_kind['metadata'][0]
            if m['record_index']!=0 or m['payload']['capture_id']!=capture_id:problems.append('metadata_identity')
        if len(by_kind['end'])==1:
            e=by_kind['end'][0]
            if e['record_index']!=0:problems.append('end_index')
            if any(e['payload'][kind+'_count']!=len(by_kind[kind]) for kind in ('raw','cycle')):
                problems.append('record_count')
            if e['payload']['flags']!=1 or e['payload']['rejected_records']:
                problems.append('incomplete_or_lost')
            if not 1<=e['payload']['freeze_reason']<=5:problems.append('freeze_reason')
        windows.append({'capture_id':capture_id,'complete':not problems,'errors':problems,
                        'raw_count':len(by_kind['raw']),'cycle_count':len(by_kind['cycle'])})
    return {'records':records,'windows':windows,'errors':errors,
            'complete':bool(windows) and not errors and all(w['complete'] for w in windows)}


def encode_kf_config(config):
    if config.get('units')!='rotor_rad_s':raise ValueError('KF units must be rotor_rad_s')
    if type(config.get('synthetic',False)) is not bool:raise ValueError('synthetic must be boolean')
    for name in KF_INTEGER_FIELDS:
        v=config.get(name)
        if type(v) is not int or not 1<=v<=0xffffffff:raise ValueError(name+': positive uint32 required')
        if name!='version' and v>=0x80000000:raise ValueError(name+': must be below uint32 half range')
    for name in KF_FLOAT_FIELDS:
        v=config.get(name)
        if type(v) not in (int,float) or not math.isfinite(v) or (v<0 if name=='q' else v<=0):
            raise ValueError(name+': finite valid variance/gate required')
    if type(config.get('max_rejections')) is not int or not 1<=config['max_rejections']<=255:
        raise ValueError('max_rejections: 1..255 required')
    try:
        packed=struct.pack('<4I5fB3x',*(config[n] for n in KF_INTEGER_FIELDS+KF_FLOAT_FIELDS),config['max_rejections'])
    except (OverflowError,struct.error) as exc:raise ValueError('KF binary32 range') from exc
    values=struct.unpack_from('<5f',packed,16)
    if not all(math.isfinite(v) for v in values) or any(v<=0 for v in values[1:]):
        raise ValueError('KF binary32 underflow/overflow')
    return packed


def emit_kf_header(config):
    packed=encode_kf_config(config)
    # Round through binary32: generated text reconstructs exactly the CRC payload.
    normalized=decode_kf_config(packed)
    values=[]
    for name in KF_INTEGER_FIELDS:values.append('    .'+name+' = '+str(normalized[name])+'U,')
    for name in KF_FLOAT_FIELDS:
        literal=format(normalized[name],'.9g')
        if '.' not in literal and 'e' not in literal:literal+='.0'
        values.append('    .'+name+' = '+literal+'f,')
    values.append('    .max_rejections = '+str(normalized['max_rejections'])+'U,')
    return ('/* Generated experimental configuration; provenance remains in manifest. */\n'
        '#ifndef CHASSIS_SPEED_KF_EXPERIMENT_H\n#define CHASSIS_SPEED_KF_EXPERIMENT_H\n'
        '#include "wheel_speed_kf.h"\n'
        'static const PPWheelKFConfig chassis_speed_experiment_kf_config = {\n'+'\n'.join(values)+'\n};\n'
        'static const uint16_t chassis_speed_experiment_kf_checksum = '+str(crc16(packed))+'U;\n'
        'static const bool chassis_speed_experiment_kf_synthetic = '+('true' if config.get('synthetic',False) else 'false')+';\n#endif\n')


MEASURED_FIELDS=('firmware','build_options','toolchain','pid_actual','model','sensors','wiring',
    'trajectory_id','initial_energy_j','final_energy_j','temperature_c','load','wheel_geometry',
    'timing_alignment','splits','frozen_thresholds','capture_integrity')


def _canonical(value):
    return json.dumps(value,ensure_ascii=False,sort_keys=True,separators=(',',':'),allow_nan=False).encode('utf-8')


def build_manifest(metadata, root=ROOT):
    if not isinstance(metadata,dict):raise ValueError('metadata must be an object')
    if type(metadata.get('synthetic',False)) is not bool:raise ValueError('synthetic must be boolean')
    result=json.loads(json.dumps(metadata,allow_nan=False))
    for name in MEASURED_FIELDS:
        if name not in result or result[name] is None:result[name]={'value':None,'status':'unmeasured'}
    def git(*args):
        run=subprocess.run(['git',*args],cwd=root,capture_output=True,check=True)
        return run.stdout
    patch=git('diff','HEAD','--binary')
    untracked={}
    for raw in git('ls-files','--others','--exclude-standard','-z').split(b'\0'):
        if not raw:continue
        relative=raw.decode('utf-8')
        path=Path(root)/relative
        if path.is_file():untracked[relative]=hashlib.sha256(path.read_bytes()).hexdigest()
    artifacts=metadata.get('artifacts',{})
    if not isinstance(artifacts,dict):raise ValueError('artifacts must map names to paths')
    hashes={}
    for name,value in artifacts.items():
        if not isinstance(name,str) or not isinstance(value,str):raise ValueError('artifact name/path must be strings')
        path=Path(value)
        if not path.is_absolute():path=Path(root)/path
        if not path.is_file():raise ValueError('missing artifact: '+value)
        data=path.read_bytes()
        hashes[name]={'path':value,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
    result.update(schema='chassis-speed-experiment/1',synthetic=metadata.get('synthetic',False),
        source={'commit':git('rev-parse','HEAD').decode().strip(),
                'dirty':bool(git('status','--porcelain')),
                'worktree_patch_sha256':hashlib.sha256(patch).hexdigest(),
                'untracked_files':untracked,
                'metadata_sha256':hashlib.sha256(_canonical(metadata)).hexdigest()})
    result['artifact_hashes']=hashes
    result['missing_evidence']=[n for n in MEASURED_FIELDS if isinstance(result[n],dict) and result[n].get('status')=='unmeasured']
    result['hardware_gate']='insufficient_evidence' if result['synthetic'] or result['missing_evidence'] else 'pending_review'
    result.pop('manifest_sha256',None)
    result['manifest_sha256']=hashlib.sha256(_canonical(result)).hexdigest()
    return result


def validate_manifest(manifest):
    if not isinstance(manifest,dict) or manifest.get('schema')!='chassis-speed-experiment/1':return False
    copy=manifest.copy();digest=copy.pop('manifest_sha256',None)
    try:return digest==hashlib.sha256(_canonical(copy)).hexdigest()
    except (TypeError,ValueError):return False


def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__)
    sub=parser.add_subparsers(dest='command',required=True)
    decode=sub.add_parser('decode');decode.add_argument('--input',required=True);decode.add_argument('--output',required=True)
    manifest=sub.add_parser('manifest');manifest.add_argument('--metadata',required=True);manifest.add_argument('--output',required=True)
    emit=sub.add_parser('emit-kf-header');emit.add_argument('--kf-config',required=True);emit.add_argument('--output',required=True)
    args=parser.parse_args(argv)
    try:
        output=Path(args.output);output.parent.mkdir(parents=True,exist_ok=True)
        if args.command=='decode':
            data=Path(args.input).read_bytes();result=decode_capture(data)
            summary={k:v for k,v in result.items() if k!='records'}
            summary.update(type='summary',input_sha256=hashlib.sha256(data).hexdigest())
            output.write_text('\n'.join(json.dumps(r,ensure_ascii=False,allow_nan=False) for r in result['records']+[summary])+'\n',encoding='utf-8')
            return 0 if result['complete'] else 2
        if args.command=='manifest':
            result=build_manifest(json.loads(Path(args.metadata).read_text(encoding='utf-8')))
            output.write_text(json.dumps(result,ensure_ascii=False,indent=2,allow_nan=False)+'\n',encoding='utf-8')
        else:
            output.write_text(emit_kf_header(json.loads(Path(args.kf_config).read_text(encoding='utf-8'))),encoding='utf-8')
        return 0
    except (ValueError,OSError,subprocess.CalledProcessError) as exc:
        print(str(exc),file=sys.stderr);return 1


if __name__=='__main__':raise SystemExit(main())
