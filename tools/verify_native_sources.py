"""Verify exact owner-authorized native DSP source bytes (no secrets or downloads)."""
from pathlib import Path
import hashlib
ROOT = Path(__file__).resolve().parents[1] / "Source"
FILES = {
    "NativeFlangus/Flangus.cpp": "ddc4c62c73778316fec062288391487cbf96b6b490df94342d75dba97c892359",
    "NativeFlangus/SkellFlangusAdapter.h": "080c20c7a54bd443a70735fc01f4f08b8bf4617a9ba4bcf48808f9e079aeed8d",
    "NativeFlangus/core/FlangusFastDist.h": "f307be8fb18320e2ad5751f42b5edcfe6f88f9cf68fde27f0444946b6e189874",
    "NativeChorus/ChorusFx.cpp": "781f27549fefb01eca6e2adf105f9fe187dabba4995b2ed4df7fbad180adea60",
    "NativeChorus/core/ChorusFx.h": "ff68b7e2f07b7ecb8030a2f00abd03408b07f0260d3ae06a4f530a34399bfb5b",
    "NativeChorus/core/Simd.h": "b24e516aaaab9b8114699be002031b5f8e80880f0f143319f8632360f493656b"
}
for name, digest in FILES.items():
    path = ROOT / name
    if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest() != digest:
        raise SystemExit("Native source absent or changed: " + name)
print("All six vendored native source SHA256 checks passed")
