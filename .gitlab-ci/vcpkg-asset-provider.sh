#!/usr/bin/env bash
set -euo pipefail

url="$1"
sha512="$2"
dst="$3"

package_registry="${CI_API_V4_URL}/projects/${CI_PROJECT_ID}/packages/generic/vcpkg-assets/$(date +%Y-%m)"
filename="$(basename "${url}")"
pkg_name="${filename}-${sha512}"
auth_header="JOB-TOKEN: ${CI_JOB_TOKEN}"

mkdir -p "$(dirname "${dst}")"

echo "checking asset cache for ${pkg_name}"
if curl --fail --silent --show-error \
    --header "${auth_header}" \
    --output "${dst}" \
    "${package_registry}/${pkg_name}"; then
  echo "asset cache hit: ${pkg_name}"
else
  echo "asset cache miss: ${pkg_name}, downloading ${url}"
  curl -L "${url}" --create-dirs --output "${dst}"

  echo "uploading to asset cache"
  curl --fail --silent --show-error \
    --header "${auth_header}" \
    --upload-file "${dst}" \
    "${package_registry}/${pkg_name}" || echo "warning: upload failed, continuing anyway"
fi

