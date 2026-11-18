#!/bin/sh
cd "$(dirname "$0")/.." || exit 1
make clean -f tools/make_tools.mk
