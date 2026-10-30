"""Compare CSC1 wheel observations using the production C Kalman filter.

Reports receive-time prediction quality, not synchronized acceleration truth.
Short windows are kept separate. No report produced here authorizes takeover.
"""
import argparse
from bisect import bisect_left
from collections import Counter, defaultdict
import ctypes as ct
import hashlib
import json
import math
from pathlib import Path
import statistics
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
import chassis_speed_capture as capture

WHEELS=('LF','RF','LB','RB')
HORIZONS=(2000,8000,20000)
REASONS=('ok','initialized','duplicate','time','value','innovation','stale','config','numeric')
SCALE=2*math.pi/60
UINT32_MAX=0xffffffff


class Config(ct.Structure):
    _fields_=[(n,ct.c_uint32) for n in ('version','max_gap_us','max_age_us','max_horizon_us')]+[
        (n,ct.c_float) for n in ('q','r','initial_p00','initial_p11','gate_squared')]+[('max_rejections',ct.c_uint8)]


class Sample(ct.Structure):
    _fields_=[('rx_time_us',ct.c_uint64),('sequence',ct.c_uint32),('source_epoch',ct.c_uint32),
              ('omega_rad_s',ct.c_float),('online',ct.c_bool)]


class Estimate(ct.Structure):
    _fields_=[(n,ct.c_float) for n in ('omega','alpha','p00','p01','p11','innovation','innovation_variance')]+[
        ('corrected_time_us',ct.c_uint64),('output_time_us',ct.c_uint64)]+[
        (n,ct.c_uint32) for n in ('sequence','source_epoch','config_version')]+[('valid',ct.c_bool),('reason',ct.c_int)]


class Filter(ct.Structure):
    _fields_=[('corrected',Estimate),('seen_time_us',ct.c_uint64),('seen_sequence',ct.c_uint32),
        ('seen_source_epoch',ct.c_uint32),('rejected_consecutive',ct.c_uint8),('seen',ct.c_bool),('initialized',ct.c_bool)]


class PowerModel(ct.Structure):
    _fields_=[(n,ct.c_float) for n in ('k0','k1','k2','k3','k4','k5','current_conversion')]


def load_library(directory):
    directory=Path(directory);directory.mkdir(parents=True,exist_ok=True)
    path=directory/('wheel_speed.dll' if sys.platform=='win32' else 'wheel_speed.so')
    sources=[ROOT/'modules/predictive_power/wheel_speed_kf.c',ROOT/'modules/algorithm/power_model.c',
             ROOT/'tests/predictive_power/wheel_speed_replay_bridge.c']
    subprocess.run(['gcc','-shared','-fPIC','-std=c11','-O2','-fno-fast-math','-Wall','-Wextra','-Werror',
        '-I'+str(ROOT/'modules/predictive_power'),*map(str,sources),'-lm','-o',str(path)],check=True)
    lib=ct.CDLL(str(path.resolve()))
    lib.PPReplayAbi.argtypes=[ct.c_uint,ct.c_uint];lib.PPReplayAbi.restype=ct.c_size_t
    for index,kind in enumerate((Config,Sample,Estimate,Filter)):
        layout=[ct.sizeof(kind)]+[getattr(kind,name).offset for name,_ in kind._fields_]
        if any(lib.PPReplayAbi(index,j)!=offset for j,offset in enumerate(layout)):
            raise RuntimeError('Production C/Python ABI mismatch: '+kind.__name__)
    lib.PPWheelKFConfigValid.argtypes=[ct.POINTER(Config)];lib.PPWheelKFConfigValid.restype=ct.c_bool
    lib.PPWheelKFCorrect.argtypes=[ct.POINTER(Filter),ct.POINTER(Config),ct.POINTER(Sample)]
    lib.PPWheelKFCorrect.restype=ct.c_int
    lib.PPWheelKFPredict.argtypes=[ct.POINTER(Filter),ct.POINTER(Config),ct.c_uint64,ct.c_uint32,ct.POINTER(Estimate)]
    lib.PPWheelKFPredict.restype=ct.c_bool
    lib.PowerModelConfigIsValid.argtypes=[ct.POINTER(PowerModel)];lib.PowerModelConfigIsValid.restype=ct.c_bool
    lib.PowerModelPredict.argtypes=[ct.POINTER(PowerModel),ct.c_float,ct.c_float];lib.PowerModelPredict.restype=ct.c_float
    return lib


