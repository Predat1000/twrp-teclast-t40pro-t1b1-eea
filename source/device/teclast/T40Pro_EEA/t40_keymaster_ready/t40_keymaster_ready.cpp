#include <android/hardware/keymaster/4.1/IKeymasterDevice.h>
#include <unistd.h>
#include <cstdio>
#include <sys/resource.h>
int main(int argc, char**) {
    if (argc != 1) return 2;
    struct rlimit z{0,0}; setrlimit(RLIMIT_CORE,&z);
    alarm(8);
    using Device=android::hardware::keymaster::V4_1::IKeymasterDevice;
    for (int i=0;i<50;i++) {
        if (Device::tryGetService("default") != nullptr) {
            puts("T40_KEYMASTER_READY"); return 0;
        }
        usleep(100000);
    }
    fputs("T40_KEYMASTER_UNAVAILABLE\n",stderr);
    return 1;
}
