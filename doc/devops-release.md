# Azure Devops Release

## Install Build Tools

```bash
pip install twine keyring artifacts-keyring
pip install --upgrade pip
```

We also need docker to build manylinux wheels. This document assumes you have a functional docker setup in your host.

## Setup `~/.pypirc`

Add this to your `$HOME/.pypirc` file.

```ini
[distutils]
Index-servers =
  dev

[dev]
Repository = https://pkgs.dev.azure.com/textmt/_packaging/dev/pypi/upload/
```


## Build Package
```bash
cd ./python  # subdir inside the repo
rm -rf build dist
bash python/make_py_wheel.sh
```

## Upload Package to DevOps

```bash
twine upload -r dev dist/*
```
> **Important**: always remember to provide the `-r dev` flag to avoid publishing your private packages to PyPI.


## Install Package from DevOps

```bash
# required for authentication with Devops
pip install keyring artifacts-keyring

INDEX=https://pkgs.dev.azure.com/textmt/_packaging/dev/pypi/simple/
pip install sentencepiece-ms -i $INDEX
```

> NOTE: It is possible to configure pip to include this private index by default, but we have not thoroghly tested that feature yet.


## Summary

We have renamed `sentencepiece` --> `sentencepiece-ms` to disambiguate the package, however, the module is still called `sentencepiece`.

Remember to `pip install sentencepiece-ms` but `import sentencepiece`.


