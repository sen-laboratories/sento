<!--
SPDX-License-Identifier: MIT
SPDX-FileCopyrightText: 2026 SEN Labs e.U.
-->

# Analysis: the Haiku VM freezes, and hangs at "Asking other processes to quit"

Status: **solved for symptom B (the hang at restart), 2026-11-26**: the emulated Intel HD Audio controller (the `hda` driver) hangs the guest at shutdown; with `virtio-sound-pci` (unsupported by Haiku: effectively no sound card) the VM restarts every time (see "Resolution" below). Symptom A (the freezes while running) is not shown to have the same cause; watch it. Started 2026-11-26 after the VM hung on a reboot. Nothing below the resolution is a conclusion unless it says so. The log excerpts are from the guest
(`/var/log/previous_syslog` of the boot that hung, `/var/log/syslog.1` of 2026-11-23); the host side is UTM 5.0.9 on macOS.

## Resolution (2026-11-26): the sound card

Cause: **the `hda` driver of Haiku (hrev60455 and hrev60493) hangs the guest when the emulated Intel HD Audio controller (UTM "Sound card: Intel HDA", PCI 8086:293e) is shut down.** The log of every hung boot with a record ends at the teardown of its MSI vector (`msi_free_vectors ... from 35`, vector 35 = `hda`), the first one with a kernel page fault in an add-on right after.

