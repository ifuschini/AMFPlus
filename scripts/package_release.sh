#!/bin/sh
# SPDX-License-Identifier: AGPL-3.0-or-later

set -eu

version="${1:-}"
if [ -z "$version" ]; then
    version=$(sed -n 's/^#define AMF_VERSION "\(.*\)"/\1/p' mod_amf.c | sed -n '1p')
fi

if [ -z "$version" ]; then
    echo "Unable to determine AMFPlus version" >&2
    exit 1
fi

output_dir="${OUT_DIR:-dist}"
archive="$output_dir/mod_amf.$version.tar.gz"
prefix="mod_amf.$version/"

mkdir -p "$output_dir"

package_files="mod_amf.c mod_amf.h mod_amf.tmpl install.sh README.md CHANGE CHANGELOG.md LICENSE rules tests images"

if tar --exclude ".DS_Store" --disable-copyfile -czf "$archive" -s ",^,$prefix," $package_files 2>/dev/null; then
    printf '%s\n' "$archive"
    exit 0
fi

COPYFILE_DISABLE=1 tar --exclude ".DS_Store" -czf "$archive" --transform "s,^,$prefix," $package_files
printf '%s\n' "$archive"
