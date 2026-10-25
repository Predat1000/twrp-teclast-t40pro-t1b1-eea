import sys, struct, gzip, hashlib, stat
from pathlib import Path

def al(x,a): return (x+a-1)//a*a

def parse_cpio(raw):
    out=[]; pos=0
    while pos+110<=len(raw):
        h=raw[pos:pos+110]
        if h[:6] not in (b'070701',b'070702'): raise ValueError(f'bad cpio magic at {pos}')
        vals=[int(h[6+i*8:14+i*8],16) for i in range(13)]
        pos+=110
        ns=vals[11]; name=raw[pos:pos+ns-1].decode('utf-8','surrogateescape')
        pos+=ns; pos=al(pos,4)
        size=vals[6]; data=raw[pos:pos+size]; pos+=size; pos=al(pos,4)
        ent={'magic':h[:6],'ino':vals[0],'mode':vals[1],'uid':vals[2],'gid':vals[3],
             'nlink':vals[4],'mtime':vals[5],'data':data,'devmajor':vals[7],'devminor':vals[8],
             'rdevmajor':vals[9],'rdevminor':vals[10],'check':vals[12],'name':name}
        if name=='TRAILER!!!': break
        out.append(ent)
    return out

def pack_cpio(entries):
    b=bytearray()
    for e in entries:
        name=e['name'].encode('utf-8','surrogateescape')+b'\0'; data=e['data']
        vals=[e['ino'],e['mode'],e['uid'],e['gid'],e['nlink'],e['mtime'],len(data),e['devmajor'],e['devminor'],e['rdevmajor'],e['rdevminor'],len(name),e.get('check',0)]
        b += e.get('magic',b'070701') + b''.join(f'{v:08x}'.encode() for v in vals)
        b += name; b += b'\0'*((-len(b))%4); b += data; b += b'\0'*((-len(b))%4)
    name=b'TRAILER!!!\0'; vals=[0,0,0,0,1,0,0,0,0,0,0,len(name),0]
    b += b'070701'+b''.join(f'{v:08x}'.encode() for v in vals)+name
    b += b'\0'*((-len(b))%4)
    return bytes(b)

def unpack_boot(path):
    b=Path(path).read_bytes()
    if b[:8]!=b'ANDROID!': raise ValueError('not Android boot image')
    ks,rs,ss=struct.unpack_from('<III',b,8)[0],struct.unpack_from('<I',b,16)[0],struct.unpack_from('<I',b,24)[0]
    ps=struct.unpack_from('<I',b,36)[0]; hv=struct.unpack_from('<I',b,40)[0]
    k_off=ps; r_off=al(k_off+ks,ps); s_off=al(r_off+rs,ps)
    recsz=struct.unpack_from('<I',b,1632)[0] if hv>=1 else 0
    recoff=struct.unpack_from('<Q',b,1636)[0] if hv>=1 else 0
    dtbsz=struct.unpack_from('<I',b,1648)[0] if hv>=2 else 0
    after=al(s_off+ss,ps)
    if recsz:
        rec_off=recoff if recoff else after
        after=al(rec_off+recsz,ps)
    else: rec_off=after
    dtb_off=after
    return {'bytes':b,'ps':ps,'hv':hv,'ks':ks,'rs':rs,'ss':ss,'k':b[k_off:k_off+ks],
            'r':b[r_off:r_off+rs],'s':b[s_off:s_off+ss],'recsz':recsz,
            'rec':b[rec_off:rec_off+recsz] if recsz else b'', 'dtbsz':dtbsz,
            'dtb':b[dtb_off:dtb_off+dtbsz] if dtbsz else b'', 'header':bytearray(b[:ps])}

def cpio_from_boot(path):
    x=unpack_boot(path); return x, parse_cpio(gzip.decompress(x['r']))

def main(stock, base, out):
    sx,se=cpio_from_boot(stock); bx,be=cpio_from_boot(base)
    sm={e['name']:e for e in se}; bm={e['name']:e for e in be}
    replace=['system/bin/init','system/bin/linker64','system/bin/adbd','system/bin/charger',
             'system/bin/ueventd','system/etc/ld.config.txt','system/etc/init/hw/init.rc']
    replace += [n for n in sm if n.startswith('system/lib64/') and stat.S_ISREG(sm[n]['mode'])]
    missing=[]
    for n in replace:
        if n not in sm: continue
        e=dict(sm[n]); e['data']=bytes(sm[n]['data'])
        if n=='system/etc/init/hw/init.rc':
            t=e['data'].decode()
            t=t.replace('setprop sys.usb.configfs 0','setprop sys.usb.configfs 1')
            t=t.replace('service charger /system/bin/charger\n    critical','service charger /system/bin/charger\n    disabled\n    critical')
            e['data']=t.encode()
        if n in bm:
            idx=next(i for i,q in enumerate(be) if q['name']==n); be[idx]=e; bm[n]=e
        else:
            missing.append(e); bm[n]=e
    # Keep TWRP's hardware entry point minimal: no stock modem/factory services yet.
    n='init.recovery.ums512_1h10.rc'
    if n in bm:
        idx=next(i for i,q in enumerate(be) if q['name']==n)
        e=dict(be[idx]); e['data']=b'# T40 Pro hybrid Android 11 diagnostic entry point.\nimport /init.t40diag.rc\n'; be[idx]=e
    be += missing
    cpio=pack_cpio(be); gz=gzip.compress(cpio,compresslevel=9,mtime=0)
    h=bx['header']; struct.pack_into('<I',h,16,len(gz)); struct.pack_into('<I',h,44,0x16000158)
    cmd=b'console=ttyS1,115200n8 buildvariant=user'; h[64:576]=b'\0'*512; h[64:64+len(cmd)]=cmd
    ps=bx['ps']; raw=bytearray(h)
    def add(data):
        raw.extend(data); raw.extend(b'\0'*((-len(raw))%ps))
    add(bx['k']); add(gz); add(bx['s'])
    if bx['recsz']: add(bx['rec'])
    if bx['dtbsz']: add(bx['dtb'])
    Path(out).write_bytes(raw)
    print('output',out); print('raw_size',len(raw)); print('ramdisk_compressed',len(gz),'ramdisk_uncompressed',len(cpio))
    print('sha256',hashlib.sha256(raw).hexdigest().upper())
    print('replaced',len(replace),'added_missing',len(missing))
    for n in ['system/bin/init','system/bin/linker64','system/bin/adbd','system/etc/init/hw/init.rc','init.recovery.ums512_1h10.rc','first_stage_ramdisk/fstab.ums512_1h10','system/etc/recovery.fstab']:
        e=next((x for x in be if x['name']==n),None); print(n, len(e['data']) if e else 'MISSING', oct(e['mode']) if e else '')

if __name__=='__main__': main(*sys.argv[1:4])