How it was found, and what settled it:
1. The boot after a restart that hung always held the same last lines: `msi_free_vectors` / `free_io_interrupt_vectors` for vector 35. The log names the driver at boot (`hda: using MSI vector 35`), so the vector told the device without a serial console (this was the experiment "name the device at vector 35", section 5, item 2).
2. A restart without any package update before it hung as well (5c): not the update, not Akita, not the disk, not BFS.
3. The user powered the VM down with the Intel sound driver in place (a power-off from outside does not run the driver's teardown path), then changed the sound card in UTM to **`virtio-sound-pci`** (Haiku has no driver for it: the sound preferences list no output, so for the guest this is the same as having no sound card): since then **no hang on restart**, also after a Haiku package update (Akita) and a restart of Akita, i.e. the sequences that hung before.

What to do: in UTM, Edit > Devices > Sound, remove the sound device, or choose **virtio-sound-pci**, which Haiku does not support (no output in the sound preferences), so it amounts to the same: the VM has no sound, and needs none for builds and tests. Intel HDA stays unusable for restarts in this setup until a fix of Haiku's `hda` driver: a bug report is possible with this recipe (QEMU/UTM `intel-hda` + `hda-duplex`, Haiku hrev60493, `shutdown -r`, hang at "Asking other processes to quit", QEMU spinning on all vCPUs).

Not explained / still open:
- The freezes while running (symptom A) were not shown to come from `hda`; observe the VM for some days with the new sound card before calling them solved.
- The BFS lines (`inode ... already deleted`, `Could not find value in index`) and the query that lists a file more than once are a separate matter: they exist in logs of boots that did not hang at all (`syslog.1` of 2026-11-23), and the hang is gone with the BFS state unchanged, so they did not cause it. The duplicates are handled in the SEN server (keyed by `SEN:ID`).
- Hypotheses 1 to 3 below were written before this: hypothesis 1 (a driver crashes when a device is torn down) was right, with `hda` as the driver.

## 1. What is seen

Two symptoms that may or may not have one cause:

**A. The guest freezes while running** (since about 2026-11-10; reported by both of us, see also the entry on the lockups in the project notes).
- ssh: `Connection timed out during banner exchange`. The TCP port (forwarded by QEMU) still accepts the connection, `sshd` in the guest does not answer.
- UTM says the VM is `started`; the QEMU process on the Mac sits at 300 to 380% CPU (three to four virtual CPUs spinning), not idle.
- The display of the guest is frozen as well. A restart from outside (UTM) is the only way out.

**B. The guest hangs when it is restarted from inside** (2026-11-26, but "always", says the user): the shutdown sequence stops at the step **"Asking other processes to quit"**.
Seen on the screen by the user. After a power-off from outside the guest boots normally.

## 2. Evidence from the boot that hung (`previous_syslog`, Haiku hrev60455, debug level 2)

The log ends with this, after a `pkgman update` had activated hrev60478 packages (the running kernel stayed hrev60455: the new one only loads at the next boot):

```
bfs: inode at 2099648 is already deleted!
bfs: IndexIteratorGetEntry:199: Bad data
bfs: could not get inode 2099648 in index!
msi_free_vectors: freeing 1 vectors starting from 35
free_io_interrupt_vectors: freeing 1 vectors starting from 35
bfs: Could not find value in index "size"!
bfs: Could not find value in index "last_modified"!
vm_page_fault: vm_soft_fault returned error 'Bad address' on fault at 0xc, ip 0xffffffff803b1a2f, write 0, kernel, exec 0, thread 105
```

and, earlier in the same boot, 27 lines `bfs: Could not find value in index "BEOS:TYPE"!`.

What this says, and what it does not:
- The last line is a **kernel** page fault (`kernel`, not `user`) at address `0xc`: a NULL pointer plus 12, a field of a structure that is not there. It is the last thing in the log, so it is
  the likely end of the guest's life: after it the guest cannot shut down.
- It comes **right after `msi_free_vectors` / `free_io_interrupt_vectors`**: a device is being torn down. The vectors of the virtio devices are logged at boot
  (`virtio_pci: using MSI-X count 2 starting at 26`, `4 at 28` (the network, `virtio_net`), `3 at 32` (`virtio_gpu`)). One vector starting at 35 is the next
  device after the GPU; its driver is not named in the log. The NVMe disk logs only `nvme_disk: using MSI-X`.
- The instruction pointer `0xffffffff803b1a2f` is **not inside the kernel image**: the symbol table of `kernel_x86_64` of hrev60455 (taken from the old package that `pkgman` kept in
  `administrative/state_2026-11-26_09:08:46`) ends at `_end = 0xffffffff80236000`. So the fault is in a loaded kernel add-on (a driver, a file system or a bus manager). The log does not
  give the load addresses of add-ons, so the add-on is not known. This is the cheapest thing to find out next (see 5).
- `bfs: inode at ... is already deleted`, `IndexIteratorGetEntry: Bad data`, `Could not find value in index` say that **an index of a BFS volume has entries for files that are gone** (or lacks
  entries for files that are there). Which volume is not in the log (the boot volume has 2 KiB blocks and about 4.2 million of them, `/Develop` about 8.4 million; the numbers fit either).
- The log of 2026-11-23 (`syslog.1`) already has **115 such lines**: `inode at 3168140`, `3169511` and `3169512` is already deleted, 28 times each. This is **before** the disk was ever
  full, so the index problem is older than the event of 2026-11-26 and the two should not be tied together.

## 3. What was checked and what it showed

- `checkfs -c` (check only) on `/Develop` and on the boot volume, 2026-11-26, after the hang: **0 blocks not allocated, 0 blocks already set**, 10,003 (`/Develop`) and 14,365 (boot) blocks
  that "could be freed" (about 39 and 56 MB marked used that no file uses: harmless). No error lines. **The checker does not look at the contents of the indices**, so a clean result does not
  clear the stale index entries.
- The package state after the reboot is consistent: 280 packages in `/boot/system/packages`, 280 entries in `activated-packages`, the Haiku packages are hrev60478. Akita passes `abi-check.sh`
  against the new libraries.
- **The disk was full on 2026-11-26** (a runaway log of 1.7 GB: see the notes on `pkgman` below), and that is **not** the cause: the user saw the hang at reboot also before that. The
  `syslog.1` lines show the index problem on 2026-11-23.
- A query on the volume shows the **same file more than once**: `query -v /Develop 'SEN:TO == "*<id>*"'` listed `/Develop/test/pb6/paper.pdf` three times for two of the three ids that
  its `SEN:TO` contains (once for the third). After the reboot the same. It did not reproduce with a small file whose attribute was rewritten four times (with and without `rmattr`). This
  looks like **the same kind of fault as the stale index entries** (an index with entries that should not be there), but that is a guess. SEN works around it where it counts
  (`ResolveReverseRelations()` adds the sets of a file once, keyed by its `SEN:ID`).

## 4. Hypotheses, most likely first

1. **A driver crashes when a device is torn down** (symptom B, perhaps A as well): a NULL pointer in an add-on right after the interrupt vectors of one device are freed. Candidates: the
   device whose vector starts at 35 (not named in the log), `virtio_net`, `virtio_gpu`, `nvme_disk`. The earlier suspicion that the **network** triggers the freezes (symptom A after
   network-heavy runs) fits `virtio_net`, but nothing connects them yet.
2. **A BFS index with stale entries** makes a query or a shutdown sync fault in `bfs`. The messages are real, and a reproducible one exists (the duplicates). It would explain the BFS lines but
   not why the fault is outside the kernel image unless `bfs` itself is the add-on at `0xffffffff803b1a2f` (it may be: a file system is an add-on).
3. **The host side**: QEMU spinning at 300% could be a guest in a tight loop (a spinlock after the fault), which would be a symptom, not a cause. Not separable without the guest's console.

Excluded or unlikely: a full disk (the hang predates it), damaged blocks (`checkfs -c`), a process that blocks the quit (the guest dies in the kernel; it does not wait in userland: but the
screen text "Asking other processes to quit" says the last visible step, the crash may be a few lines after it).

## 5. What to do next (cheapest first)

1. **Find the add-on of the fault.** Add a serial port in UTM that writes to a file (or a pty), and set `serial_debug_output true` in the kernel settings (`~/config/settings/kernel/drivers`: check the exact file and name in the Haiku documentation before relying on it). The next hang
   then ends in the kernel debugger on the serial line: `bt` and `ls` of the add-on that holds the address (the KDL command `image` / `area` lists the load addresses). Without that, only guesses.
2. **Name the device at vector 35**: the PCI list in the syslog (`listdev` in the guest, or the `PCI:` lines at the start of the boot) against the order of the virtio devices.
3. **Try without the suspects**, one at a time, each for several days of use: the other UTM network mode (emulated e1000 instead of virtio), no sound device (`hda` is there), no virtio-gpu if
   the display can be another one.
4. **Make the index problem reproducible**: copy the volume (UTM disk image) and run `checkfs` without `-c` on a copy, and see whether the "already deleted" lines go away. Never on the
   original first.
5. **A newer Haiku**: the VM runs hrev60478 now (after 2026-11-26). Note whether the next restart hangs again. If it does not, a fix in hrev60455 to hrev60478 is possible (the log of the
   Haiku commits between them for `virtio`, `nvme`, `bfs`, `interrupts` is worth reading).
6. If it is still reproducible with a known recipe, an entry in `haiku-issues.md` (and a report, if the project decides to send one) can be written from it.

## 5a. Does Akita's restart logic interfere? (read 2026-11-26, from the code; not tested in the VM)

Checked in `senryu/src/kits/tracker` (`AkitaUpdateWatcher.h`, `Tracker.cpp`, `TrackerSen*.cpp`) and in the server (`sen-core/src/server/SenServer.cpp`).

- **What stock Tracker does:** nothing about its own package. After a `pkgman update` the running Tracker keeps its old files until the system restarts. Its `QuitRequested()` (unchanged in Akita, as far
  as the function reads: not diffed against upstream) saves the open windows into an attribute of the desktop and waits at most 5 seconds (50 x 100 ms) for copy threads. The watcher is the only
  addition of this kind.
- **What the watcher does:** it watches the two package folders (`watch_node`, `B_WATCH_DIRECTORY`) and reacts only to an entry `akita-*.hpkg` that is created, removed or moved. Then, for up to
  15 x 4 s, it compares the program file (`stat`) with the one it started from, and asks once with a **non-blocking** `BAlert::Go(BInvoker*)`. "Restart" runs
  `(sleep 4; Akita) &` through `system()` and quits. A Haiku update that does not touch an Akita package never starts it. Nothing of it runs in `QuitRequested()`.
- **Against it being the cause:** the freezes began on 2026-11-10; the watcher was added on **2026-11-18** (`09c1aac1d6`). And the syslog of the hung boot ends in the **kernel** (device teardown, page fault):
  userland, Akita included, had finished by then. The text "Asking other processes to quit" may be a stale frame: once `app_server` has quit, nothing repaints the screen, so the last text painted stays.
- **Not excluded:** an "Akita updated" alert left open during the update, or a click on "Restart" shortly before a shutdown (the helper would start Akita again 4 s later, in the middle of the shutdown). Nobody has
  said that this happened.
- **Hazards found on the way** (real, but not shown to be involved; each could keep Akita or the server from answering the quit in time):
  1. Three calls from Akita to the server wait **without a timeout** for the reply (`TrackerSen.cpp` lines 652, 792, 1004: create the class entity, ask the ref for an id, get the ROOT of a relation); others wait 100 s
     (`TrackerSenRelations.cpp` 953, 1001, 1771). All are started by the user, so they are unlikely at a shutdown, but a stopped server turns them into a blocked thread.
  2. `SenServer::QuitRequested()` calls `BDeskbar::RemoveItem` (a call that waits for the reply of the Deskbar, which is quitting at the same time).
  3. The server runs `sen_oni --update` through `system()` in its worker; if the installer waits for the server while the server quits, that process stays behind (the server itself does not wait for it).
- **How to settle it without guessing:** after the next hang read `previous_syslog` first (does it end in the kernel, as before?), and run an A/B: `akita disable` (stock Tracker) and the server stopped for a few
  restarts after a `pkgman update`, then with each back on. A restart in a healthy state that hangs the same way with both off clears all of SEN.

## 5b. The second hang (2026-11-26, after the next Haiku update): no kernel error in the log

The VM was restarted from inside after a second `pkgman update` (hrev60478 to hrev60493), hung again, and was powered off with `utmctl stop Haiku --kill` and started again.

- `previous_syslog` was **not rewritten** by this restart: it is still the log of the first hung boot (56,284 bytes, same content). The log of the second one is the **tail of `syslog`**
  (lines before the new boot loader's "Welcome to the Haiku boot loader!").
- That tail ends with the package daemon's activation of the hrev60493 packages (six `_PackagesEntryRemoved` / `_PackagesEntryCreated` lines each) and then, directly, the next boot (`options = 0`). **No `vm_page_fault`, no
  `bfs:` line, no shutdown message at all.** Either the guest froze before it wrote anything about the shutdown, or the log lines were lost with the power-off (the log is a file; a `--kill` throws away what the guest had
  not written out). So this hang does **not** show the kernel fault of the first one, and the page fault should not be taken as the one cause.
- Both hangs came **after a `pkgman update` that replaced the `haiku` packages** in the running system (the first: hrev60455 to 60478, the second: 60478 to 60493). The user says the restart "always" hangs, so the update is
  probably not required, but both recorded cases have it. A restart without a preceding update has not been recorded yet: that is the cheapest next experiment (restart the VM now, on hrev60493, with nothing updated).
- Because the second hang left no trace, the "stale frame" remark of 5a (the screen text may be old once `app_server` has quit) is only a possibility: the user read the step on the screen, and this time nothing in the log
  says that userland was finished.

## 5c. The third hang (2026-11-26, NO update before it): it ends right after the HD Audio driver is torn down

A plain `shutdown -r` on hrev60493 with nothing updated or installed before (only `pkgman refresh`) hung at "Asking other processes to quit" again. QEMU used about 314 % CPU (all four vCPUs spinning), ssh timed out in the banner exchange. So **a Haiku package update is not required** (5b's "both followed an update" was a coincidence of timing, or one trigger among several).

The log of the hung boot (the head of `syslog` after the forced restart; `previous_syslog` is not rewritten) has no BFS error and no page fault this time, and ends with:

```
KERN: msi_free_vectors: freeing 1 vectors starting from 35
KERN: free_io_interrupt_vectors: freeing 1 vectors starting from 35
```

Vector 35 belongs to the **`hda` driver** (`hda: using MSI vector 35`, the Intel ICH9 HD Audio controller 8086:293e that QEMU emulates, PCI 0:3.0, driver `/boot/system/add-ons/kernel/drivers/dev/audio/hmulti/hda`). The first hung boot (section 2) ended with the very same two lines and then the page fault at ip 0xffffffff803b1a2f (above the kernel image, so inside a loaded add-on). Two of the three hangs therefore end at the teardown of the audio driver; the second one left nothing in the log at all (forced kill, lines may be lost).

This is the first hypothesis with evidence in all recorded cases that does not depend on BFS or on Akita. It is not proven: no stack of the spinning CPUs was seen.

Cheapest tests, in this order:
1. Remove the sound device from the VM (UTM: Haiku > Edit > Devices > Sound > remove) and restart normally. If it comes back, the `hda` teardown is the cause; the VM needs no audio for building and testing.
2. If audio is wanted: change the emulated device in UTM (the Sound card type), or check whether Haiku's `hda` driver is newer or fixed in a later nightly (`hda` MSI/uninit path).
3. To see where it spins: a serial console / KDL capture of a hung boot (see section 5), now with `hda` as the first suspect.

## 5d. A hang after the fix (2026-11-26, evening): with `virtio-sound-pci`, after the screen froze

The sound card was `virtio-sound-pci` (the QEMU command line has no `intel-hda`), the guest had run about 7.5 hours, and the **screen froze while the guest was alive** (`ssh` answered, load 0.27, QEMU at 22 % CPU,
`app_server`, `input_server`, Deskbar and Akita running, no crash report, no KDL). Restarting `input_server` did not help. A `shutdown -r` over `ssh` ended the session, and then the guest hung: `ssh` timed out in
the banner exchange and QEMU spun at 285-310 %, the same signature as the `hda` hang. So the `hda` fix removes one cause of the hang at shutdown, **not necessarily all of them**.

What the log of that boot showed (head of `syslog` after the forced stop; `previous_syslog` was stale, from 14:25): no kernel panic, no page fault, no `msi_free_vectors`, but the **PS/2 mouse driver flapping**: four
times `ps2: hot plugin of input/mouse/ps2/intelli_0`, `device ... still active, republishing...`, `ps2_standard_mouse_disconnect`, `Mouse device exiting, General system error` (once `Bad semaphore ID`), `strange mouse data,
x/y overflow, trying resync`, `bad mouse data, trying resync`, then probe and publish again. The whole earlier uptime (`syslog.1`, 7,216 lines) has none of it, so it began suddenly, after 20:30. The VM's only pointer
device is the emulated PS/2 mouse (no USB tablet), so the frozen pointer and the loop of the driver are the first suspects for the freeze; whether the spin at shutdown is the same loop is not known.
`checkfs -c` of both volumes after the restart: 0 blocks not allocated, 0 already set, 10,003 / 14,370 blocks that could be freed (leaks, no damage); the installed packages were intact.

Next: see whether the flapping returns without a freeze (`grep -c "Mouse device exiting" /var/log/syslog`), try a USB tablet instead of the PS/2 pointer in UTM, and keep the log of the next hang.

## 6. Notes from the session that found this (not Haiku's fault, but they cost time)

- **`pkgman install <hpkg>` without `-y` loops at EOF**: with stdin from `/dev/null` (a script that only wants to print the plan) it prints `Continue? [yes/no]` again and again. It wrote **1.7 GB**
  into a log, filled the 8 GiB boot disk to 71 MB free, and the greps of that log kept the CPU busy. Never call `pkgman` without `-y` in a script, and keep a script's log to a limited size
  (`| head -c`, `| tail -n`).
- **`du` on the package states does not finish** on a loaded VM: measure with `ls -l | awk` in a background loop that writes a file. `administrative/state_*` held 2.6 GB (183 states): trimmed to
  the states since 2026-11-20 (+1.5 GB free).
- **`ssh` + `tar` pipelines must not have `</dev/null`**, background scripts are started with `ssh -f ... &` and their end is a `done` file; `nohup`-less loops of `sleep` are blocked on the Mac side.
- A kernel crash at shutdown leaves **no crash report**: the only trace is `previous_syslog` (the log of the boot before: read it first after a reboot, before anything rotates it).