def read_config(lib,values):
    capture.encode_kf_config(values)  # Strict units, ranges, booleans and finite float checks.
    config=Config(**{name:values[name] for name,_ in Config._fields_})
    if not lib.PPWheelKFConfigValid(ct.byref(config)):raise ValueError('production C rejected KF configuration')
    return config


def filter_to_dict(state):
    out={n:getattr(state,n) for n,_ in Filter._fields_ if n!='corrected'}
    out['corrected']={n:getattr(state.corrected,n) for n,_ in Estimate._fields_}
    return out


def restore_filter(lib,values,config):
    state=Filter()
    for name,kind in Filter._fields_:
        if name=='corrected':
            for field,_ in Estimate._fields_:setattr(state.corrected,field,values['corrected'][field])
        else:setattr(state,name,values[name])
    if state.initialized:
        check=Estimate()
        if not lib.PPWheelKFPredict(ct.byref(state),ct.byref(config),
                state.corrected.corrected_time_us,0,ct.byref(check)):
            raise ValueError('invalid KF checkpoint')
    return state


def summary(values):
    values=sorted(v for v in values if v is not None and math.isfinite(v))
    if not values:return dict(samples=0,p50=None,p95=None,p99=None,max=None)
    def percentile(q):
        position=(len(values)-1)*q;low=int(position);high=min(low+1,len(values)-1)
        return values[low]+(values[high]-values[low])*(position-low)
    return dict(samples=len(values),p50=percentile(.5),p95=percentile(.95),p99=percentile(.99),max=values[-1])


def _window_records(records):
    windows=[];active=None
    for row in records:
        if row.get('type')=='summary':continue
        kind=row.get('kind')
        if kind=='metadata':
            active=dict(capture_id=row['capture_id'],metadata=row['payload'],cycles=[],raw=[],end=None)
            windows.append(active)
        elif active is not None and row['capture_id']==active['capture_id']:
            if kind=='cycle':active['cycles'].append(row['payload'])
            elif kind=='raw':active['raw'].append(row['payload'])
            elif kind=='end':active['end']=row['payload'];active=None
        else:raise ValueError('record outside a capture window')
    return windows


def _window_complete(win):
    end=win['end']
    return bool(end and end.get('flags',0)&1 and not end.get('flags',0)&2 and
        not end.get('rejected_records',0) and end.get('raw_count')==len(win['raw']) and
        end.get('cycle_count')==len(win['cycles']))


def _same_config(metadata,values):
    try:return capture.encode_kf_config(metadata)==capture.encode_kf_config(values)
    except (ValueError,KeyError,TypeError):return False


def _receive_time(now,cycles,tick,wheel,hz,max_age):
    age=((cycles-wheel['rx_cycles'])&UINT32_MAX)*1000000//hz
    coarse=((tick-wheel['rx_tick_ms'])&UINT32_MAX)*1000
    if age>max_age or coarse>max_age+1000 or abs(coarse-age)>2000 or age>now:return None
    return now-age


def _metric(errors,truth_errors,under):
    return dict(samples=len(errors),observed_future_rmse_rad_s=(math.sqrt(sum(x*x for x in errors)/len(errors)) if errors else None),
        synthetic_truth_rmse_rad_s=(math.sqrt(sum(x*x for x in truth_errors)/len(truth_errors)) if truth_errors else None),
        accelerating_speed_underprediction_rad_s=summary(under))


