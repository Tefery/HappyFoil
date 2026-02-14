#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IMAGE_NAME="${IMAGE_NAME:-happyfoil-builder:nro}"
DOCKERFILE_PATH="${ROOT_DIR}/docker/Dockerfile.nro"

if [[ ! -f "${ROOT_DIR}/include/Plutonium/Makefile" ]]; then
    echo "[error] Missing Plutonium submodule files."
    echo "[hint] Run: git submodule update --init --recursive include/Plutonium"
    exit 1
fi

CORES="$(getconf _NPROCESSORS_ONLN 2>/dev/null || nproc 2>/dev/null || echo 2)"
JOBS=$((CORES / 2))
if [[ "${JOBS}" -lt 1 ]]; then
    JOBS=1
fi

echo "Building Docker image: ${IMAGE_NAME}"
docker build -f "${DOCKERFILE_PATH}" -t "${IMAGE_NAME}" "${ROOT_DIR}"

echo "Building happyfoil.nro with -j${JOBS}..."
docker run --rm \
    --user "$(id -u):$(id -g)" \
    --volume "${ROOT_DIR}:/workspace" \
    --workdir /workspace \
    "${IMAGE_NAME}" \
    sh -lc "make -C include/Plutonium -f Makefile lib && make -j${JOBS} && mkdir -p dist && cp -f happyfoil.nro dist/happyfoil.nro"

echo "Done: ${ROOT_DIR}/dist/happyfoil.nro"
