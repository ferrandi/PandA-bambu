#!/usr/bin/env python3
"""Recreate the benchmark CSVs from a pinned, checksum-verified public mirror.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
Copyright (C) 2026 Politecnico di Milano
The downloaded data has its own attribution in README.md and LICENSE.phoebetronic.
"""
import argparse
import hashlib
import os
from pathlib import Path
import shutil
import tempfile
import urllib.request
import zipfile

REVISION = 'c3579c6434dc6ae12d6b65f2444a53918a0ab819'
HASHES = {
    'test': ('dbc6569a26a27d7d9903bb870fac6baaecac50d87c4e9e329288fba06ad8d43f',
             'c5adfcc9ce29069f8a988a6af1dcdc009bba975d2c98a52233661f79d880292b'),
    'train': ('f734a89b95d2e514456e251fd609ebf37a77ac8236b2a18e06b151c03507683d',
              '06e64b0bf65de89c522348319ce5f63f43fb1cb40e1ce3c2ff43c1d3b8a0e14c'),
}


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, required=True)
    parser.add_argument('--cache-dir', type=Path)
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    cache = args.cache_dir or args.output_dir / '.downloads'
    cache.mkdir(parents=True, exist_ok=True)
    header = ('label,' + ','.join(f'{r}x{c}' for r in range(1, 29) for c in range(1, 29)) + '\r\n').encode()
    for kind, (archive_hash, csv_hash) in HASHES.items():
        target = args.output_dir / f'mnist_{kind}.csv'
        if target.exists():
            if digest(target) != csv_hash:
                raise RuntimeError(f'Refusing to overwrite a different CSV: {target}')
            print(f'Already verified: {target}')
            continue
        archive = cache / f'mnist_{kind}.zip'
        if not archive.exists():
            url = f'https://raw.githubusercontent.com/phoebetronic/mnist/{REVISION}/mnist_{kind}.csv.zip'
            with tempfile.NamedTemporaryFile(dir=cache, delete=False) as temporary:
                temporary_path = Path(temporary.name)
                try:
                    with urllib.request.urlopen(url, timeout=60) as source:
                        shutil.copyfileobj(source, temporary)
                    temporary.close()
                    if digest(temporary_path) != archive_hash:
                        raise RuntimeError(f'Archive checksum mismatch: {url}')
                    os.replace(temporary_path, archive)
                finally:
                    temporary_path.unlink(missing_ok=True)
        if digest(archive) != archive_hash:
            raise RuntimeError(f'Cached archive checksum mismatch: {archive}')
        with tempfile.NamedTemporaryFile(dir=args.output_dir, delete=False) as temporary:
            temporary_path = Path(temporary.name)
            try:
                temporary.write(header)
                with zipfile.ZipFile(archive) as zipped:
                    with zipped.open(f'mnist_{kind}.csv') as rows:
                        for row in rows:
                            temporary.write(row.rstrip(b'\r\n') + b'\r\n')
                temporary.close()
                if digest(temporary_path) != csv_hash:
                    raise RuntimeError(f'Reconstructed CSV checksum mismatch: {kind}')
                os.replace(temporary_path, target)
            finally:
                temporary_path.unlink(missing_ok=True)
        print(f'Verified: {target}')
    license_path = Path(__file__).with_name('LICENSE.phoebetronic')
    destination = args.output_dir / license_path.name
    if destination.resolve() != license_path.resolve():
        shutil.copy2(license_path, destination)


if __name__ == '__main__':
    main()
