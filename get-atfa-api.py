#!/usr/bin/python3
# -*- coding: utf-8 -*-

"""
Universidade Federal do Rio de Janeiro
Escola Politécnica
Projeto Final de Graduação
Ambiente de Teste para Filtros Adaptativos
Pedro Angelo Medeiros Fonini <pedro.fonini@smt.ufrj.br>
Orientador: Markus Lima
"""

import sys
import pathlib
import urllib.request
from urllib.parse import urlparse

default_header_filename = 'atfa_api.h'
default_header_dir = pathlib.Path('src/')
default_url = 'https://github.com/fofoni/atfa-examples/raw/master/{}'.format(
    default_header_filename)

if __name__ == '__main__':

    ### Get URL input and filesystem output files from command line options

    import argparse

    parser = argparse.ArgumentParser(description='Download ATFA API header')
    parser.add_argument(
        "url",
        help="URL from which to download the header",
        nargs='?',
        default=default_url
    )
    parser.add_argument(
        "outfile",
        help="save the header in this location",
        nargs='?',
    )
    parser.add_argument(
        "-f", "--force",
        help="download even if file already exists",
        action='store_true',
    )
    args = parser.parse_args()

    if not args.url:
        raise ValueError("URL must be non-empty")

    # deduce header filename from URL
    url_path = urlparse(args.url).path
    if not url_path.endswith('/'):
        default_header_filename = pathlib.PurePosixPath(url_path).name

    # deduce final header filename and location
    if args.outfile is None:
        # user didn't provide a path
        header_dir = default_header_dir.resolve()
        header_filename = default_header_filename
    else:
        # user did provide a path
        outfile_path = pathlib.Path(args.outfile)
        if outfile_path.is_dir():
            header_dir = outfile_path.resolve()
            header_filename = default_header_filename
        else:
            header_dir = outfile_path.parent.resolve()
            header_filename = outfile_path.name
    header_path = header_dir / header_filename

    ### Check if output file already exists

    if not args.force and header_path.exists():
        # if target file exists but is empty, silently override it
        if header_path.stat().st_size != 0:
            try:
                header_path = header_path.relative_to(pathlib.Path.cwd())
            except ValueError: pass
            print(("Nonempty file ‘{}’ already exists: skipping download. Use"
                   " ‘-f’ to download and override.").format(header_path))
            sys.exit(0)

    # ignore `--force' when it would override a directory
    if header_path.is_dir():
        raise ValueError("OUTFILE is ‘{}’, which is a"
                            " directory".format(header_path))

    ### Download header file

    with urllib.request.urlopen(args.url) as resp:
        if resp.getcode() != 200:
            raise RuntimeError("Could not download ‘{}’ from ‘{}’".format(
                header_filename, args.url))
        b = resp.read()

    ### Write downloaded data to output file

    with header_path.open('wb') as f:
        f.write(b)

    ### End

    try:
        header_path = header_path.relative_to(pathlib.Path.cwd())
    except ValueError: pass
    print("Successfully downloaded ATFA API header file ‘{}’".format(
        header_path))
