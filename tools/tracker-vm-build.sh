#!/bin/sh
# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2026 SEN Labs e.U.
#
# Mac side: builds the Tracker of SENryu (a fork of the Haiku tree) in the Haiku VM and prints the result.
#
#   tracker-vm-build.sh <senryu-worktree>      e.g. /Volumes/SenryuCS/senryu
#
# The tree must be checked out on a CASE-SENSITIVE volume (the Haiku sources have paths that differ only by case; on a
# case-insensitive macOS volume files overwrite each other and show up as changed): create one with
#   hdiutil create -type SPARSE -size 10g -fs "Case-sensitive APFS" -volname SenryuCS ~/Develop/SenryuCS
# The copy goes to /Develop/haiku-senryu in the VM (the big volume), so the next build is incremental.
# Needs in the VM: jam nasm bison flex gawk git mkisofs makeinfo zip unzip xorriso mtools python3.10 (pkgman install -y xorriso mtools).
#
# Settings: SEN_VM_PORT, SEN_VM_USER, SEN_VM_HOST as in sen-vm.sh; HAIKU_REVISION (default hrev60209: the VM has no git history).
set -e
TREE=${1:?usage: tracker-vm-build.sh <senryu-worktree>}
PORT=${SEN_VM_PORT:-2222}; USER=${SEN_VM_USER:-user}; HOST=${SEN_VM_HOST:-localhost}
REVISION=${HAIKU_REVISION:-hrev60209}
SSH="ssh -o ServerAliveInterval=5 -o ServerAliveCountMax=3 -p $PORT $USER@$HOST"

# the clock of the VM drifts (after a restart it can be hours off): files with a time in the future or the past confuse jam
$SSH 'Time --update' </dev/null 2>&1 | tail -1

echo "copying $TREE ..."
COPYFILE_DISABLE=1 tar -C "$TREE" --exclude .git --exclude .DS_Store -cf - . \
	| $SSH 'mkdir -p /Develop/haiku-senryu && cd /Develop/haiku-senryu && tar -xf - 2>/dev/null; echo copied' 2>&1 | tail -1

$SSH "cat > /tmp/tracker-build.sh" <<SCRIPT
#!/bin/sh
rm -f /tmp/tracker-build.done /tmp/tracker-build.log
# jam appends its host library folder to LIBRARY_PATH, which REPLACES the loader's defaults when it is empty (ssh): set them
export LIBRARY_PATH=%A/lib:/boot/home/config/non-packaged/lib:/boot/home/config/lib:/boot/system/non-packaged/lib:/boot/system/lib
mkdir -p ~/config/non-packaged/bin
[ -e ~/config/non-packaged/bin/python3 ] || ln -s /boot/system/bin/python3.10 ~/config/non-packaged/bin/python3
export PATH=\$HOME/config/non-packaged/bin:\$PATH
export HAIKU_REVISION=$REVISION
cd /Develop/haiku-senryu
[ -f generated/build/BuildConfig ] || ./configure --use-gcc-pipe > /tmp/tracker-configure.log 2>&1
cd src/kits/tracker
jam -q -j\${NUM_PROC:-4} Tracker > /tmp/tracker-build.log 2>&1
echo "jam exit \$?" >> /tmp/tracker-build.log
touch /tmp/tracker-build.done
SCRIPT
$SSH 'chmod +x /tmp/tracker-build.sh' </dev/null
ssh -f -o ServerAliveInterval=5 -p $PORT $USER@$HOST '/tmp/tracker-build.sh >/dev/null 2>&1 </dev/null &' </dev/null

echo "building (the first build takes about 25 minutes, later ones are incremental) ..."
i=0
while [ $i -lt 360 ]; do
	sleep 10; i=$((i+1))
	$SSH 'test -f /tmp/tracker-build.done' </dev/null 2>/dev/null && break
done
$SSH 'grep -E "^\.\.\.failed|error:|jam exit" /tmp/tracker-build.log | head -20; ls -la /Develop/haiku-senryu/generated/objects/haiku/x86_64/release/kits/tracker/libtracker.so' </dev/null
