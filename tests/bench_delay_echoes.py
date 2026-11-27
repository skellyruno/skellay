"""Private vocal echo/trim benchmark. No recording identities or rows in Git."""
import argparse, ctypes as C, hashlib, json, pathlib, subprocess, tempfile
import numpy as np
import soundfile as sf
ROOT=pathlib.Path(__file__).resolve().parents[1]
def db(x):return float(20*np.log10(max(float(x),1e-15)))
def band(x,fs,lo,hi):
    f=np.fft.rfftfreq(len(x),1/fs);a=np.fft.rfft(x,axis=0);return float(np.sqrt(np.sum(np.abs(a[(f>=lo)&(f<hi)])**2)))
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--manifest',type=pathlib.Path,required=True);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
    with tempfile.TemporaryDirectory() as d:
        lib=pathlib.Path(d)/'echo.so';subprocess.run(['g++','-std=c++17','-O3','-Wall','-Wextra','-Werror','-shared','-fPIC',str(ROOT/'tests/research_delay_echo_adapter.cpp'),'-o',str(lib)],check=True)
        f=C.CDLL(str(lib)).echo_render;ptr=np.ctypeslib.ndpointer(dtype=np.float32,ndim=2,flags='C_CONTIGUOUS');f.argtypes=[ptr,ptr,C.c_uint,C.c_uint,C.c_double,C.c_double,C.c_float,C.c_float,C.c_float,C.c_int,C.c_float,C.c_int];f.restype=C.c_int
        def render(x,fs,delay,fb,wet,cross,cubic,trim=0,jump=0,total=None):
            y=np.empty((total or len(x)+fs,2),np.float32)
            if f(x,y,len(x),len(y),fs,delay,fb,wet,cross,cubic,trim,jump)!=1 or not np.isfinite(y).all():raise RuntimeError('Render failed')
            return y
        rows=[];seen=set();tone=[]
        # Isolated impulse echoes: measure frequency-domain decay of each repeat.
        for fs in (44100,48000,96000):
            D=fs//4;x=np.zeros((64,2),np.float32);x[0]=[.25,-.25]
            for cross in (0,.25,.5,1):
                y=render(x,fs,D,.75,1,cross,0,total=6*D+64)
                expected=np.array([.25,-.25])
                for repeat in range(1,7):
                    if not np.allclose(y[repeat*D],expected,rtol=0,atol=1e-7):raise RuntimeError('Side eigenmode mismatch')
                    expected*=.75*(1-2*cross)
            for cubic,fraction in ((0,0),(0,.5),(1,.5)):
                y=render(x,fs,D+fraction,.75,1,0,cubic,total=6*D+64)
                for hz in (1000,4000,8000,12000,16000):
                    phase=np.exp(-2j*np.pi*hz*np.arange(64)/fs);amps=[]
                    for i in range(1,7):
                        z=y[i*D-16:i*D+48,0];amps.append(float(abs(np.dot(z,phase))))
                    expected=db(.75)+(0 if fraction==0 else db(abs(np.cos(np.pi*hz/fs))))
                    actual=db(amps[5]/amps[0])/5
                    if abs(actual-expected)>1e-3:raise RuntimeError('Echo decay oracle mismatch')
                    tone.append({'fs':fs,'hz':hz,'cubic':cubic,'fraction':fraction,'per_repeat_db':actual,'expected_db':expected})
        for item in json.loads(a.manifest.read_text())['inputs']:
            src=pathlib.Path(item['path']);digest=hashlib.sha256(src.read_bytes()).hexdigest()
            if digest!=item['sha256'] or digest in seen:raise ValueError('Fingerprint mismatch or duplicate')
            seen.add(digest);x,fs=sf.read(src,dtype='float32',always_2d=True)
            if not len(x) or x.shape[1] not in (1,2) or not np.isfinite(x).all():raise ValueError('Invalid audio')
            if x.shape[1]==1:x=np.repeat(x,2,axis=1)
            x=np.ascontiguousarray(x);D=fs//4;L=min(len(x),fs//8);starts=range(0,len(x)-L+1,max(1,L//2));start=max(starts,key=lambda i:np.sum(x[i:i+L]**2));clip=np.ascontiguousarray(x[start:start+L]);echo=[];trims=[]
            for cubic,fraction in ((0,0),(0,.5),(1,.5)):
                y=render(clip,fs,D+fraction,.75,1,0,cubic,total=6*D+L+32)
                e=[y[i*D-16:i*D+L+16] for i in range(1,7)]
                vals=[]
                for lo,hi in ((250,1000),(4000,8000),(8000,12000)):
                    b=[band(z,fs,lo,hi) for z in e];vals.append(db(b[5]/max(b[0],1e-15))/5)
                echo.append({'cubic':cubic,'fraction':fraction,'per_repeat_db_bands':vals})
            for cubic in (0,1):
                for jump in (0,1):
                    for wet in (.5,1):
                        y=render(x,fs,round(fs*.12)+.5,.45,wet,.35,cubic,jump=jump);z=render(x,fs,round(fs*.12)+.5,.45,wet,.35,cubic,-3,jump)
                        dry=np.pad(x,((0,fs),(0,0)))*(1-np.float32(wet));g=np.float32(10**(-3/20));err=float(np.max(np.abs(z-(dry+(y-dry)*g))))
                        if err>3e-7:raise RuntimeError('Wet-only gain mismatch')
                        trims.append({'cubic':cubic,'jump':jump,'wet':wet,'before_peak_dbfs':db(np.max(np.abs(y))),'after_peak_dbfs':db(np.max(np.abs(z))),'before_over':int(np.count_nonzero(np.abs(y)>1)),'after_over':int(np.count_nonzero(np.abs(z)>1)),'max_error':err})
            rows.append({'id':item['id'],'stage':item['stage'],'fs':fs,'echo':echo,'trim':trims})
        a.output.write_text(json.dumps({'schema':1,'tone_oracle':tone,'rows':rows,'limits':'Fixed-delay isolated repeats and wet-trim tests on owned vocal excerpts; sample peaks, not true peak; no host or listening claims.'},indent=2)+'\n')
if __name__=='__main__':main()
