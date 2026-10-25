#!/usr/bin/env python3
"""Export preserved-layout native outputs; never accesses ADB or device blocks."""
from pathlib import Path
import argparse,gzip,hashlib,io,json,shutil,tarfile
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    for flag in ('tree','inputs','helper','build-result','output'):
        ap.add_argument('--'+flag,type=Path,required=True)
    a=ap.parse_args()
    def digest(b):return hashlib.sha256(b).hexdigest()
    build=json.loads(a.build_result.read_text())
    if build.get('status')!='PASS':raise SystemExit('A successful real build result is required')
    f=a.tree.resolve()/'out-clean-v53/target/product/T40Pro_EEA'
    if not (f/'boot.img').is_file():raise SystemExit('Missing fresh output boot.img')
    drm=a.tree/'bootable/recovery/minuitwrp/graphics_drm.cpp'
    if digest(drm.read_bytes())!='49eae437dce63b3fdc69102e70932a3a5fe486cc1c3c2160b57448f48bb12315':
        raise SystemExit('Unexpected graphics_drm.cpp source')
    mapping=json.loads((a.inputs/'native-output-map.json').read_text())
    if len(mapping)!=120:raise SystemExit('Expected 120 native payloads')
    rows={};blobs={}
    for name,v in sorted(mapping.items()):
        candidates=[(rel,(f/rel).read_bytes()) for rel in v['fresh_output_candidates'] if (f/rel).is_file()]
        if not candidates or len({digest(data) for _,data in candidates})!=1:
            raise SystemExit('Missing or ambiguous fresh output: '+name)
        rel,data=candidates[0]
        if not data.startswith(b'\x7fELF'):raise SystemExit('Not ELF: '+name)
        rows[name]={'fresh_output':(f/rel).as_posix(),'sha256':digest(data),'size':len(data),
                    'v53b_reference_sha256':v['v53_reference_sha256'],
                    'byte_identical':digest(data)==v['v53_reference_sha256']}
        blobs['artifacts/'+name]=data
    helper=a.helper.read_bytes()
    if digest(helper)!='93ca5f53a0d737d140e473d0b6ca7765ae61609f19bf55ccdc692458a050219d':
        raise SystemExit('Unexpected rebuilt PID1 helper')
    capsule=a.inputs/'source-capsule-v53.tar.gz'
    capsule_sha=digest(capsule.read_bytes())
    if capsule_sha!='cb3dbab9441ba4cd773adffd1906b62b18e57e63f448b65bdee16a570e9bd4aa':
        raise SystemExit('Unexpected frozen source capsule')
    blobs['artifacts/init']=helper
    manifest={'status':'CLEAN_OUTPUT_EXPORTED_PENDING_ELF_AUDIT','output_dir':'out-clean-v53',
              'native_entry_count':len(rows),'native_identical_count':sum(v['byte_identical'] for v in rows.values()),
              'native_boot_image_sha256':digest((f/'boot.img').read_bytes()),
              'custom_init_sha256':digest(helper),'source_capsule_sha256':capsule_sha,'entries':rows}
    blobs['native-artifacts-manifest.json']=json.dumps(manifest,indent=2).encode()
    a.output.mkdir(parents=True,exist_ok=False)
    archive=a.output/'native-artifacts-v53.tar.gz'
    with archive.open('wb') as raw:
        with gzip.GzipFile(filename='',mode='wb',fileobj=raw,mtime=0) as gz:
            with tarfile.open(fileobj=gz,mode='w') as tf:
                for name,data in sorted(blobs.items()):
                    item=tarfile.TarInfo(name);item.size=len(data);item.mode=0o644;item.mtime=0
                    tf.addfile(item,io.BytesIO(data))
    transfer={'status':'EXPORTED','bytes':archive.stat().st_size,'sha256':digest(archive.read_bytes()),
              'identical':manifest['native_identical_count'],'count':len(rows)}
    (a.output/'native-transfer-manifest.json').write_text(json.dumps(transfer,indent=2)+'\n')
    shutil.copyfile(a.build_result,a.output/'pc-clean-build-result.json')
    print(json.dumps(transfer,indent=2))
if __name__=='__main__':main()
