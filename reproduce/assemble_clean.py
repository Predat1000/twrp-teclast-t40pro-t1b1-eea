"""Assemble only; never connects to a tablet or flashes a partition."""
from pathlib import Path
import argparse,sys,json,hashlib,struct,gzip,tarfile,copy,subprocess,os,shutil,datetime
ap=argparse.ArgumentParser()
ap.add_argument('--inputs',type=Path,default=Path(__file__).resolve().parent)
ap.add_argument('--output',type=Path,required=True)
ap.add_argument('--key',type=Path,required=True)
ap.add_argument('--avbtool',type=Path,required=True)
args=ap.parse_args();D=args.inputs;O=args.output;O.mkdir(exist_ok=False,parents=True)
sha=lambda b:hashlib.sha256(b).hexdigest()
m=json.loads((D/'inputs-manifest.json').read_text())
for name,spec in m['files'].items():
 data=(D/name).read_bytes();assert len(data)==spec['bytes'] and sha(data)==spec['sha256'],name
sys.path.insert(0,str(D));from cpio_boot_utils import parse_cpio,pack_cpio,cpio_from_boot
from elf_symbols import elf
from elftools.elf.elffile import ELFFile
import io
architectures=json.loads((D/'native-architectures.json').read_text())
transfer=json.loads((D/'native-transfer-manifest.json').read_text());artifact=D/'native-artifacts-v53.tar.gz'
assert sha(artifact.read_bytes())==transfer['sha256'] and artifact.stat().st_size==transfer['bytes']
assert json.loads((D/'pc-clean-build-result.json').read_text())['status']=='PASS'
assert sha((D/'source-capsule-v53.tar.gz').read_bytes())=='cb3dbab9441ba4cd773adffd1906b62b18e57e63f448b65bdee16a570e9bd4aa'
es=parse_cpio((D/'ramdisk-template.cpio').read_bytes());f={e['name']:e for e in es}
mapping=json.loads((D/'native-output-map.json').read_text());specs=json.loads((D/'baseline-native-elf-spec.json').read_text())
assert len(mapping)==120 and set(mapping)==set(specs)
assert b'ro.t40.build=V53B-native-crypto-drm-guard' in f['prop.default']['data']
def signature(info):
 return sha(json.dumps({'needed':sorted(info['needed']),'soname':info['soname'],'exports':sorted(info['exports'],key=lambda x:json.dumps(x,sort_keys=True)),'undefined':sorted(info['undefined'],key=lambda x:json.dumps(x,sort_keys=True))},sort_keys=True).encode())
changed=[];interface_changes=[]
with tarfile.open(artifact,'r:gz') as tf:
 names=[x.name for x in tf.getmembers()]
 allowed={'artifacts/'+n for n in mapping}|{'artifacts/init','native-artifacts-manifest.json'}
 assert len(names)==len(allowed) and set(names)==allowed,'Unexpected or duplicate archive members'
 native=json.loads(tf.extractfile('native-artifacts-manifest.json').read())
 assert native['output_dir']=='out-clean-v53' and set(native['entries'])==set(mapping)
 for n in mapping:
  data=tf.extractfile('artifacts/'+n).read()
  assert data.startswith(b'\x7fELF') and not f[n]['data']
  v=native['entries'][n];assert sha(data)==v['sha256'] and len(data)==v['size']
  assert '/out-clean-v53/target/product/T40Pro_EEA/' in v['fresh_output']
  parsed=ELFFile(io.BytesIO(data));assert [parsed.elfclass,parsed["e_machine"]]==architectures[n],n
  info=elf(data)
  if signature(info)!=specs[n]['interface_sha256']:interface_changes.append(n)
  if sha(data)!=specs[n]['byte_sha256']:changed.append(n)
  f[n]['data']=data
 helper=tf.extractfile('artifacts/init').read()
 assert not f['init']['data'] and sha(helper)==m['custom_init_expected_sha256']
 f['init']['data']=helper
(O/'native-comparison.json').write_text(json.dumps({'native_count':120,'byte_identical_count':120-len(changed),'changed_bytes':changed,'interface_changes':interface_changes},indent=2))
assert not interface_changes,'ELF interface changed; inspect before creating an image'
init=bytearray((D/'stock-init-original.elf').read_bytes())
for p in m['patches']:
 off=int(p['offset'],16);assert struct.unpack_from('<I',init,off)[0]==int(p['before'],16)
 struct.pack_into('<I',init,off,int(p['after'],16))
assert sha(init)==m['patched_init_expected_sha256'] and not f['system/bin/init']['data']
f['system/bin/init']['data']=bytes(init)
base={n:e['data'] for n,e in f.items()};cache={}
def info(n):
 if n not in cache:cache[n]=elf(base[n])
 return cache[n]
