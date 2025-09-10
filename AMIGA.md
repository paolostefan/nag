# Amiga-related Linux tools

## amitools

In a venv, do:

`pip install amitools`

Contains the `xdftools` command line tool, you can use it to extract files from an Amiga disk image. E.g.:

```bash
xdftools disk.adf unpack output_dir
```

Contains also the `hunktool` CLI tool, which depends on the `machine68k` Python package.

`pip install machine68k`

Usage example:

`hunktool -d info my_exe_file`

## binwalk

`sudo apt install binwalk`

A tool to find embedded files in a binary.

## unadf

`sudo apt install unadf`

Similar to `xdftools`, but it's a binary.

## xdms

`sudo apt install xdms`

A portable DMS archive extractor.
