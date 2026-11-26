<!--
SPDX-License-Identifier: MIT
SPDX-FileCopyrightText: 2026 SEN Labs e.U.
-->

# Analysis: the Haiku VM freezes, and hangs at "Asking other processes to quit"

Status: **open**, tracked on the side. Started 2026-11-26 after the VM hung on a reboot. Nothing here is a conclusion unless it says so. The log excerpts are from the guest
(`/var/log/previous_syslog` of the boot that hung, `/var/log/syslog.1` of 2026-11-23); the host side is UTM 5.0.9 on macOS.

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

## 6. Notes from the session that found this (not Haiku's fault, but they cost time)

- **`pkgman install <hpkg>` without `-y` loops at EOF**: with stdin from `/dev/null` (a script that only wants to print the plan) it prints `Continue? [yes/no]` again and again. It wrote **1.7 GB**
  into a log, filled the 8 GiB boot disk to 71 MB free, and the greps of that log kept the CPU busy. Never call `pkgman` without `-y` in a script, and keep a script's log to a limited size
  (`| head -c`, `| tail -n`).
- **`du` on the package states does not finish** on a loaded VM: measure with `ls -l | awk` in a background loop that writes a file. `administrative/state_*` held 2.6 GB (183 states): trimmed to
  the states since 2026-11-20 (+1.5 GB free).
- **`ssh` + `tar` pipelines must not have `</dev/null`**, background scripts are started with `ssh -f ... &` and their end is a `done` file; `nohup`-less loops of `sleep` are blocked on the Mac side.
- A kernel crash at shutdown leaves **no crash report**: the only trace is `previous_syslog` (the log of the boot before: read it first after a reboot, before anything rotates it).
