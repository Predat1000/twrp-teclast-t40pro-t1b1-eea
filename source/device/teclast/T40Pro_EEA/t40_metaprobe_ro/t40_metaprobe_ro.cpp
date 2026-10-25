/* T40 read-only metadata filesystem probe v2. No key updates or filesystem writes. Creates only a unique temporary dm-default-key READONLY mapping; reads both F2FS superblocks and removes its mapping. Refuses Android A, existing userdata mapper and existing probe mapper. */
#include <libdm/dm.h>
#include <libdm/dm_table.h>
#include <libdm/dm_target.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/system_properties.h>
#include <sys/sysmacros.h>
#include <android/hardware/keymaster/4.1/IKeymasterDevice.h>
#include <openssl/sha.h>
#include <array>
#include <vector>
#include <string>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/prctl.h>
namespace km = android::hardware::keymaster::V4_0;
using android::hardware::hidl_vec;
using Device = android::hardware::keymaster::V4_1::IKeymasterDevice;
static void wipe(void* p, size_t n) { volatile unsigned char* q=(volatile unsigned char*)p; while(n--) *q++=0; }
struct Secret { std::vector<uint8_t> b; ~Secret(){ if(!b.empty()) wipe(b.data(),b.size()); } };
struct Plain { std::array<uint8_t,128> b{}; size_t n=0; bool bad=false;
 ~Plain(){wipe(b.data(),b.size());}
 void take(const hidl_vec<uint8_t>& v) {if(v.size()>b.size()-n){bad=true;return;} if(v.size())memcpy(b.data()+n,v.data(),v.size());n+=v.size();}
};
static bool read_existing(const char* name,Secret& s) {
 const std::string path=std::string("/metadata/vold/metadata_encryption/key/")+name;
 int fd=open(path.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
 if(fd<0){fprintf(stderr,"READ_FAIL %s errno=%d\n",name,errno);return false;}
 struct stat st{}; if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_size<1||st.st_size>65536){close(fd);return false;}
 s.b.resize(st.st_size);size_t n=0;
 while(n<s.b.size()){ssize_t r=read(fd,s.b.data()+n,s.b.size()-n);if(r<0&&errno==EINTR)continue;if(r<=0){close(fd);return false;}n+=r;}
 close(fd);return true;
}
static hidl_vec<uint8_t> vec(const uint8_t* p,size_t n){hidl_vec<uint8_t> v;v.resize(n);if(n)memcpy(v.data(),p,n);return v;}
static bool recovery_b_guard() {
 char slot[PROP_VALUE_MAX]{},version[PROP_VALUE_MAX]{};
 __system_property_get("ro.boot.slot_suffix",slot);__system_property_get("ro.twrp.version",version);
 return !strcmp(slot,"_b") && !strncmp(version,"3.7.0",5);
}
static int probe_metadata_ro(const Plain& key) {
 using namespace android::dm;
 constexpr const char* name="t40_meta_ro_20261020";
 constexpr const char* source="/dev/t40_userdata";
 constexpr const char* node="/dev/t40_meta_ro";
 if(!recovery_b_guard() || key.n!=64){puts("RO_PROBE_GUARD_FAIL");return 20;}
 struct stat st{};
 if(lstat(node,&st)==0 || errno!=ENOENT){puts("RO_NODE_ALREADY_EXISTS");return 21;}
 int raw=open(source,O_RDONLY|O_CLOEXEC);
 uint64_t bytes=0;
 if(raw<0 || fstat(raw,&st) || !S_ISBLK(st.st_mode) || ioctl(raw,BLKGETSIZE64,&bytes) || bytes!=119727439360ULL){
  if(raw>=0)close(raw);puts("USERDATA_SOURCE_GUARD_FAIL");return 22;
 }
 close(raw);
 auto& dm=DeviceMapper::Instance();
 std::vector<DeviceMapper::DmBlockDevice> devices;
 if(!dm.GetAvailableDevices(&devices)){puts("DM_ENUM_FAIL");return 23;}
 for(const auto& d:devices) if(d.name()==name || d.name()=="userdata"){puts("EXISTING_DM_GUARD_FAIL");return 24;}
 std::string hex;hex.reserve(128);constexpr char digits[]="0123456789abcdef";
 for(size_t i=0;i<key.n;++i){hex.push_back(digits[key.b[i]>>4]);hex.push_back(digits[key.b[i]&15]);}
 DmTable table;table.set_readonly(true);
 auto target=std::make_unique<DmTargetDefaultKey>(0,(bytes/512)&~uint64_t(7),"aes-xts-plain64",hex,source,0);
 target->SetSetDun();
 if(!table.AddTarget(std::move(target))){wipe(hex.data(),hex.size());puts("DM_TABLE_INVALID");return 25;}
 wipe(hex.data(),hex.size());
 if(!dm.CreateDevice(name,table)){puts("DM_CREATE_RO_FAIL");return 26;}
 int fd=-1;bool made_node=false;
 auto cleanup=[&](int code){
  if(fd>=0){close(fd);fd=-1;}
  bool removed=dm.DeleteDevice(name);
  if(made_node)unlink(node);
  printf("DM_CLEANUP %s\n",removed?"PASS":"FAIL");
  return removed?code:99;
 };
 auto info=dm.GetDetailedInfo(name);
 if(!info || !info->IsReadOnly() || !info->IsActiveTablePresent()){puts("DM_NOT_READONLY_STOP");return cleanup(27);}
 dev_t number=0;if(!dm.GetDeviceNumber(name,&number)){puts("DM_NUMBER_FAIL");return cleanup(28);}
 if(mknod(node,S_IFBLK|0400,number)){puts("DM_NODE_CREATE_FAIL");return cleanup(29);}made_node=true;
 fd=open(node,O_RDONLY|O_CLOEXEC);
 int ro=0;
 if(fd<0 || ioctl(fd,BLKROGET,&ro) || ro!=1){puts("KERNEL_RO_GUARD_FAIL");return cleanup(30);}
 puts("DM_KERNEL_READONLY_CONFIRMED");
 for(off_t offset:{off_t(1024),off_t(5120)}){
  std::array<uint8_t,64> b{};
  if(pread(fd,b.data(),b.size(),offset)!=ssize_t(b.size())){puts("SUPERBLOCK_READ_FAIL");return cleanup(31);}
  uint32_t magic=0,log_blocksize=0;memcpy(&magic,b.data(),4);memcpy(&log_blocksize,b.data()+16,4);
  if(magic!=0xF2F52010U || log_blocksize!=12){printf("F2FS_SIGNATURE_FAIL copy=%d\n",offset==1024?1:2);return cleanup(32);}
 }
 puts("F2FS_SUPERBLOCK_PASS copies=2 block_size=4096 filesystem_not_mounted=1");
 return cleanup(0);
}

