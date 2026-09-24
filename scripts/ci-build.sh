#!/bin/sh
set -eu

target=${1:?usage: ci-build.sh <target>}
case "$target" in
    linux-x86_64|macos-arm64) cc=${CC:-cc}; exe= ;;
    windows-x86_64) cc=${CC:-gcc}; exe=.exe ;;
    i386-aros|aarch64-aros|x86_64-aros) cc=${CC:-aros-cc}; exe= ;;
    *) echo "Unknown target: $target" >&2; exit 1 ;;
esac

# Cross builds cannot run the executable probes in the original configure.
# These are the portable facilities used by the supported targets.
cat > config.h <<'EOF'
#define DIRENT 1
#define UTIME 1
#define HAVE_LONG_FILE_NAMES 1
EOF

"$cc" -O2 -I. -o "freeze$exe" \
    bitio.c debug.c decode.c default.c encode.c freeze.c huf.c lz.c
cp "freeze$exe" "melt$exe"

case "$target" in
    *-aros) exit 0 ;;
esac

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
awk 'BEGIN { for (i = 0; i < 200; i++) print "Freeze/Melt release test" }' > "$work/input"
printf '\r\n\000\032\377\n' >> "$work/input"
"./freeze$exe" -fc < "$work/input" > "$work/input.F"
"./melt$exe" -c < "$work/input.F" > "$work/output"
cmp "$work/input" "$work/output"

if [ "${GITHUB_REF_TYPE:-}" = tag ]; then
    version=${GITHUB_REF_NAME#v}
    case "$version" in ''|*[!0-9.]*) echo "Invalid version: $version" >&2; exit 1 ;; esac
    stage="freeze-$version-$target"
    mkdir -p "dist/$stage"
    cp "freeze$exe" "melt$exe" LICENSE freeze.1 "dist/$stage/"
    tar -C dist -czf "dist/$stage.tar.gz" "$stage"
fi
