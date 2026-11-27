"""Private benchmark for the isolated owned reverse kernel, not a full host render."""
import argparse,ctypes as C,hashlib,json,pathlib,subprocess,tempfile,time,statistics as st
import numpy as np
import soundfile as sf
ROOT=pathlib.Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser();p.add_argument('--manifest',type=pathlib.Path,required=True);p.add_argument('--output',type=pathlib.Path,required=True);p.add_argument('--start',type=int,default=0);p.add_argument('--count',type=int,default=3);a=p.parse_args();rows=[]
 with tempfile.TemporaryDirectory() as d:
  lib=pathlib.Path(d)/'reverse.so';subprocess.run(['g++','-std=c++17','-O3','-shared','-fPIC','-Wall','-Wextra','-Werror',str(ROOT/'tests/reverse_probe_adapter.cpp'),'-o',str(lib)],check=True)
  f=C.CDLL(str(lib)).reverse_render;ptr=np.ctypeslib.ndpointer(dtype=np.float32,ndim=2,flags='C_CONTIGUOUS');f.argtypes=[C.c_int,ptr,ptr,C.c_uint,C.c_double,C.c_int];f.restype=C.c_int
  for item in json.loads(a.manifest.read_text())['inputs'][a.start:a.start+a.count]:
   assert hashlib.sha256(pathlib.Path(item['path']).read_bytes()).hexdigest()==item['sha256'];x,fs=sf.read(item['path'],dtype='float32',always_2d=True);x=np.ascontiguousarray(np.repeat(x,2,axis=1) if x.shape[1]==1 else x);base=np.empty_like(x);fast=np.empty_like(x);cases=[]
   for mode in range(3):
    ts=[[],[]]
    for rep in range(3):
     for k in ((0,1) if rep%2==0 else (1,0)):
      start=time.perf_counter_ns();assert f(k,x,fast if k else base,len(x),fs,mode);ts[k].append((time.perf_counter_ns()-start)/1e6)
     assert np.array_equal(base,fast) and np.isfinite(base).all()
    cases.append({'mode':mode,'exact':True,'ms_per_audio_second':[st.median(v)/(len(x)/fs) for v in ts]})
   rows.append({'id':item['id'],'cases':cases});print(item['id'],flush=True)
 a.output.write_text(json.dumps({'rows':rows,'limits':'Isolated owned Reverse kernel fixture; full plugin not tested.'},indent=2)+'\n')
if __name__=='__main__':main()
