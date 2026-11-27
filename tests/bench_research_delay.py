"""Private owned real-vocal delay benchmark. Never commit input or row results."""
import argparse, ctypes as C, hashlib, json, pathlib, subprocess, tempfile, time
import numpy as np
import soundfile as sf
ROOT=pathlib.Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser();p.add_argument('--manifest',type=pathlib.Path,required=True);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
    rows=[];seen=set()
    with tempfile.TemporaryDirectory() as d:
        lib=pathlib.Path(d)/'delay.so'
        subprocess.run(['g++','-std=c++17','-O3','-Wall','-Wextra','-Werror','-shared','-fPIC',str(ROOT/'tests/research_delay_adapter.cpp'),'-o',str(lib)],check=True)
        f=C.CDLL(str(lib)).render_delay;ptr=np.ctypeslib.ndpointer(dtype=np.float32,ndim=2,flags='C_CONTIGUOUS');f.argtypes=[ptr,ptr,C.c_uint,C.c_uint,C.c_double,C.c_int];f.restype=C.c_int
        for item in json.loads(a.manifest.read_text())['inputs']:
            src=pathlib.Path(item['path']);sha=hashlib.sha256(src.read_bytes()).hexdigest()
            if sha!=item['sha256'] or sha in seen:raise ValueError('Fingerprint mismatch or duplicate')
            seen.add(sha);x,fs=sf.read(src,dtype='float32',always_2d=True)
            if not len(x) or x.shape[1] not in (1,2) or not np.isfinite(x).all():raise ValueError('Invalid input')
            if x.shape[1]==1:x=np.repeat(x,2,axis=1)
            x=np.ascontiguousarray(x);tail=fs;out=np.empty((len(x)+tail,2),np.float32);cases=[]
            for mode in range(3):
                t=time.perf_counter_ns();ok=f(x,out,len(x),tail,fs,mode);ms=(time.perf_counter_ns()-t)/1e6
                if ok!=1 or not np.isfinite(out).all():raise RuntimeError('Render failure')
                if mode==0 and (not np.array_equal(out[:len(x)],x) or np.any(out[len(x):])):raise RuntimeError('Dry identity failed')
                power=float(np.mean(out[len(x):]**2));peak=float(np.max(np.abs(out)))
                if mode and power<=0:raise RuntimeError('No tail')
                cases.append({'mode':mode,'peak':peak,'tail_rms':power**.5,'over_scale_samples':int(np.count_nonzero(np.abs(out)>1)),'ms_per_audio_second':ms/((len(x)+tail)/fs)})
            rows.append({'id':item['id'],'stage':item['stage'],'sha256':sha,'fs':fs,'frames':len(x),'cases':cases})
    a.output.write_text(json.dumps({'schema':1,'rows':rows,'limits':'Owned standalone kernel, not plugin host. Sample peak not true peak. Timing includes prepare and tail. No vendor or listening comparison.'},indent=2)+'\n')
if __name__=='__main__':main()
