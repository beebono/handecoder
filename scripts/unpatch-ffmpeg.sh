#!/bin/bash

SCRIPT_DIR="$(dirname $0)"
PROJECT_ROOT="${SCRIPT_DIR}/.."
FFMPEG_DIR="${PROJECT_ROOT}/subprojects/ffmpeg"

if [ ! -f "${FFMPEG_DIR}/ffmpeg-patched" ]; then
  echo "No patches to revert."
  exit 0
fi

cd "${FFMPEG_DIR}"
git reset --hard
git clean -df