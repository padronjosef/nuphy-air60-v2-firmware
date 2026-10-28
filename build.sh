#!/bin/sh
# Build the custom NuPhy Air60 V2 (ANSI) firmware from NuPhy's public QMK tree.
# Needs git and Docker; no ARM toolchain on the host.
#
#   ./build.sh            custom firmware (battery report + Fn+N for ñ)
#   ./build.sh --stock    unmodified firmware from the same upstream commit
set -eu

UPSTREAM=https://github.com/nuphy-src/qmk_firmware.git
BRANCH=nuphy-keyboards
COMMIT=f1856912d603800eaca227ae2e1c5c8548fdf261
KEYBOARD=nuphy/air60_v2/ansi:via

root="$(cd "$(dirname "$0")" && pwd)"
work="$root/.qmk"

if [ ! -d "$work/.git" ]; then
    git clone --branch "$BRANCH" --single-branch "$UPSTREAM" "$work"
fi
cd "$work"
git checkout --quiet --force "$COMMIT"
git clean --quiet -fd keyboards/nuphy/air60_v2
git submodule update --init --depth 1 --quiet

if [ "${1:-}" != "--stock" ]; then
    git apply "$root/air60_v2.patch"
    out=nuphy_air60_v2_ansi_via-custom.bin
else
    out=nuphy_air60_v2_ansi_via-stock.bin
fi

docker run --rm -e SKIP_GIT=1 -v "$work":/qmk_firmware -w /qmk_firmware ghcr.io/qmk/qmk_cli sh -c \
    "/opt/uv/tools/qmk/bin/python3 -m pip install -q -r requirements.txt && make $KEYBOARD && chown -R $(id -u):$(id -g) .build ./*.bin"

cp nuphy_air60_v2_ansi_via.bin "$root/firmware/$out"
sha256sum "$root/firmware/$out"
