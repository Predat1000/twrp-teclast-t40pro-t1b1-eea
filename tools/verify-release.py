#!/usr/bin/env python3
"""Verify downloaded CP10 assets without accessing a device."""
import argparse,hashlib,json,zipfile
from pathlib import Path,PurePosixPath
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('directory',type=Path);a=ap.parse_args()
for line in (a.directory/'SHA256SUMS').read_text().splitlines():
    expected,name=line.split(None,1);name=name.lstrip('*')
    if Path(name).name!=name:raise SystemExit('Unsafe checksum path')
    p=a.directory/name
    actual=hashlib.sha256(p.read_bytes()).hexdigest()
    if actual!=expected:raise SystemExit('Hash mismatch: '+name)
    print('SHA256 PASS',name)
image=a.directory/'TWRP-T40Pro-T1B1-EEA-CP10-V53B.img'
if image.stat().st_size!=67108864:raise SystemExit('Image size mismatch')
if hashlib.sha256(image.read_bytes()).hexdigest()!='b029e36391ca919feef66d608eb5964562433d1b205ee0ebf8ba8579a0af4451':
    raise SystemExit('Image is not accepted CP10')
with zipfile.ZipFile(a.directory/'CP10-reproduction-inputs.zip') as z:
    records=json.loads(z.read('INPUTS-SHA256.json'))
    names=z.namelist()
    if len(names)!=len(set(names)) or set(names)!=set(records)|{'INPUTS-SHA256.json'}:
        raise SystemExit('Unexpected or duplicate ZIP member')
    for name,v in records.items():
        p=PurePosixPath(name)
        if p.is_absolute() or '..' in p.parts:raise SystemExit('Unsafe ZIP path')
        b=z.read(name)
        if len(b)!=v['bytes'] or hashlib.sha256(b).hexdigest()!=v['sha256']:
            raise SystemExit('ZIP input mismatch: '+name)
print('PASS: accepted image and all reproduction input hashes')
