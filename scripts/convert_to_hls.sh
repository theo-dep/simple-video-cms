#!/bin/bash

INPUT_DIR="${1:?Usage: $0 <input_dir>}"
if [[ ! -d "$INPUT_DIR" ]]; then
    echo "ERROR: '$INPUT_DIR' is not a directory" >&2
    exit 1
fi

HLS_TIME=6

mapfile -t videos < <(find "$INPUT_DIR" -maxdepth 1 -type f | sort)
if [[ ${#videos[@]} -eq 0 ]]; then
    echo "No video files found in '$INPUT_DIR'" >&2
    exit 0
fi

for input in "${videos[@]}"; do
    filename="$(basename "$input")"
    out_dir="${INPUT_DIR}/hls_${filename}"

    # Skip if already converted
    if [[ -f "${out_dir}/${filename}.m3u8" ]]; then
        echo "${filename} (${out_dir} already exists)"
        continue
    fi

    mkdir -p "$out_dir"
    echo "${filename}"

    if ffmpeg \
        -i "$input" \
        -hls_time "$HLS_TIME" \
        -hls_list_size 0 \
        -hls_flags independent_segments \
        -hls_segment_type mpegts \
        -hls_segment_filename "${out_dir}/${filename}_%03d.ts" \
        -f hls \
        "${out_dir}/${filename}.m3u8" \
        -y \
        -loglevel error \
        2>&1 | sed 's/^/       /'; then

        segment_count=$(find "$out_dir" -name "*.ts" | wc -l)
        echo "${filename} => hls_${filename}/ (${segment_count} segments)"
    else
        echo "${filename} fail"
    fi
done
