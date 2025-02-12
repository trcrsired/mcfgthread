#!/bin/bash -e

# setup
export CC=${CC:-"gcc"}
export CFLAGS='-O0 -g0'

# build
${CC} --version
mkdir -p m4
autoreconf -ifv

cd $(mktemp -d)
trap 'rm -rf ~+ || true' EXIT
~-/configure --disable-silent-rules --enable-debug-checks --{build,host}=${CONFIGURE_HOST} ${CONFIGURE_OPTS}

# test
if ! make -j$(nproc) distcheck DISTCHECK_CONFIGURE_FLAGS=${CONFIGURE_OPTS}
then
  find . -name 'test-suite.log' -exec cat '{}' ';'
  exit 1
fi
