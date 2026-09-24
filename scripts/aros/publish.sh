#!/bin/sh
set -eu

tag=${1:?usage: publish.sh <tag> <archive>...}
shift
[ "$#" -gt 0 ]
version=${tag#v}
case "$version" in ''|*[!0-9.]*) exit 1 ;; esac
: "${PKG_SIGNKEY:?Set PKG_SIGNKEY to your signing key file}"

pkg=${PKG:-pkg}
repo=${GITHUB_REPOSITORY:-bitplane/freeze}
channel_url=${PKG_CHANNEL_URL:-https://aros-pkg.azurewebsites.net/bitplane}
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM

for archive do
    name=$(basename "$archive")
    case "$name" in
        "freeze-$version-i386-aros.tar.bz2") arch=i386 ;;
        "freeze-$version-aarch64-aros.tar.bz2") arch=aarch64 ;;
        "freeze-$version-x86_64-aros.tar.bz2") arch=x86_64 ;;
        *) echo "Archive does not match release: $name" >&2; exit 1 ;;
    esac
    archive=$(CDPATH='' cd "$(dirname "$archive")" && pwd)/$name
    source="$archive!/freeze"
    "$pkg" MANIFEST "$source" KIND application NAME freeze VERSION "$version" > "$work/manifest"
    for field in "Name: freeze" "Version: $version" "Architecture: $arch"; do
        grep -Fxq "$field" "$work/manifest" || exit 1
    done
    "$pkg" PUBLISH "$source" CHANNEL "$work/channel" KIND application \
        NAME freeze VERSION "$version" \
        UPSTREAM "https://github.com/$repo/releases/download/$tag/$name" \
        SHORT 'Freeze and Melt compression utilities' \
        CATEGORY util/arc TAGS 'archive, compression' \
        AUTHOR 'Leonid A. Broukhis' \
        HOMEPAGE "https://github.com/$repo" REPOSITORY "https://github.com/$repo" \
        LICENSE MIT DISTRIBUTION other
done

"$pkg" PUSH CHANNEL "$work/channel" TO "$channel_url"
