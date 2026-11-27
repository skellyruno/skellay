"""Owned delay interpolation probe; private manifest and per-input report stay local."""
import argparse, ctypes as C, hashlib, json, pathlib, subprocess, tempfile
import numpy as np
import soundfile as sf
ROOT=pathlib.Path(__file__).resolve().parents[1]
def rms(x):return float(np.sqrt(np.mean(x*x)))
def db(x):return float(20*np.log10(max(float(x),1e-15)))
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--manifest',type=pathlib.Path,required=True);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
    with tempfile.TemporaryDirectory() as d:
        lib=pathlib.Path(d)/'delay.so';subprocess.run(['g++','-std=c++17','-O3','-Wall','-Wextra','-Werror','-shared','-fPIC',str(ROOT/'tests/research_delay_cubic_adapter.cpp'),'-o',str(lib)],check=True)
        f=C.CDLL(str(lib)).cubic_delay;ptr=np.ctypeslib.ndpointer(dtype=np.float32,ndim=2,flags='C_CONTIGUOUS');f.argtypes=[C.c_int,ptr,ptr,C.c_uint,C.c_uint,C.c_double,C.c_double,C.c_float,C.c_int];f.restype=C.c_int
        def render(k,x,fs,delay,fb,jump,tail=0):
            y=np.empty((len(x)+tail,2),np.float32)
            if f(k,x,y,len(x),tail,fs,delay,fb,jump)!=1 or not np.isfinite(y).all():raise RuntimeError('Render failed')
            return y
        tones=[];rows=[];seen=set()
        for fs in (44100,48000,96000):
            t=np.arange(fs)/fs
            for hz in (1000,4000,8000,12000,16000,20000):
                x=np.ascontiguousarray(np.column_stack((.1*np.sin(2*np.pi*hz*t),.1*np.cos(2*np.pi*hz*t))),dtype=np.float32)
                ys=[render(k,x,fs,32.5,0,0) for k in (0,1)]
                tones.append({'fs':fs,'hz':hz,'gain_db':[db(rms(y[fs//2:])/rms(x[fs//2:])) for y in ys]})
        for item in json.loads(a.manifest.read_text())['inputs']:
            src=pathlib.Path(item['path']);digest=hashlib.sha256(src.read_bytes()).hexdigest()
            if digest!=item['sha256'] or digest in seen:raise ValueError('Fingerprint mismatch or duplicate')
            seen.add(digest);x,fs=sf.read(src,dtype='float32',always_2d=True)
            if not len(x) or x.shape[1] not in (1,2) or not np.isfinite(x).all() or rms(x)<=1e-12:raise ValueError('Invalid input')
            if x.shape[1]==1:x=np.repeat(x,2,axis=1)
            x=np.ascontiguousarray(x);cases=[]
            for fb,jump in ((0,0),(.45,0),(.45,1)):
                ys=[render(k,x,fs,round(fs*.12)+.5,fb,jump,fs) for k in (0,1)]
                cases.append({'feedback':fb,'jump':jump,'gain_db':[db(rms(y)/rms(x)) for y in ys],'peak_dbfs':[db(np.max(np.abs(y))) for y in ys],'over_scale_samples':[int(np.count_nonzero(np.abs(y)>1)) for y in ys],'tail_rms':[rms(y[len(x):]) for y in ys],'difference_db':db(rms(ys[1]-ys[0])/max(rms(ys[0]),1e-15))})
            rows.append({'id':item['id'],'stage':item['stage'],'fs':fs,'cases':cases})
        a.output.write_text(json.dumps({'schema':1,'tone_rows':tones,'vocal_rows':rows,'limits':'Wet readout only; feedback stays linear. Sample peaks, no perceptual or host comparison.'},indent=2)+'\n')
if __name__=='__main__':main()