loader=info('system/bin/linker64');audits=[]
for root in json.loads((D/'primary-audit-roots.json').read_text()):
 nodes={root};q=[root];edges={};missinglib=[]
 while q:
  n=q.pop();edges[n]=[]
  for dep in info(n)['needed']:
   k='system/lib64/'+dep
   if k not in base:missinglib.append({'from':n,'needed':dep});continue
   edges[n].append(k)
   if k not in nodes:nodes.add(k);q.append(k)
 missing=[];resolved=0
 assert sum(len(info(n)['exports']) for n in nodes)>1000
 for n in nodes:
  seen={n};q=[n]
  while q:
   for k in edges.get(q.pop(),[]):
    if k not in seen:seen.add(k);q.append(k)
  providers={}
  for prov in [info(k) for k in seen]+[loader]:
   for x in prov['exports']:providers.setdefault(x['symbol'],[]).append((prov['soname'],x))
  for u in info(n)['undefined']:
   matches=providers.get(u['symbol'],[])
   if 'name' in u:matches=[(sn,x) for sn,x in matches if x['version']==u['name'] and (sn==u['file'] or u['symbol'].startswith('__loader_'))]
   else:matches=[(sn,x) for sn,x in matches if not x['hidden']]
   if matches:resolved+=1
   else:missing.append({'from':n,**u})
 audits.append({'root':root,'nodes':len(nodes),'resolved':resolved,'missing_libraries':missinglib,'missing_symbols':missing})
audit={'status':'PASS' if all(not r['missing_libraries'] and not r['missing_symbols'] for r in audits) else 'UNRESOLVED','roots':audits,'all_120_native_interfaces_match_verified_baseline':True,'isolated_runtime':'Frozen stock namespace unchanged; four newly exported isolated native inputs have identical ELF interfaces to verified baseline'}
(O/'elf-audit.json').write_text(json.dumps(audit,indent=2));assert audit['status']=='PASS'
header=bytearray((D/'boot-header.bin').read_bytes());kernel=(D/'kernel.bin').read_bytes();dtb=(D/'dtb.bin').read_bytes()
second=(D/'second.bin').read_bytes();rec=(D/'recovery-dtbo.bin').read_bytes()
assert header[:8]==b'ANDROID!' and len(header)==2048 and struct.unpack_from('<I',header,40)[0]==2 and not second and not rec
cpio=pack_cpio(es);gz=gzip.compress(cpio,compresslevel=9,mtime=0)
struct.pack_into('<I',header,16,len(gz))
digest=hashlib.sha1()
for blob in [kernel,gz,second,rec,dtb]:digest.update(blob);digest.update(struct.pack('<I',len(blob)))
header[576:608]=digest.digest()+b'\0'*12;raw=bytearray(header)
for blob in [kernel,gz,second,dtb]:raw+=blob;raw+=b'\0'*((-len(raw))%2048)
assert len(raw)<67108864-65536
out=O/'TWRP-T40Pro-v53b-clean-rebuilt-signed.img';out.write_bytes(raw)
env=os.environ.copy()
gitopenssl=Path(r'C:\Program Files\Git\mingw64\bin')
if gitopenssl.exists():env['PATH']=str(gitopenssl)+os.pathsep+env['PATH']
avb=[sys.executable,str(args.avbtool)]
flags=['add_hash_footer','--image',str(out),'--partition_size','67108864','--partition_name','boot','--algorithm','SHA256_RSA4096','--key',str(args.key),'--salt','08d3dc58f63932bb6be29042feec98b29840bc106ece1cc61f3c0def4ae7e7c4','--prop','com.android.build.boot.fingerprint:Teclast/T40Pro_EEA/T40Pro_EEA:11/RP1A.201005.001/20210924:user/release-keys','--prop','com.android.build.boot.os_version:11']
r=subprocess.run(avb+flags,capture_output=True,env=env,timeout=45);assert r.returncode==0,r.stderr.decode(errors='replace')
V=O/'avb-verify';V.mkdir();shutil.copy2(out,V/'boot.img')
for name,flags in [('info',['info_image','--image',str(out)]),('verify',['verify_image','--image',str(V/'boot.img'),'--key',str(args.key)])]:
 r=subprocess.run(avb+flags,capture_output=True,env=env,timeout=45);s=(r.stdout+r.stderr).decode(errors='replace');(O/('avb-'+name+'.txt')).write_text(s);assert r.returncode==0,s
 if name=='info':assert 'ea410c1b46cdb2e40e526880ff383f083bd615d5' in s
x,got=cpio_from_boot(out)
assert got==es and x['k']==kernel and x['dtb']==dtb and out.stat().st_size==67108864
result={'timestamp':datetime.datetime.now().astimezone().isoformat(),'status':'CLEAN_ASSEMBLY_AVB_ELF_PASS_NOT_FLASHED','signed_path':str(out),'signed_bytes':out.stat().st_size,'signed_sha256':sha(out.read_bytes()),'verified_v53b_reference_sha256':m['base_signed_sha256'],'signed_image_byte_identical':sha(out.read_bytes())==m['base_signed_sha256'],'native_byte_identical_count':120-len(changed),'native_entries':120,'custom_init_source_rebuilt_exact':True,'stock_init_repatched_exact':True,'kernel_sha256':sha(kernel),'dtb_sha256':sha(dtb),'AVB':'PASS','ELF':'PASS','clean_output':'out-clean-v53','source_capsule_sha256':native['source_capsule_sha256']}
(O/'result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2),flush=True)
