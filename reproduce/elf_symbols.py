from elftools.elf.elffile import ELFFile
import io
def elf(data):
 f=ELFFile(io.BytesIO(data));assert (f.elfclass,f["e_machine"]) in [(64,"EM_AARCH64"),(32,"EM_ARM")]
 dyn=f.get_section_by_name(".dynamic");tags=list(dyn.iter_tags()) if dyn else []
 needed=[x.needed for x in tags if x.entry.d_tag=="DT_NEEDED"]
 soname=next((x.soname for x in tags if x.entry.d_tag=="DT_SONAME"),None)
 vr={};v=f.get_section_by_name(".gnu.version_r")
 if v:
  for ver,auxs in v.iter_versions():
   for a in auxs:vr[a["vna_other"]&0x7fff]={"name":a.name,"file":ver.name}
 vd={};v=f.get_section_by_name(".gnu.version_d")
 if v:
  for ver,auxs in v.iter_versions():vd[ver["vd_ndx"]&0x7fff]=next(iter(auxs)).name
 vs=f.get_section_by_name(".gnu.version");syms=f.get_section_by_name(".dynsym");undefined=[];exports=[]
 if syms:
  for i,x in enumerate(syms.iter_symbols()):
   if not x.name:continue
   idx=vs.get_symbol(i)["ndx"] if vs else 0;hidden=isinstance(idx,int) and bool(idx&0x8000);idx=idx&0x7fff if isinstance(idx,int) else 0
   if x["st_shndx"]=="SHN_UNDEF":
    if x["st_info"]["bind"]!="STB_WEAK":undefined.append({"symbol":x.name,**vr.get(idx,{})})
   elif x["st_info"]["bind"] in ("STB_GLOBAL","STB_WEAK") and x["st_other"]["visibility"] in ("STV_DEFAULT","STV_PROTECTED"):
    exports.append({"symbol":x.name,"version":vd.get(idx),"hidden":hidden})
 return {"needed":needed,"soname":soname,"undefined":undefined,"exports":exports}
