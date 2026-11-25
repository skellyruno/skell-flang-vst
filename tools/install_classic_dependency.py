"""Install the exact owner-supplied Classic Chorus source for the one normal build."""
import argparse
import base64
import hashlib
import io
import os
from pathlib import Path
import zipfile

FILES = {
    "ChorusFx.cpp": "781f27549fefb01eca6e2adf105f9fe187dabba4995b2ed4df7fbad180adea60",
    "core/ChorusFx.h": "ff68b7e2f07b7ecb8030a2f00abd03408b07f0260d3ae06a4f530a34399bfb5b",
    "core/Simd.h": "b24e516aaaab9b8114699be002031b5f8e80880f0f143319f8632360f493656b",
}
def validate(data):
    for name, digest in FILES.items():
        if hashlib.sha256(data[name]).hexdigest() != digest:
            raise ValueError("Native source SHA256 mismatch: " + name)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--required", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1] / "Source" / "NativeChorus"
    if any((root / name).exists() for name in FILES):
        if not all((root / name).is_file() for name in FILES):
            raise SystemExit("Incomplete native Classic Chorus dependency")
        validate({name: (root / name).read_bytes() for name in FILES})
        print("Native Classic Chorus source SHA256 checks passed")
        return
    encoded = os.environ.get("CHORUS_SOURCE_B64", "").strip()
    if not encoded:
        if args.required:
            raise SystemExit("Native Classic Chorus required: set SKELL_CHORUS_SOURCE_B64 repository secret")
        print("Native dependency absent; running remaining rack and native Flangus regressions")
        return
    try:
        with zipfile.ZipFile(io.BytesIO(base64.b64decode(encoded, validate=True))) as archive:
            if sorted(archive.namelist()) != sorted(FILES):
                raise ValueError("Native dependency must contain exactly the three expected files")
            if any(archive.getinfo(name).file_size > 100000 for name in FILES):
                raise ValueError("Unexpected native dependency file size")
            data = {name: archive.read(name) for name in FILES}
        validate(data)
    except (ValueError, OSError, zipfile.BadZipFile, KeyError) as error:
        raise SystemExit("Native dependency validation failed: " + str(error)) from None
    for name, content in data.items():
        path = root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(content)
    print("Native Classic Chorus installed; three SHA256 checks passed")

if __name__ == "__main__":
    main()
