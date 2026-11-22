<!--
SPDX-License-Identifier: MIT
SPDX-FileCopyrightText: 2026 SEN Labs e.U.
-->

# Problems of Haiku found while building SEN

What did not behave as documented or as POSIX says, with a small program that shows it, what SEN does about it, and whether it was reported. Tested on Haiku R1/beta6 (x86_64,
hrev60299 nightlies) unless an entry says otherwise. Add new findings here as they turn up; a report to the Haiku bug tracker can be written from an entry.

| # | Where | Short | Reported |
|---|---|---|---|
| 1 | libroot, `mktime` | one second late for times before 1970 | no |
| 2 | libroot, time zones | `tm_zone`/`tzname` are zone names, an unknown `TZ` is silently GMT | no |
| 3 | FileTypes | `-type <mime>` is ignored when FileTypes already runs | no |
| 4 | Support kit, `BPropertyInfo` | `FindMatch` did not match entries with index specifiers | no |
| 5 | package daemon | a second revision of a package makes it wait for a dialog, silently | no |

## 1. `mktime` is one second late before 1970

`mktime` of a time before 1970-01-01 returns a value that is one second too large; `localtime_r` of that value shows `00:00:01`. From 1970 on it is right. The zone does not matter
(the VM runs in Vienna, `TZ` unset).

```c
#include <stdio.h>
#include <string.h>
#include <time.h>
int main(void) {
	int years[] = {1939, 1950, 1969, 1970, 1971, 2024};
	for (int i = 0; i < 6; i++) {
		struct tm p; memset(&p, 0, sizeof p);
		p.tm_year = years[i] - 1900; p.tm_mon = 4; p.tm_mday = 5; p.tm_isdst = -1;      // 5 May, 00:00:00
		time_t r = mktime(&p);
		struct tm q; localtime_r(&r, &q);
		printf("%d: %lld -> %04d-%02d-%02d %02d:%02d:%02d\n", years[i], (long long)r,
			q.tm_year + 1900, q.tm_mon + 1, q.tm_mday, q.tm_hour, q.tm_min, q.tm_sec);
	}
}
```

```
1939: -967597199 -> 1939-05-05 00:00:01      (should be -967597200 -> 00:00:00)
1950: -620441999 -> 1950-05-05 00:00:01
1969: -20825999  -> 1969-05-05 00:00:01
1970: 10710000   -> 1970-05-05 00:00:00      (right from here on)
```

Expected: `mktime` and `localtime_r` are inverse to each other; the first line should read `-967597200`.
Found with a birthday of 1939 in a calendar file: its start was stored as 00:00:01. **SEN:** the calendar extractor looks at the result with `localtime_r` and moves it by the
difference when that is less than a minute (`CheckedMktime` in `sensei/plugins/calendar-extractor/core/IcsCalendar.cpp`).

## 2. Time zone names

With `TZ` set to an IANA name (`Europe/Vienna`) the library finds the rules, but:

```
TZ=Europe/Vienna    tm_zone=[GMT+2] gmtoff=7200 isdst=1 tzname=[Europe/Vienna,GMT+2]     (POSIX: CEST, tzname = CET, CEST)
TZ=America/New_York tm_zone=[EDT]   gmtoff=-14400 isdst=1 tzname=[America/New_York,EDT]  (tzname[0] should be EST)
TZ=Not/AZone        tm_zone=[GMT]   gmtoff=0 isdst=0 tzname=[GMT,GMT]                    (no error: a typo is UTC)
```

- `tzname[0]` is the name that was set, not the abbreviation of the standard time; `tm_zone` is `GMT+2` for Vienna (a zone for which the data has no abbreviation) but `EDT` for New York.
- A name that is not a zone is not an error: the time is GMT, and the program cannot tell it from a real zone with offset 0 except by trying January and July.

**SEN:** zone names are checked by their offsets in January and July (`PosixZoneOffset` in the calendar extractor); a few names that are UTC all year are listed by name.

## 3. FileTypes: `-type` only works at the start

`FileTypes -type <mime>` selects the type in the list, but only when the process starts and opens its window (`FileTypes::MessageReceived`, `kMsgOpenTypesWindow`: `fArgvType` is applied once).
When FileTypes runs already, the argument arrives as `B_ARGV_RECEIVED`, the type is stored and nothing changes on the screen. A bare MIME type as argument is taken for a path and ignored. The
window understands `kMsgSelectNewType` with a string field `type`, but no message that selects a type that exists.
Expected: opening FileTypes again with `-type` shows that type (and brings the window to the front). **SEN:** the FileTypes of SEN (`sen_filetypes`, signature `application/x-vnd.sen-labs.FileTypes`) is asked by a message (`sen::cmd::kOpenMimeAttribute`) and selects the type also when it runs; the SEN server decides which of the two to ask (`kUiOpenType`, the setting *Use the FileTypes of SEN*), and Shoji's link on `META:TYPE` sends that message to the server (without a server it starts the one of Haiku with `-type`).

## 4. `BPropertyInfo::FindMatch` and index specifiers

Seen while writing the scripting suite of Toji: an entry of the property table that is declared with `B_INDEX_SPECIFIER` or `B_RANGE_SPECIFIER` was not matched by `FindMatch`, a
handler that matched by its own code worked. Not re-checked in this repository, no small program yet. **SEN:** a small matcher of its own.

## 5. Package daemon waits for a dialog

Two revisions of one package in `~/config/packages` (or a package that is installed from a repository in `/boot/system` as well) make the package daemon open a "Package changes" window that asks which
to keep, and nothing is activated until someone answers. The syslog says only `Volume::_PackagesEntryCreated`. Expected at least a line in the log; a test that copies packages into a VM looks as if
the package did not install. **SEN:** the test scripts delete the older revision first.