def _all_events(lib,windows,config,config_values,tolerance):
    """Use every raw event as a separate experiment; never credit added samples
    to the online latest-frame filter. Window-local time is receive time only."""
    accepted=[0]*4;rejected=[Counter() for _ in WHEELS];errors=defaultdict(list)
    for win in windows:
        meta=win['metadata'];clock=meta['clock'];hz=meta['dwt_hz']
        filters=[Filter() for _ in WHEELS];samples=[[] for _ in WHEELS];forecasts=[[] for _ in WHEELS]
        same=_same_config(meta.get('kf_config',{}),config_values)
        if clock.get('flags',0)&4 and same:
            filters=[restore_filter(lib,v,config) for v in meta['checkpoints']]
        for event in win['raw']:
            w=event['wheel'];delta=(event['rx_cycles']-clock['cycles'])&UINT32_MAX
            if delta>=0x80000000:delta-=0x100000000
            now=clock['now_us']+(delta*1000000+clock.get('fraction',0))//hz
            if now<0:rejected[w]['time']+=1;continue
            s=Sample(now,event['sequence'],clock['source_epoch'],event['raw_rpm']*SCALE,True)
            reason=lib.PPWheelKFCorrect(ct.byref(filters[w]),ct.byref(config),ct.byref(s))
            if reason not in (0,1):rejected[w][REASONS[reason]]+=1;continue
            accepted[w]+=1;samples[w].append((now,event['raw_rpm']*SCALE))
            for horizon in HORIZONS:
                e=Estimate()
                if lib.PPWheelKFPredict(ct.byref(filters[w]),ct.byref(config),now,horizon,ct.byref(e)):
                    forecasts[w].append((now,horizon,e.omega))
        for w in range(4):
            times=[t for t,_ in samples[w]]
            for now,horizon,predicted in forecasts[w]:
                index=bisect_left(times,now+horizon)
                choices=[i for i in (index-1,index) if 0<=i<len(times) and times[i]>now]
                if choices:
                    i=min(choices,key=lambda i:abs(times[i]-now-horizon))
                    if abs(times[i]-now-horizon)<=tolerance:errors[(w,horizon)].append(predicted-samples[w][i][1])
    return dict(available=bool(sum(accepted)),sampling='all_raw_events',attributable_to_online_kf=False,
        wheels={name:dict(accepted_events=accepted[w],rejected=dict(rejected[w]),
            horizons={str(h):_metric(errors[(w,h)],[],[]) for h in HORIZONS}) for w,name in enumerate(WHEELS)})


def _trajectory_report(windows,manifest):
    annotation=manifest.get('trajectory',{})
    unavailable=dict(available=False,reason='not_observable_in_window')
    if not isinstance(annotation,dict) or annotation.get('complete') is not True or len(windows)!=1:return unavailable
    win=windows[0];cycles=win['cycles']
    if not _window_complete(win) or len(cycles)<2:return unavailable
    first,last=cycles[0]['observation_now_us'],cycles[-1]['observation_now_us']
    if annotation.get('start_us',-1)<first or annotation.get('end_us',last+1)>last:return unavailable
    chosen=[p for p in cycles if annotation['start_us']<=p['observation_now_us']<=annotation['end_us']]
    if len(chosen)<2:return unavailable
    wheels={}
    for w,name in enumerate(WHEELS):
        iae=0.;variation=0.;peak=0.;steady=[];previous=None;previous_command=None;steps=[]
        response=[];raw_values=[]
        for p in chosen:
            row=p['wheels'][w];now=p['observation_now_us']
            valid=bool(p['online_mask']&(1<<w)) and bool(row['flags']&1)
            error=(row['target_rpm']-row['raw_rpm'])*SCALE
            if valid:
                if previous:
                    dt=(now-previous[0])*1e-6
                    if not 0<dt<=.02:return unavailable
                    iae+=.5*(abs(previous[1])+abs(error))*dt
                    if row['target_rpm']!=previous[2]:steps.append((now,previous[2],row['target_rpm']))
                previous=(now,error,row['target_rpm']);raw_values.append((now,row['raw_rpm']))
                if now>=annotation.get('steady_start_us',last+1):steady.append(error)
            else:previous=None
            submitted=bool(row['flags']&16)
            if submitted:
                command=row['final_command'];peak=max(peak,abs(command))
                if previous_command is not None:variation+=abs(command-previous_command)
                previous_command=command
            else:previous_command=None
        for index,(start,old,target) in enumerate(steps):
            end=steps[index+1][0] if index+1<len(steps) else last
            direction=1 if target>old else -1;threshold=old+.9*(target-old)
            crossing=next((t for t,actual in raw_values if start<=t<end and direction*(actual-threshold)>=0),None)
            response.append(dict(step_us=start,target_rpm=target,t90_s=(crossing-start)*1e-6 if crossing is not None else None,
                                 reason='observed' if crossing is not None else 'not_observable_in_window'))
        wheels[name]=dict(speed_iae_rad=iae,steady_error_rad_s=statistics.mean(steady) if steady else None,
            steady_error_std_rad_s=statistics.pstdev(steady) if len(steady)>1 else None,
            successful_command_total_variation=variation,successful_command_peak=peak,
            step_responses=response)
    return dict(available=True,measurement='receive_time_wheel_feedback',duration_s=(chosen[-1]['observation_now_us']-chosen[0]['observation_now_us'])*1e-6,
                wheels=wheels)


