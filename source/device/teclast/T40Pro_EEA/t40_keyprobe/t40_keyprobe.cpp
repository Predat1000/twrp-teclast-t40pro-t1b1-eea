/*
 * T40 metadata-key read-only diagnostic.
 * Format/operation parameters audited against AOSP/TWRP Android 11
 * KeyStorage.cpp retrieveKey(), hashWithPrefix(), beginParams(),
 * decryptWithKeymasterKey() (Apache-2.0).
 * Calls only existing HAL getService/begin(DECRYPT)/update/finish/abort.
 * No generation, upgrade, deletion, HMAC agreement, DM, mounts or file writes.
 */
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
int main(int argc,char** argv) {
 setvbuf(stdout,nullptr,_IONBF,0);setvbuf(stderr,nullptr,_IONBF,0);
 struct rlimit lim{0,0};setrlimit(RLIMIT_CORE,&lim);prctl(PR_SET_DUMPABLE,0);alarm(20);
 if(argc==2 && !strcmp(argv[1],"--selfcheck")){puts("T40_KEYPROBE_V1 READ_ONLY NO_KEY_OUTPUT");return 0;}
 if(argc!=2 || strcmp(argv[1],"--read-existing-metadata-key") || getuid()!=0){fprintf(stderr,"Expected root and explicit --read-existing-metadata-key\n");return 2;}
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
 printf("KEY_RETRIEVE_PASS bytes=%zu no_key_output=1 no_mutations=1\n",plain.n);
 return 0;
}