int main(int argc,char** argv) {
 setvbuf(stdout,nullptr,_IONBF,0);setvbuf(stderr,nullptr,_IONBF,0);
 struct rlimit lim{0,0};setrlimit(RLIMIT_CORE,&lim);prctl(PR_SET_DUMPABLE,0);alarm(20);
 if(argc==2 && !strcmp(argv[1],"--selfcheck")){puts("T40_METAPROBE_RO_V2 READ_ONLY NO_KEY_OUTPUT");return 0;}
 if(argc!=2 || strcmp(argv[1],"--read-only-f2fs-superblock") || getuid()!=0){fprintf(stderr,"Expected root and explicit --read-only-f2fs-superblock\n");return 2;}
 if(!recovery_b_guard()){puts("RECOVERY_B_ONLY");return 19;}
 Secret ver,stretch,sec,blob,cipher;
 if(!read_existing("version",ver)||!read_existing("stretching",stretch)||!read_existing("secdiscardable",sec)||!read_existing("keymaster_key_blob",blob)||!read_existing("encrypted_key",cipher))return 3;
 if(ver.b.size()!=1 || ver.b[0]!='1' || stretch.b.size()!=10 || memcmp(stretch.b.data(),"nopassword",10) || sec.b.size()!=16384 || cipher.b.size()<28 || cipher.b.size()>128){puts("UNSUPPORTED_KEY_FORMAT_READ_ONLY_STOP");return 4;}
 const uint8_t prefixBlob[8]={'p','K','M','b','l','o','b',0};
 size_t skip=blob.b.size()>=8 && !memcmp(blob.b.data(),prefixBlob,8)?8:0;
 std::array<uint8_t,SHA512_CBLOCK> prefix{};
 const char* p="Android secdiscardable SHA512";memcpy(prefix.data(),p,strlen(p));
 std::array<uint8_t,SHA512_DIGEST_LENGTH> appId{};
 SHA512_CTX c;SHA512_Init(&c);SHA512_Update(&c,prefix.data(),prefix.size());SHA512_Update(&c,sec.b.data(),sec.b.size());SHA512_Final(appId.data(),&c);wipe(&c,sizeof(c));
 puts("FORMAT_PASS");
 auto dev=Device::tryGetService("default");
 if(!dev){puts("HAL_4_1_NOT_AVAILABLE");wipe(appId.data(),appId.size());return 5;}
 puts("HAL_4_1_CONNECTED");
 hidl_vec<km::KeyParameter> params;params.resize(5);
 params[0].tag=km::Tag::BLOCK_MODE;params[0].f.blockMode=km::BlockMode::GCM;
 params[1].tag=km::Tag::PADDING;params[1].f.paddingMode=km::PaddingMode::NONE;
 params[2].tag=km::Tag::MAC_LENGTH;params[2].f.integer=128;
 params[3].tag=km::Tag::APPLICATION_ID;params[3].blob=vec(appId.data(),appId.size());wipe(appId.data(),appId.size());
 params[4].tag=km::Tag::NONCE;params[4].blob=vec(cipher.b.data(),12);
 auto keyblob=vec(blob.b.data()+skip,blob.b.size()-skip);
 km::ErrorCode err=km::ErrorCode::UNKNOWN_ERROR;uint64_t op=0;
 auto b=dev->begin(km::KeyPurpose::DECRYPT,keyblob,params,km::HardwareAuthToken{},
  [&](km::ErrorCode e,const hidl_vec<km::KeyParameter>&,uint64_t h){err=e;op=e==km::ErrorCode::OK?h:0;});
 if(!b.isOk()){puts("BEGIN_TRANSPORT_FAIL");return 6;}
 if(err!=km::ErrorCode::OK){printf("BEGIN_ERROR %d UPGRADE_DISABLED\n",int32_t(err));return 7;}
 puts("BEGIN_PASS");
 auto abort_op=[&](){if(op){auto a=dev->abort(op);if(!a.isOk())puts("ABORT_TRANSPORT_FAIL");op=0;}};
 Plain plain;size_t pos=12;
 while(pos<cipher.b.size()){
  uint32_t consumed=0;err=km::ErrorCode::UNKNOWN_ERROR;
  auto u=dev->update(op,hidl_vec<km::KeyParameter>{},vec(cipher.b.data()+pos,cipher.b.size()-pos),km::HardwareAuthToken{},km::VerificationToken{},
   [&](km::ErrorCode e,uint32_t used,const hidl_vec<km::KeyParameter>&,const hidl_vec<uint8_t>& out){err=e;consumed=used;if(e==km::ErrorCode::OK)plain.take(out);});
  if(!u.isOk()){puts("UPDATE_TRANSPORT_FAIL");abort_op();return 8;}
  if(err!=km::ErrorCode::OK || consumed==0 || consumed>cipher.b.size()-pos || plain.bad){printf("UPDATE_ERROR %d\n",int32_t(err));abort_op();return 9;}
  pos+=consumed;
 }
 err=km::ErrorCode::UNKNOWN_ERROR;
 auto f=dev->finish(op,hidl_vec<km::KeyParameter>{},hidl_vec<uint8_t>{},hidl_vec<uint8_t>{},km::HardwareAuthToken{},km::VerificationToken{},
  [&](km::ErrorCode e,const hidl_vec<km::KeyParameter>&,const hidl_vec<uint8_t>& out){err=e;if(e==km::ErrorCode::OK)plain.take(out);});
 op=0;
 if(!f.isOk()){puts("FINISH_TRANSPORT_FAIL");return 10;}
 if(err!=km::ErrorCode::OK || plain.bad){printf("FINISH_ERROR %d\n",int32_t(err));return 11;}
 if(plain.n!=32 && plain.n!=64){printf("UNEXPECTED_KEY_SIZE %zu\n",plain.n);return 12;}
 printf("KEY_RETRIEVE_PASS bytes=%zu no_key_output=1 key_mutations=0\n",plain.n);
 return probe_metadata_ro(plain);
}