def _power_report(lib,windows,manifest,states):
    wiring=manifest.get('wiring',{});align=manifest.get('timing_alignment',{});model=manifest.get('model',{})
    if not (isinstance(wiring,dict) and wiring.get('input_boundary_verified') is True and
            wiring.get('load_boundary_verified') is True and isinstance(align,dict) and align.get('verified') is True):
        return dict(available=False,reason='unverified_measurement_boundary')
    values=model.get('models') if isinstance(model,dict) else None
    if not isinstance(values,list) or len(values)!=4 or not model.get('calibration_evidence'):
        return dict(available=False,reason='unverified_power_model')
    models=[]
    for value in values:
        item=PowerModel(**{name:float(value[name]) for name,_ in PowerModel._fields_})
        if not lib.PowerModelConfigIsValid(ct.byref(item)):raise ValueError('invalid power model')
        models.append(item)
    errors={name:[] for name in ('raw','legacy_filtered','kf')}
    excess={'input':0.0,'load':0.0};samples={'input':0,'load':0}
    limits=manifest.get('power_limits_w',{})
    for window_index,win in enumerate(windows):
        previous={};seen_load=None
        for p in win['cycles']:
            now=p['observation_now_us'];d=p['diagnostics']
            for source in ('input','load'):
                value=d.get(source+'_power_w');seq=d.get(source+'_sequence')
                if d.get(source+'_flags',0)&15!=15 or value is None:previous.pop(source,None);continue
                old=previous.get(source)
                if old and seq==old[1]:continue
                limit=limits.get(source)
                if old and limit is not None and 0<now-old[0]<=20000:
                    excess[source]+=.5*(max(0,old[2]-limit)+max(0,value-limit))*(now-old[0])*1e-6
                    samples[source]+=1
                previous[source]=(now,seq,value)
            if d.get('load_flags',0)&15!=15 or d.get('load_power_w') is None or d.get('load_sequence')==seen_load:continue
            seen_load=d.get('load_sequence');omega=states.get((window_index,p['cycle_index']))
            if omega is None or any(v is None for v in omega):continue
            for method in errors:
                total=0
                for w,row in enumerate(p['wheels']):
                    rpm=(row['raw_rpm'] if method=='raw' else row['filtered_rpm'] if method=='legacy_filtered' else omega[w]/SCALE)
                    total+=lib.PowerModelPredict(ct.byref(models[w]),row['raw_feedback_current'],rpm)
                if not math.isfinite(total):raise ValueError('nonfinite power prediction')
                errors[method].append(max(0,d['load_power_w']-total))
    return dict(available=bool(errors['kf']),reason='same_model_receive_time_comparison',
                underprediction_w={name:summary(values) for name,values in errors.items()},
                excess_energy_j={source:excess[source] if samples[source] else None for source in excess},
                excess_energy_intervals=samples,hard_bound=False)


