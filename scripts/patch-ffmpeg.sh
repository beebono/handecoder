#!/bin/bash

# Based on https://github.com/LibreELEC/LibreELEC.tv/blob/master/tools/ffmpeg/gen-patches.sh

FFMPEG_REPO="https://git.ffmpeg.org/ffmpeg.git"
FFMPEG_VERSION="n7.1"

FEATURE_SETS="v4l2-drmprime v4l2-request"

SCRIPT_DIR="$(dirname $(realpath $0))"
PROJECT_ROOT="$(realpath ${SCRIPT_DIR}/..)"
FFMPEG_DIR=$1

create_patch() {
  FEATURE_SET="$1"

  BASE_REPO="${FFMPEG_REPO}"
  BASE_VERSION="${FFMPEG_VERSION}"

  REPO="https://github.com/jernejsk/FFmpeg.git"
  REFSPEC="${FEATURE_SET}-${FFMPEG_VERSION}"

  cd "${FFMPEG_DIR}"
  git fetch "${BASE_REPO}" "${BASE_VERSION}"
  BASE_REV=$(git rev-parse FETCH_HEAD)

  PATCH_FILE="build/patches/${FEATURE_SET}.patch"
  mkdir -p "${PROJECT_ROOT}/build/patches"

  git fetch "${REPO}" "${REFSPEC}"
  REV=$(git rev-parse FETCH_HEAD)
  BASE_REV=$(git merge-base "${BASE_REV}" "${REV}")

  git format-patch --stdout --no-signature "${BASE_REV}..${REV}" >"${PROJECT_ROOT}/${PATCH_FILE}"
}

if [ -f "${FFMPEG_DIR}/ffmpeg-patched" ]; then
  echo "FFmpeg already patched, skipping."
  exit 0
fi

for SET in ${FEATURE_SETS}; do
  create_patch "${SET}"
done

cd "${FFMPEG_DIR}"
for PATCH in $(ls ${PROJECT_ROOT}/build/patches/*.patch 2>/dev/null); do
  git apply "${PATCH}"
  if [ $? -ne 0 ]; then
    echo "ERROR: Failed to apply patch ${PATCH}"
    echo "Please run ./scripts/unpatch-ffmpeg.sh and try building again."
    exit 1
  fi
done
touch ffmpeg-patched