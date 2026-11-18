#!/bin/sh
cd "$(dirname "$0")/.." || exit 1
make -f tools/make_tools.mk
