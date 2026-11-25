"""Install the exact owner-supplied Flangus source for the one normal build."""
import argparse
import base64
import hashlib
import io
import os
from pathlib import Path
import zipfile

FILES = {
    "Flangus.cpp": "ddc4c62c73778316fec062288391487cbf96b6b490df94342d75dba97c892359",
    "SkellFlangusAdapter.h": "080c20c7a54bd443a70735fc01f4f08b8bf4617a9ba4bcf48808f9e079aeed8d",
    "core/FlangusFastDist.h": "f307be8fb18320e2ad5751f42b5edcfe6f88f9cf68fde27f0444946b6e189874",
}
def validate(data):
    for name, digest in FILES.items():
        if hashlib.sha256(data[name]).hexdigest() != digest:
            raise ValueError("Native source SHA256 mismatch: " + name)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--required", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1] / "Source" / "NativeFlangus"
    if any((root / name).exists() for name in FILES):
        if not all((root / name).is_file() for name in FILES):
            raise SystemExit("Incomplete native Flangus dependency")
        validate({name: (root / name).read_bytes() for name in FILES})
        print("Native Flangus source SHA256 checks passed")
        return
    encoded = os.environ.get("NATIVE_SOURCE_B64", "").strip()
    if not encoded:
        if args.required:
            raise SystemExit("Native Flangus required: set SKELL_FLANGUS_SOURCE_B64 repository secret")
        print("Native dependency absent; running nonnative rack regressions only")
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
    print("Native Flangus installed; three SHA256 checks passed")

if __name__ == "__main__":
    main()
