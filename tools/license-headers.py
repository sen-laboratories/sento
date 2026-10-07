#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2026 SEN Labs e.U.
"""
license-headers: gives the source files of a repository the SPDX header of the SEN projects.

    license-headers.py [--check] <repo> [<repo> ...]

    /*
     * SPDX-License-Identifier: MIT
     * SPDX-FileCopyrightText: 2024-2026 SEN Labs e.U.
     */

The first year is the year of the first commit of the file (else of the old header), the last is the current one. An old header
(the "@author ... All Rights Reserved. Distributed under the terms of the MIT License" block) is replaced; other leading comments
are kept below the new header. Files that carry a license of their own (GPL, Apache, BSD, ...), those listed in the file
.license-headers-skip of the repository, and third party folders are left alone.
With --check nothing is written; the exit code says whether all files have a header (used by CI).
"""
import os
import re
import subprocess
import sys
from datetime import date

LAST_YEAR = date.today().year
OWNER = 'SEN Labs e.U.'
LICENSE = 'MIT'

SKIP_DIRS = ('/.git/', '/qpdf', '/3rdparty/', '/3rd-party/', '/objects.', '/bin/', '/generated/', '/node_modules/', '/.venv',
             '/images/', '/assets/', '/include/clang', '/LICENSES/', '/__pycache__/')
C_LIKE = ('.cpp', '.h', '.hpp', '.c', '.cc')
HASH_LIKE = ('.sh', '.py', '.yaml', '.yml', '.properties')
SLASH_LIKE = ('.rdef',)
THIRD_PARTY = re.compile(r'GNU General Public|GNU Lesser|Apache License|BSD|Mozilla Public|Copyright \(C\)|Copyright \(c\) \d{4} [A-Z]'
                         r'|Licensed under|Permission is hereby granted', re.I)
OLD_BLOCK = re.compile(r'\A(?:\s*\n)*/\*+(?P<body>.*?)\*/[ \t]*\n', re.S)


def first_commit_year(path, repo):
    try:
        out = subprocess.run(['git', '-C', repo, 'log', '--diff-filter=A', '--follow', '--format=%ad', '--date=format:%Y', '--',
                              os.path.relpath(path, repo)], capture_output=True, text=True, timeout=30).stdout.split()
        return int(out[-1]) if out else None
    except Exception:
        return None


def years(first):
    first = first or LAST_YEAR
    return str(first) if first >= LAST_YEAR else f'{first}-{LAST_YEAR}'


def spdx_lines(first):
    return [f'SPDX-License-Identifier: {LICENSE}', f'SPDX-FileCopyrightText: {years(first)} {OWNER}']


def c_header(first):
    return '/*\n' + ''.join(f' * {line}\n' for line in spdx_lines(first)) + ' */\n'


def prefixed_header(first, prefix):
    return ''.join(f'{prefix} {line}\n' for line in spdx_lines(first))


def old_years(text):
    m = re.search(r'(20\d\d)(?:\s*-\s*(20\d\d))?', text)
    return int(m.group(1)) if m else None


def is_old_header(body):
    """the block that the first SEN files had: author, rights and the MIT license, nothing else of interest"""
    return bool(re.search(r'All Rights Reserved|Distributed under the terms of the MIT|@author|Copyright 20\d\d', body)) \
        and not THIRD_PARTY.search(body.replace('Copyright 20', 'Copyright20'))


def skipped(path, repo):
    """paths listed in .license-headers-skip of the repository (one per line, relative, # for comments): files with a license of their own"""
    listing = os.path.join(repo, '.license-headers-skip')
    if not os.path.exists(listing):
        return False
    relative = os.path.relpath(path, repo)
    with open(listing) as f:
        return any(line.strip() == relative for line in f if line.strip() and not line.startswith('#'))


def process(path, repo, check):
    if skipped(path, repo):
        return 'foreign'
    with open(path, errors='surrogateescape') as f:
        text = f.read()
    head = text[:1500]
    if 'SPDX-License-Identifier' in head:
        return 'ok'
    if THIRD_PARTY.search(head) and not re.search(r'All Rights Reserved|Distributed under the terms of the MIT', head):
        return 'foreign'
    if check:
        return 'missing'

    name = os.path.basename(path)
    first = first_commit_year(path, repo)

    if name.endswith(C_LIKE):
        m = OLD_BLOCK.match(text)
        if m and is_old_header(m.group('body')):
            first = old_years(m.group('body')) or first
            text = c_header(first) + text[m.end():]
        else:
            text = c_header(first) + '\n' + text.lstrip('\n') if not text.startswith('/*') else c_header(first) + '\n' + text
    elif name.endswith(SLASH_LIKE):
        text = prefixed_header(first, '//') + '\n' + text.lstrip('\n')
    elif name.endswith(HASH_LIKE) or name == 'Makefile':
        header = prefixed_header(first, '#')
        if text.startswith('#!'):
            line_end = text.index('\n') + 1
            text = text[:line_end] + '#\n' + header + text[line_end:]
        else:
            text = header + '\n' + text.lstrip('\n')
    else:
        return 'skip'

    with open(path, 'w', errors='surrogateescape') as f:
        f.write(text)
    return 'fixed'


def main(argv):
    check = '--check' in argv
    repos = [a for a in argv if not a.startswith('--')]
    if not repos:
        print(__doc__)
        return 2
    counts = {}
    problems = 0
    for repo in repos:
        repo = os.path.abspath(repo)
        if os.path.isfile(repo):
            # a single file: the repository is the one that holds it
            root = subprocess.run(['git', '-C', os.path.dirname(repo), 'rev-parse', '--show-toplevel'], capture_output=True,
                                  text=True).stdout.strip() or os.path.dirname(repo)
            result = process(repo, root, check)
            counts[result] = counts.get(result, 0) + 1
            if result == 'missing':
                print(f'missing: {os.path.relpath(repo, root)}')
                problems += 1
            continue
        for dirpath, dirnames, filenames in os.walk(repo):
            if any(skip in dirpath + '/' for skip in SKIP_DIRS):
                continue
            for filename in sorted(filenames):
                if not filename.endswith(C_LIKE + HASH_LIKE + SLASH_LIKE) and filename != 'Makefile':
                    continue
                path = os.path.join(dirpath, filename)
                result = process(path, repo, check)
                counts[result] = counts.get(result, 0) + 1
                if result == 'missing':
                    print(f'missing: {os.path.relpath(path, repo)}')
                    problems += 1
                elif result == 'foreign':
                    print(f'own license, left alone: {os.path.relpath(path, repo)}')
    print(counts)
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