def replay(lib,records,manifest,config_values,include_predictions=False):
    if not capture.validate_manifest(manifest):raise ValueError('manifest checksum mismatch or unsupported schema')
    config=read_config(lib,config_values);windows=_window_records(records)
    tau=float(config_values.get('comparison_lpf_tau_s',.01))
    tolerance=int(config_values.get('score_tolerance_us',500))
    if not math.isfinite(tau) or tau<=0 or not 0<=tolerance<=5000:raise ValueError('invalid comparison settings')
    synthetic=bool(manifest.get('synthetic'))
    predictions=[];points=defaultdict(list);timing=defaultdict(list);current_states={}
    rejected=[Counter() for _ in WHEELS];accepted=[0]*4;valid=[0]*4;attempts=[0]*4
    innovations=[[] for _ in WHEELS];raw_noise=[[] for _ in WHEELS]
    checkpoint_count=0;incomplete=0;missing_cycles_clock=0
    for window_index,win in enumerate(windows):
        meta=win['metadata'];hz=meta.get('dwt_hz',0)
        if meta.get('unit_schema')!=1 or hz<=0:raise ValueError('invalid capture units or DWT clock')
        if not _window_complete(win):incomplete+=1
        filters=[Filter() for _ in WHEELS]
        same_config=_same_config(meta.get('kf_config',{}),config_values)
        if meta['clock'].get('flags',0)&4 and same_config:
            if len(meta.get('checkpoints',[]))!=4:raise ValueError('missing warm KF checkpoint')
            filters=[restore_filter(lib,v,config) for v in meta['checkpoints']];checkpoint_count+=1
        low=[None]*4;last_time=[None]*4;last_filtered=[None]*4;last_raw=[None]*4
        alpha=[0.0]*4;legacy_alpha=[0.0]*4
        last_sequence=[None]*4;segment=0;previous_versions=None;previous_now=None
        for p in win['cycles']:
            now=int(p['observation_now_us']);d=p['diagnostics']
            versions=(p['source_epoch'],p['config_version'],p.get('model_version',0),p.get('kf_config_version',0))
            if previous_versions is not None and (versions!=previous_versions or now<=previous_now or now-previous_now>config.max_gap_us):
                segment+=1;low=[None]*4;last_time=[None]*4;last_filtered=[None]*4;last_raw=[None]*4;last_sequence=[None]*4
                alpha=[0.0]*4;legacy_alpha=[0.0]*4
            previous_versions,previous_now=versions,now
            cycle_clock=d.get('observation_cycles')
            if cycle_clock is None:
                missing_cycles_clock+=1
                continue  # A motor task start is not an observation timestamp.
            for name in ('period_us','execution_us'):
                if p.get(name) is not None and p[name]!=UINT32_MAX:timing[name].append(p[name])
            for name in ('kf_us','allocator_us','fast_shadow_us','slow_shadow_us','control_to_submit_end_us'):
                if d.get(name) is not None and d[name]!=UINT32_MAX:timing[name].append(d[name])
            state_row=[]
            for w,row in enumerate(p['wheels']):
                attempts[w]+=1
                rx=_receive_time(now,cycle_clock,p['tick_ms'],row,hz,config.max_age_us)
                online=bool(p['online_mask']&(1<<w)) and bool(row['flags']&1) and rx is not None
                if not online:
                    filters[w].initialized=False;filters[w].corrected.valid=False
                    rejected[w]['time' if rx is None else 'stale']+=1;state_row.append(None);continue
                raw=row['raw_rpm']*SCALE
                s=Sample(rx,row['sequence'],p['source_epoch'],raw,True)
                reason=lib.PPWheelKFCorrect(ct.byref(filters[w]),ct.byref(config),ct.byref(s))
                if reason in (0,1):accepted[w]+=1
                else:rejected[w][REASONS[reason]]+=1
                current=Estimate()
                current_valid=lib.PPWheelKFPredict(ct.byref(filters[w]),ct.byref(config),now,0,ct.byref(current))
                state_row.append(current.omega if current_valid else None)
                if current_valid:valid[w]+=1
                if reason in (0,1,5):innovations[w].append(filters[w].corrected.innovation)
                if reason in (3,4,7,8):continue
                new_event=last_sequence[w] is None or 0<((row['sequence']-last_sequence[w])&UINT32_MAX)<0x80000000
                if new_event:
                    if last_time[w] is None:low[w]=raw
                    elif rx>last_time[w]:
                        dt=(rx-last_time[w])*1e-6;beta=dt/(tau+dt)
                        updated=low[w]+beta*(raw-low[w]);alpha[w]=(updated-low[w])/dt;low[w]=updated
                    if last_filtered[w] is not None and last_time[w] is not None and rx>last_time[w]:
                        legacy_alpha[w]=(row['filtered_rpm']*SCALE-last_filtered[w])/((rx-last_time[w])*1e-6)
                    else:legacy_alpha[w]=0.0
                    last_time[w]=rx;last_sequence[w]=row['sequence'];last_filtered[w]=row['filtered_rpm']*SCALE
                    last_raw[w]=raw
                    truth=row.get('truth_omega') if synthetic else None
                    points[(window_index,segment,w)].append((rx,raw,truth))
                    raw_noise[w].append(raw)
                elif last_raw[w] is not None:raw=last_raw[w]
                for horizon in HORIZONS:
                    e=Estimate();ok=lib.PPWheelKFPredict(ct.byref(filters[w]),ct.byref(config),now,horizon,ct.byref(e))
                    guard=d.get('old_guard_rpm',[None]*4)[w]
                    values={'raw_hold':raw,'legacy_filtered':last_filtered[w]+legacy_alpha[w]*horizon*1e-6 if last_filtered[w] is not None else None,
                            'dt_lpf':low[w]+alpha[w]*horizon*1e-6 if low[w] is not None else None,
                            'kf':e.omega if ok else None,
                            'legacy_guard_abs':guard*SCALE if guard is not None and horizon==8000 else None}
                    predictions.append(dict(window=window_index,segment=segment,capture_id=win['capture_id'],
                        cycle_index=p['cycle_index'],wheel=WHEELS[w],horizon_us=horizon,
                        origin_us=now,target_us=now+horizon,values=values,
                        corrected_omega=filters[w].corrected.omega,kf_p00=e.p00 if ok else None,
                        accepted_age_us=now-filters[w].corrected.corrected_time_us,
                        old_guard_abs_rad_s=d.get('old_guard_rpm',[None]*4)[w]*SCALE
                            if d.get('old_guard_rpm',[None]*4)[w] is not None else None))
            current_states[(window_index,p['cycle_index'])]=state_row
    if not sum(accepted):raise ValueError('no valid wheel speed samples')
    errors=defaultdict(list);truth_errors=defaultdict(list);under=defaultdict(list)
    for pred in predictions:
        w=WHEELS.index(pred['wheel']);ref=points[(pred['window'],pred['segment'],w)]
        times=[point[0] for point in ref];index=bisect_left(times,pred['target_us'])
        choices=[j for j in (index-1,index) if 0<=j<len(ref) and ref[j][0]>pred['origin_us']]
        if not choices:continue
        index=min(choices,key=lambda j:abs(ref[j][0]-pred['target_us']))
        if abs(ref[index][0]-pred['target_us'])>tolerance:continue
        _,observed,truth=ref[index]
        for method,value in pred['values'].items():
            if value is None:continue
            key=(w,pred['horizon_us'],method)
            observed_reference=abs(observed) if method=='legacy_guard_abs' else observed
            errors[key].append(value-observed_reference)
            if truth is not None:truth_errors[key].append(value-(abs(truth) if method=='legacy_guard_abs' else truth))
            if abs(observed)>abs(pred['values']['raw_hold']):under[key].append(max(0,abs(observed)-abs(value)))
    wheel_reports={}
    for w,name in enumerate(WHEELS):
        wheel_reports[name]=dict(accepted_events=accepted[w],attempted_cycles=attempts[w],rejected=dict(rejected[w]),
            valid_fraction=valid[w]/attempts[w] if attempts[w] else 0,innovation=summary(innovations[w]),
            window_raw_speed_std_rad_s=statistics.pstdev(raw_noise[w]) if len(raw_noise[w])>1 else None,
            steady_state_noise=dict(available=False,reason='requires_stationary_segment_annotation'),
            horizons={str(h):{method:_metric(errors[(w,h,method)],truth_errors[(w,h,method)],under[(w,h,method)])
                      for method in ('raw_hold','legacy_filtered','dt_lpf','kf','legacy_guard_abs')} for h in HORIZONS})
    result=dict(schema='wheel-speed-report/1',synthetic=synthetic,
        hardware_gate='insufficient_evidence' if synthetic or manifest.get('missing_evidence') or incomplete else 'pending_review',
        windows=dict(count=len(windows),stitched=False,incomplete=incomplete,checkpoint_restored=checkpoint_count),
        sampling='online_latest',missing_observation_clock_cycles=missing_cycles_clock,
        reference='future_raw_receive_time_observations',measurement_sample_time='unknown',
        comparison=dict(lpf_tau_s=tau,score_tolerance_us=tolerance,horizons_us=list(HORIZONS),
                        kf_config_sha256=hashlib.sha256(capture.encode_kf_config(config_values)).hexdigest()),
        wheels=wheel_reports,timing={name:summary(values) for name,values in timing.items()},
        overruns=dict(fast=sum(v>300 for v in timing['fast_shadow_us']),slow=sum(v>2000 for v in timing['slow_shadow_us'])),
        power=_power_report(lib,windows,manifest,current_states),
        trajectory=_trajectory_report(windows,manifest),
        all_events=_all_events(lib,windows,config,config_values,tolerance),
        trials=[dict(capture_id=win['capture_id'],cycles=len(win['cycles']),raw_events=len(win['raw']),
            complete=_window_complete(win),
            rejected_records=win['end'].get('rejected_records',0) if win['end'] else None)
            for win in windows])
    if include_predictions:result['predictions']=predictions
    return result


