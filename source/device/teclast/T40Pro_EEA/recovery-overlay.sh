#!/usr/bin/env bash
set -euo pipefail
dst="${1:?usage: recovery-overlay.sh <TARGET_RECOVERY_ROOT_OUT>}"
here="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
src="$here/recovery/root"
echo "T40Pro: overlaying minimal UMS512 hardware files into $dst"
cp -rf "$src"/. "$dst"/
if [[ -f "$dst/prop.default" ]]; then
  sed -i -E 's/^persist\.sys\.usb\.config=.*/persist.sys.usb.config=adb/' "$dst/prop.default"
  sed -i -E 's/^ro\.adb\.secure=.*/ro.adb.secure=0/' "$dst/prop.default" || true
  sed -i -E 's/^ro\.secure=.*/ro.secure=0/' "$dst/prop.default" || true
  sed -i -E 's/^ro\.debuggable=.*/ro.debuggable=1/' "$dst/prop.default" || true
  grep -q '^sys\.usb\.configfs=' "$dst/prop.default" || echo 'sys.usb.configfs=1' >> "$dst/prop.default"
  grep -q '^sys\.usb\.controller=' "$dst/prop.default" || echo 'sys.usb.controller=musb-hdrc.0.auto' >> "$dst/prop.default"
fi
(
  cd "$dst"
  touch ramdisk-files.txt ramdisk-files.sha256sum
  find . | sed 's#^\./##' | sed '/lib\/modules\//d' > ramdisk-files.txt
  find -type f | sed '/\.\/ramdisk-files\.sha256sum/d' | sed '/lib\/modules/d' | sed '/prop\.default/d' | xargs sha256sum > ramdisk-files.sha256sum
)
echo "T40Pro: minimal Android 11 overlay complete"
