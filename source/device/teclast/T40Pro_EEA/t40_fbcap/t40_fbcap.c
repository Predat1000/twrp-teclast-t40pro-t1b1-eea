#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/system_properties.h>
#include <linux/fb.h>
int main(void) {
 char p[PROP_VALUE_MAX]={0};
 __system_property_get("ro.twrp.boot",p);if(strcmp(p,"1"))return 2;
 __system_property_get("ro.boot.slot_suffix",p);if(strcmp(p,"_b"))return 3;
 int fd=open("/dev/graphics/fb0",O_RDONLY|O_CLOEXEC);if(fd<0){perror("fbopen");return 4;}
 struct fb_var_screeninfo v={0};struct fb_fix_screeninfo f={0};
 if(ioctl(fd,FBIOGET_VSCREENINFO,&v)||ioctl(fd,FBIOGET_FSCREENINFO,&f)){perror("fbinfo");return 5;}
 uint64_t start=(uint64_t)v.yoffset*f.line_length,total=(uint64_t)v.yres*f.line_length;
 if(v.bits_per_pixel!=32 || !v.xres || !v.yres || v.xres+v.xoffset>v.xres_virtual ||
    v.yres+v.yoffset>v.yres_virtual || total>268435456 || start+total>f.smem_len)return 6;
 const char* path="/tmp/t40-framebuffer.raw";
 int out=open(path,O_CREAT|O_EXCL|O_WRONLY|O_CLOEXEC,0600);if(out<0){perror("output");return 7;}
 char buf[65536];uint64_t done=0;
 while(done<total) {
  size_t count=total-done>sizeof(buf)?sizeof(buf):(size_t)(total-done);
  ssize_t n=pread(fd,buf,count,(off_t)(start+done));if(n<=0){perror("fbread");return 8;}
  ssize_t wrote=0;while(wrote<n){ssize_t z=write(out,buf+wrote,n-wrote);if(z<=0)return 9;wrote+=z;}
  done+=n;
 }
 close(out);close(fd);
 printf("{\"width\":%u,\"height\":%u,\"virtual_width\":%u,\"virtual_height\":%u,"
        "\"xoffset\":%u,\"yoffset\":%u,\"bits_per_pixel\":%u,\"stride\":%u,"
        "\"red\":[%u,%u],\"green\":[%u,%u],\"blue\":[%u,%u],\"alpha\":[%u,%u],"
        "\"rotate\":%u,\"bytes\":%llu,\"path\":\"%s\"}\n",
        v.xres,v.yres,v.xres_virtual,v.yres_virtual,v.xoffset,v.yoffset,v.bits_per_pixel,f.line_length,
        v.red.offset,v.red.length,v.green.offset,v.green.length,v.blue.offset,v.blue.length,
        v.transp.offset,v.transp.length,v.rotate,(unsigned long long)done,path);
 return 0;
}
