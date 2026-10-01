import os
import tempfile
from pathlib import Path
from typing import Dict, Any, Optional


# Threshold: when to create in ram or writning to disk
DISK_THRESHOLD_KB = 50 * 1024


def generate_file_on_disk(file_path: Path, size_kb: int, chunk_size_mb: int = 8) -> None:
    total_bytes = size_kb * 1024
    chunk_bytes = chunk_size_mb * 1024 * 1024
    
    # Generating random block, then it's reused
    block = os.urandom(chunk_bytes)
    
    bytes_written = 0
    with open(file_path, "wb") as f:
        while bytes_written < total_bytes:
            remaining = total_bytes - bytes_written
            to_write = min(chunk_bytes, remaining)
            f.write(block[:to_write])
            bytes_written += to_write


def create_payload_info(
    test_case: Dict[str, Any], 
    sequence_num: int, 
    temp_dir: Optional[Path] = None
) -> Dict[str, Any]:
    size_kb = test_case.get("file_size_kb", 0)
    filename = f"test_{test_case.get('name', 'payload')}_{sequence_num}_{size_kb}KB.bin"
    
    # Case 1: Smaller files (< 50 MB) in RAM for maxium speed
    if size_kb < DISK_THRESHOLD_KB:
        data = os.urandom(size_kb * 1024)
        return {
            "filename": filename,
            "content_type": "application/octet-stream",
            "is_file_on_disk": False,
            "data": data,
            "size_bytes": len(data),
            "file_path": None
        }
    
    # Case 2: Bigger files (> 50 MB, i.e. 2.5 GB) writes to disk
    target_dir = temp_dir or Path(tempfile.gettempdir())
    file_path = target_dir / filename
    
    print(f"  [Disk] Generating large payload ({size_kb / 1024:.2f} MB) to {file_path}...")
    generate_file_on_disk(file_path, size_kb)
    
    return {
        "filename": filename,
        "content_type": "application/octet-stream",
        "is_file_on_disk": True,
        "data": None,  # No data in ram
        "size_bytes": size_kb * 1024,
        "file_path": file_path
    }