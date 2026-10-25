#include <errno.h>
#include <linux/keyctl.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/system_properties.h>
int main(void) {
    char slot[PROP_VALUE_MAX]={0}, twrp[PROP_VALUE_MAX]={0};
    __system_property_get("ro.boot.slot_suffix", slot);
    __system_property_get("ro.t40.build", twrp);
    if (getuid()!=0 || strcmp(slot,"_b") || !strstr(twrp,"-native-crypto-")) {
        fputs("T40_KEYRING_REFUSED_CONTEXT\n",stderr); return 2;
    }
    long session=syscall(SYS_keyctl,KEYCTL_GET_KEYRING_ID,KEY_SPEC_SESSION_KEYRING,0);
    if(session<0) { perror("existing session keyring"); return 3; }
    long ring=syscall(SYS_keyctl,KEYCTL_SEARCH,KEY_SPEC_SESSION_KEYRING,"keyring","fscrypt",0);
    int created=0;
    if(ring<0 && errno==ENOKEY) {
        ring=syscall(SYS_add_key,"keyring","fscrypt",NULL,0,KEY_SPEC_SESSION_KEYRING);
        created=1;
    }
    if(ring<0) { perror("fscrypt keyring"); return 4; }
    long found=syscall(SYS_keyctl,KEYCTL_SEARCH,KEY_SPEC_SESSION_KEYRING,"keyring","fscrypt",0);
    if(found!=ring) { fputs("T40_KEYRING_VERIFY_FAILED\n",stderr); return 5; }
    printf("T40_KEYRING_READY session=%ld ring=%ld created=%d\n",session,ring,created);
    return 0;
}