def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('capture','manifest','kf-config','output'):parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--build-dir',type=Path,default=ROOT/'build/speed-replay')
    parser.add_argument('--include-predictions',action='store_true')
    args=parser.parse_args(argv)
    try:
        data=args.capture.read_bytes();manifest=json.loads(args.manifest.read_text(encoding='utf-8-sig'))
        digest=hashlib.sha256(data).hexdigest()
        expected=manifest.get('capture_sha256')
        if expected and expected!=digest:raise ValueError('capture hash does not match manifest')
        rows=[json.loads(line) for line in data.decode('utf-8-sig').splitlines() if line.strip()]
        report=replay(load_library(args.build_dir),rows,manifest,
            json.loads(args.kf_config.read_text(encoding='utf-8-sig')),args.include_predictions)
        report.update(capture_sha256=digest,manifest_sha256=manifest['manifest_sha256'],
            source_commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
            source_dirty=bool(subprocess.check_output(['git','status','--porcelain'],cwd=ROOT,text=True).strip()))
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(json.dumps(report,indent=2,allow_nan=False)+'\n',encoding='utf-8')
        print(json.dumps({'output':str(args.output),'hardware_gate':report['hardware_gate']}))
        return 0
    except (ValueError,KeyError,TypeError,OSError,subprocess.CalledProcessError) as error:
        print(str(error),file=sys.stderr);return 2


if __name__=='__main__':sys.exit(main())
