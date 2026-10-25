typedef unsigned long U;
static long sc(long nr,long a,long b,long c,long d,long e,long f){
 register long x0 __asm__("x0")=a,x1 __asm__("x1")=b,x2 __asm__("x2")=c;
 register long x3 __asm__("x3")=d,x4 __asm__("x4")=e,x5 __asm__("x5")=f,x8 __asm__("x8")=nr;
 __asm__ volatile("svc #0":"+r"(x0):"r"(x1),"r"(x2),"r"(x3),"r"(x4),"r"(x5),"r"(x8):"memory");
 return x0;
}
static U len(const char*s){U n=0;while(s[n])n++;return n;}
static long op(const char*p,long fl,long mode){return sc(56,-100,(long)p,fl,mode,0,0);}
static void wr(long fd,const char*s){sc(64,fd,(long)s,len(s),0,0,0);}
static void closefd(long fd){sc(57,fd,0,0,0,0,0);}
static long kfd=-1, diskfd=-1;
static void msg(const char*s){char b[512];const char*p="<0>T40WM: ";int i=0;while(*p)b[i++]=*p++;while(*s&&i<510)b[i++]=*s++;b[i++]='\n';b[i]=0;wr(kfd,b);if(diskfd>=0)wr(diskfd,b);}
static void lines(const char*s,long n){char b[481];int j=0;for(long i=0;i<n;i++){char c=s[i];if(c=='\n'||j==480){b[j]=0;msg(b);j=0;if(c=='\n')continue;}b[j++]=c;}if(j){b[j]=0;msg(b);}}
static void number(const char*key,long v){char b[32];int i=30;b[31]=0;b[i--]='\n';U n=v<0?-v:v;do{b[i--]='0'+n%10;n/=10;}while(n);if(v<0)b[i--]='-';msg(key);lines(b+i+1,len(b+i+1));}
static void dump(const char*p){char b[4097];long fd=op(p,0,0);msg(p);if(fd<0){number("open errno",fd);return;}long n=sc(63,fd,(long)b,4096,0,0,0);if(n>0){b[n]=0;lines(b,n);}else number("read result",n);closefd(fd);}
static void sleepsec(long s){long ts[2]={s,0};sc(101,(long)ts,0,0,0,0,0);}
static void mkdir(const char*p){sc(34,-100,(long)p,0755,0,0,0);}
void cmain(long*sp){
 if(sc(172,0,0,0,0,0,0)!=1){
  wr(1,"T40WM static AArch64 helper selftest OK; no PID1 actions performed\n");
  sc(93,0,0,0,0,0,0);for(;;){}
 }
 sc(33,-100,(long)"/t40kmsg",0020600,267,0,0);
 kfd=op("/t40kmsg",1,0);
 sc(24,kfd,1,0,0,0,0);sc(24,kfd,2,0,0,0,0);
 msg("BUILD_V43 STAGE_00_STATIC_INIT_ENTERED");
 mkdir("/proc");
 number("proc mount result",sc(40,(long)"proc",(long)"/proc",(long)"proc",0,0,0));
 dump("/proc/cmdline");
 long fd=op("/proc/sys/kernel/printk",1,0);if(fd>=0){wr(fd,"8 4 1 7\n");closefd(fd);}
 msg("V43: no diagnostic reboot observer; init USB failsafe retained");
 char entropy[32];
 number("getrandom nonblock result",sc(278,(long)entropy,32,1,0,0,0));
 mkdir("/dev");
 number("mknod urandom result",sc(33,-100,(long)"/dev/urandom",0020666,265,0,0));
 long ur=op("/dev/urandom",0,0);
 number("urandom open",ur);
 if(ur>=0){number("urandom read",sc(63,ur,(long)entropy,32,0,0,0));closefd(ur);}
 number("proc unmount before stock init",sc(39,(long)"/proc",0,0,0,0,0));
 msg("STAGE_01_EXEC_STOCK_INIT");
 char *argv[]={"/system/bin/init",0};
 char *envp[]={"PATH=/system/bin","LD_DEBUG=1",0};
 long ret=sc(221,(long)argv[0],(long)argv,(long)envp,0,0,0);
 number("EXEC_INIT_FAILED",ret);
 for(;;)sleepsec(60);
}
__asm__(".global _start\n_start:\nmov x0, sp\nbl cmain\nb .\n");
