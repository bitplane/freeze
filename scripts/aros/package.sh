#!/bin/sh
set -eu

target=${1:?usage: package.sh <target> <version>}
version=${2:?usage: package.sh <target> <version>}
case "$target" in i386-aros|aarch64-aros|x86_64-aros) ;; *) exit 1 ;; esac
case "$version" in ''|*[!0-9.]*) exit 1 ;; esac

name="freeze-$version-$target"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
mkdir -p "$work/freeze/C" "$work/freeze/Help/freeze" dist
cp freeze melt "$work/freeze/C/"
cp LICENSE freeze.1 "$work/freeze/Help/freeze/"
chmod 755 "$work/freeze/C/"*
chmod 644 "$work/freeze/Help/freeze/"*
epoch=${SOURCE_DATE_EPOCH:-$(git show -s --format=%ct HEAD)}
tar -C "$work" --sort=name --mtime="@$epoch" --owner=0 --group=0 \
    --numeric-owner -cjf "dist/$name.tar.bz2" freeze
(cd dist && sha256sum "$name.tar.bz2") > "dist/$name.tar.bz2.sha256"
