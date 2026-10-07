#!/bin/sh
# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2026 SEN Labs e.U.
#
# Mac side helper for building SEN components in the Haiku VM (see the haiku-vm-workflow skill).
#
#   sen-vm.sh sync <repo>...        copy the working tree of the repos (sibling folders of sento) to /Develop/SEN/<repo> in the VM
#   sen-vm.sh build <repo> [args]   sync, then build detached in the VM; prints the log and "exit N"
#   sen-vm.sh run <command>         run a command in the VM
#
# Set SEN_VM_CCACHE=1 to build with ccache (the profile of the VM sets CC; only the makefile-engine with OBJ_DIR copes with it).
# SEN_VM_CLEAN="dir ..." empties these folders of the repo in the VM before copying (e.g. generated files).
# Settings: SEN_VM_PORT (2222), SEN_VM_USER (user), SEN_VM_HOST (localhost), SEN_VM_DIR (/Develop/SEN).
set -e
PORT=${SEN_VM_PORT:-2222}; USER=${SEN_VM_USER:-user}; HOST=${SEN_VM_HOST:-localhost}; VMDIR=${SEN_VM_DIR:-/Develop/SEN}
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
SSH="ssh -o ServerAliveInterval=5 -o ServerAliveCountMax=3 -p $PORT $USER@$HOST"

sync_repo() {
	repo=$1
	[ -d "$ROOT/$repo" ] || { echo "no such repo: $repo" >&2; exit 1; }
	# folders listed in SEN_VM_CLEAN (relative to the repo) are emptied first, so that files removed on the Mac go away in the VM, too
	for dir in $SEN_VM_CLEAN; do $SSH "rm -rf $VMDIR/$repo/$dir" </dev/null; done
	COPYFILE_DISABLE=1 tar -C "$ROOT/$repo" --exclude .git --exclude .DS_Store --exclude 'objects.*' --exclude bin --exclude '*.rsrc' -cf - . \
		| $SSH "mkdir -p $VMDIR/$repo && cd $VMDIR/$repo && tar -xf -" 2>&1 | grep -v "Ignoring unknown extended header" || true
}

case "$1" in
sync) shift; for r in "$@"; do sync_repo "$r"; done ;;
run) shift; $SSH "$*" </dev/null ;;
build)
	repo=$2; shift 2
	sync_repo "$repo"
	log=/tmp/sen-build-$repo.log
	$SSH "cat > /tmp/sen-build-$repo.sh" <<SCRIPT
#!/bin/sh
${SEN_VM_CCACHE:+. ~/config/settings/profile >/dev/null 2>\&1}
cd $VMDIR/$repo
# the VM clock can be behind: make all objects look old so that nothing is skipped
find objects.* -type f -exec touch -t 202001010000 {} + 2>/dev/null
$*
echo "exit \$?" >> $log
SCRIPT
	$SSH "chmod +x /tmp/sen-build-$repo.sh; rm -f $log" </dev/null
	ssh -f -o ServerAliveInterval=5 -p $PORT $USER@$HOST "/tmp/sen-build-$repo.sh > $log 2>&1 </dev/null &" </dev/null
	# wait by polling for the exit line
	i=0
	while [ $i -lt 240 ]; do
		sleep 5; i=$((i+1))
		if $SSH "grep -q '^exit ' $log 2>/dev/null" </dev/null; then break; fi
	done
	$SSH "grep -E 'error|Error|undefined|^exit |warning: unused' $log | head -60" </dev/null
	;;
*) sed -n 4,12p "$0"; exit 1 ;;
esac
