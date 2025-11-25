#!/usr/bin/env bash
# make_dist.sh - create <build>/dist with exe + required runtime DLLs (MSYS2 / MinGW64)
# Also creates a Windows .lnk shortcut in the repository root that points to the dist exe.
# Usage:
#   ./tools/make_dist.sh <path-to-exe>
#
# If no argument is given, it tries to use build/src/bs-rtype.exe as default.
set -euo pipefail

BUILD_DIR="build"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"

if [ $# -ge 1 ]; then
  EXE_PATH="$1"
else
  EXE_PATH="${BUILD_DIR}/src/bs-rtype.exe"
fi

if [ ! -f "${EXE_PATH}" ]; then
  echo "Error: executable not found at ${EXE_PATH}"
  echo "Please build first (example): cmake --build build --config Release --target bs-rtype"
  exit 1
fi

# Compute dist dir relative to the executable, create it, then canonicalize
DIST_DIR="$(dirname "${EXE_PATH}")/../dist"
mkdir -p "${DIST_DIR}"
DIST_DIR="$(cd "${DIST_DIR}" && pwd -P)"

cp -v "${EXE_PATH}" "${DIST_DIR}/"

# Use ldd to list dependencies
echo "Resolving runtime libraries via ldd..."
ldd "${EXE_PATH}" \
  | awk '/=>/ { if ($3 ~ /^\/.*/) print $3; else if ($1 ~ /^\/.*/) print $1 } /\\.dll$/ && $3=="" { if ($1 ~ /^\/.*/) print $1 }' \
  | sed '/^$/d' \
  | sort -u \
  > "${BUILD_DIR}/runtime_libs.txt"

if [ ! -s "${BUILD_DIR}/runtime_libs.txt" ]; then
  echo "Warning: no runtime libraries found by ldd (file empty)."
fi

# System DLL basenames we DO NOT copy into dist (lowercase)
readonly SYSTEM_DLL_PATTERNS=(
  ntdll.dll kernel32.dll kernelbase.dll user32.dll gdi32.dll opengl32.dll glu32.dll
  msvcrt.dll ucrtbase.dll winmm.dll shell32.dll advapi32.dll ole32.dll combase.dll msvcp_win.dll
  sechost.dll rpcrt4.dll win32u.dll wintypes.dll
)

echo "Copying runtime libraries (skipping system DLLs)..."
while IFS= read -r dllpath; do
  [ -z "$dllpath" ] && continue
  dllname="$(basename "$dllpath")"
  dllname_lc="$(echo "$dllname" | tr '[:upper:]' '[:lower:]')"

  skip=false
  for sys in "${SYSTEM_DLL_PATTERNS[@]}"; do
    if [ "$dllname_lc" = "$sys" ]; then
      echo "  Skipping system DLL: $dllname"
      skip=true
      break
    fi
  done
  if [ "$skip" = true ]; then
    continue
  fi

  if [ -f "$dllpath" ]; then
    cp -v --no-preserve=mode,ownership "$dllpath" "${DIST_DIR}/"
  else
    echo "  Warning: resolved library not found on disk: $dllpath"
  fi
done < "${BUILD_DIR}/runtime_libs.txt"

echo ""
echo "Dist folder created: ${DIST_DIR}"
echo "Contents:"
ls -1 "${DIST_DIR}"
echo ""

# Create Windows shortcut (.lnk) in repo root that points to the dist exe
DIST_EXE="${DIST_DIR}/bs-rtype.exe"
SHORTCUT_NAME="bs-rtype.lnk"
SHORTCUT_PATH="${REPO_ROOT}/${SHORTCUT_NAME}"

# Convert paths to Windows-style backslash paths for PowerShell using cygpath if available
if command -v cygpath >/dev/null 2>&1; then
  WIN_EXE_PATH="$(cygpath -w "${DIST_EXE}")"
  WIN_WORKING_DIR="$(cygpath -w "${DIST_DIR}")"
  WIN_SHORTCUT_PATH="$(cygpath -w "${SHORTCUT_PATH}")"
else
  WIN_EXE_PATH="${DIST_EXE}"
  WIN_WORKING_DIR="${DIST_DIR}"
  WIN_SHORTCUT_PATH="${SHORTCUT_PATH}"
fi

echo "Creating Windows shortcut at: ${SHORTCUT_PATH}"

# Create a temporary .ps1 script with the exact (Windows) paths inserted, then run it
TMP_PS="${BUILD_DIR}/create_shortcut.ps1"
cat > "${TMP_PS}" <<EOF
\$Wsh = New-Object -ComObject WScript.Shell
\$sc = \$Wsh.CreateShortcut('${WIN_SHORTCUT_PATH}')
\$sc.TargetPath = '${WIN_EXE_PATH}'
\$sc.WorkingDirectory = '${WIN_WORKING_DIR}'
\$sc.IconLocation = '${WIN_EXE_PATH},0'
\$sc.Save()
EOF

# Run the temporary script with PowerShell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "${TMP_PS}"

# Remove the temporary file
rm -f "${TMP_PS}"

echo "Shortcut created: ${SHORTCUT_PATH}"
echo ""
echo "Done. Double-click ${SHORTCUT_PATH} or ${DIST_EXE} in Explorer to run."