#!/usr/bin/env bash
set -euo pipefail

# ---- config (Western Digital Caviar 2200 emulation) -------------------
BOOT_BIN="boot.bin"
STAGE2_BIN="stage2.bin"
DISK_IMG="disk.img"

SECTOR_SIZE=512
DISK_SIZE_MB=200  # Updated to ~200MB class drive

# Caviar 2200 native geometry: 958 Cylinders, 4 Heads, 34 Sectors
GEO_HEADS=4
GEO_SPT=63

STAGE2_START_LBA=1        # LBA0=sector1(boot), LBA1=sector2 -> stage2 starts here
STAGE2_SECTOR_COUNT=63    # sectors 2-63 (1-indexed) = 62 sectors

PART_START_LBA=64         # sector 64 (1-indexed) - right after stage2
PART_TYPE=0x83            # Linux native filesystem (ext2)
# -----------------------------------------------------------------------

TOTAL_BYTES=$((DISK_SIZE_MB * 1024 * 1024))
TOTAL_SECTORS=$((TOTAL_BYTES / SECTOR_SIZE))
PART_SECTOR_COUNT=$((TOTAL_SECTORS - PART_START_LBA))
PART_BYTES=$((PART_SECTOR_COUNT * SECTOR_SIZE))

echo "Total sectors:        $TOTAL_SECTORS"
echo "Partition start LBA: $PART_START_LBA"
echo "Partition sectors:   $PART_SECTOR_COUNT ($((PART_BYTES / 1024 / 1024)) MB)"

# ---- sanity checks -----------------------------------------------------
boot_size=$(stat -c%s "$BOOT_BIN")
if (( boot_size > SECTOR_SIZE )); then
    echo "ERROR: $BOOT_BIN is $boot_size bytes, exceeds one sector ($SECTOR_SIZE)" >&2
    exit 1
fi

stage2_size=$(stat -c%s "$STAGE2_BIN")
stage2_max=$((STAGE2_SECTOR_COUNT * SECTOR_SIZE))
if (( stage2_size > stage2_max )); then
    echo "ERROR: $STAGE2_BIN is $stage2_size bytes, exceeds $STAGE2_SECTOR_COUNT sectors ($stage2_max bytes)" >&2
    exit 1
fi

# ---- 1. create blank disk image ---------------------------------------
echo "creating blank ${DISK_SIZE_MB}MB disk image..."
dd if=/dev/zero of="$DISK_IMG" bs=1M count="$DISK_SIZE_MB" status=none

# ---- 2. write boot.bin to sector 1 (LBA0) -----------------------------
echo "writing $BOOT_BIN to sector 1 (LBA0)..."
dd if="$BOOT_BIN" of="$DISK_IMG" bs=$SECTOR_SIZE seek=0 conv=notrunc status=none

# ---- 3. write stage2.bin to sectors 2-63 (LBA1-62) --------------------
echo "writing $STAGE2_BIN to sectors 2-63 (LBA${STAGE2_START_LBA}-$((STAGE2_START_LBA+STAGE2_SECTOR_COUNT-1)))..."
dd if="$STAGE2_BIN" of="$DISK_IMG" bs=$SECTOR_SIZE seek=$STAGE2_START_LBA conv=notrunc status=none

# ---- 4. build an empty ext2 filesystem image --------------------------
EXT2_IMG=$(mktemp)
trap 'rm -f "$EXT2_IMG"' EXIT

echo "creating empty ext2 filesystem ($((PART_BYTES / 1024 / 1024)) MB)..."
dd if=/dev/zero of="$EXT2_IMG" bs=$SECTOR_SIZE count="$PART_SECTOR_COUNT" status=none
mke2fs -F -q -t ext2 -b 1024 -I 128 -O^extent,^64bit,^flex_bg,^dir_index,^metadata_csum,^huge_file,^extra_isize "$EXT2_IMG"

echo "writing ext2 filesystem into disk image at LBA $PART_START_LBA..."
dd if="$EXT2_IMG" of="$DISK_IMG" bs=$SECTOR_SIZE seek="$PART_START_LBA" conv=notrunc status=none

# ---- 5. write MBR partition table entry, WITHOUT touching boot code ---
echo "writing MBR partition table entry (offset 446, 16 bytes only)..."

python3 - "$DISK_IMG" "$PART_TYPE" "$PART_START_LBA" "$PART_SECTOR_COUNT" "$GEO_HEADS" "$GEO_SPT" <<'EOF'
import sys, struct

disk_img   = sys.argv[1]
part_type  = int(sys.argv[2], 0)
start_lba  = int(sys.argv[3])
sector_cnt = int(sys.argv[4])
heads      = int(sys.argv[5])
spt        = int(sys.argv[6])

def lba_to_chs(lba, heads, spt):
    c = lba // (heads * spt)
    h = (lba // spt) % heads
    s = (lba % spt) + 1
    sector_byte   = (s & 0x3F) | ((c >> 2) & 0xC0)
    cylinder_byte = c & 0xFF
    return h, sector_byte, cylinder_byte

end_lba = start_lba + sector_cnt - 1

start_h, start_s, start_c = lba_to_chs(start_lba, heads, spt)
end_h,   end_s,   end_c   = lba_to_chs(end_lba, heads, spt)

entry = struct.pack(
    "<BBBBBBBBII",
    0x80,        # boot indicator: active
    start_h, start_s, start_c,
    part_type,
    end_h, end_s, end_c,
    start_lba,
    sector_cnt,
)

assert len(entry) == 16

with open(disk_img, "r+b") as f:
    f.seek(446)         
    f.write(entry)        

print(f"  type=0x{part_type:02x} start_lba={start_lba} sectors={sector_cnt} (Geometry: Heads={heads}, SPT={spt})")
EOF

echo "done. Caviar 2200 emulated disk.img is ready."
