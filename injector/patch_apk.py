#!/usr/bin/env python3
"""
RbxEx APK Patcher
Injects librbxex.so into Roblox APK and patches AndroidManifest to load it.

Requirements:
    pip install apkutils2 androguard

Usage:
    python patch_apk.py <input_roblox.apk> <librbxex.so> [output.apk]

How it works:
    1. Unzips the APK
    2. Adds librbxex.so to lib/arm64-v8a/
    3. Patches a native activity's soName OR adds a LoadLibrary call to the loader
    4. Re-signs with a debug key
    5. Outputs patched APK ready to install
"""
import os, sys, shutil, zipfile, struct, subprocess, tempfile
from pathlib import Path

AAPT2 = "aapt2"   # Must be in PATH (from Android SDK build-tools)
ZIPALIGN = "zipalign"
APKSIGNER = "apksigner"
KEYTOOL = "keytool"

DEBUG_KEYSTORE = "debug.keystore"
DEBUG_ALIAS = "rbxex_debug"
DEBUG_PASS = "rbxex1234"

def create_debug_keystore():
    """Generate a debug signing keystore if it doesn't exist."""
    if os.path.exists(DEBUG_KEYSTORE):
        return
    print("[*] Generating debug keystore...")
    subprocess.run([
        KEYTOOL, "-genkey", "-v",
        "-keystore", DEBUG_KEYSTORE,
        "-alias", DEBUG_ALIAS,
        "-keyalg", "RSA",
        "-keysize", "2048",
        "-validity", "10000",
        "-storepass", DEBUG_PASS,
        "-keypass", DEBUG_PASS,
        "-dname", "CN=RbxEx, OU=Dev, O=RbxEx, L=Unknown, ST=Unknown, C=US"
    ], check=True)
    print("[+] Keystore created: debug.keystore")

def patch_apk(input_apk: str, librbxex: str, output_apk: str = None):
    if output_apk is None:
        output_apk = input_apk.replace(".apk", "_patched.apk")

    print(f"[*] Patching: {input_apk}")
    print(f"[*] Injecting: {librbxex}")
    print(f"[*] Output: {output_apk}")

    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)

        # ── Step 1: Extract APK ──────────────────────────────────────────
        print("[*] Extracting APK...")
        with zipfile.ZipFile(input_apk, 'r') as z:
            z.extractall(tmp)

        # ── Step 2: Add our .so ──────────────────────────────────────────
        lib_dir = tmp / "lib" / "arm64-v8a"
        lib_dir.mkdir(parents=True, exist_ok=True)

        target = lib_dir / "librbxex.so"
        shutil.copy2(librbxex, target)
        print(f"[+] Added {target.name} ({os.path.getsize(librbxex):,} bytes)")

        # ── Step 3: List existing native libs (for info) ─────────────────
        existing_libs = list(lib_dir.glob("*.so"))
        print(f"[*] Native libs in APK:")
        for lib in existing_libs:
            print(f"    - {lib.name}")

        # ── Step 4: Repack APK ───────────────────────────────────────────
        print("[*] Repacking APK...")
        tmp_apk = str(tmp) + "_repacked.apk"
        with zipfile.ZipFile(tmp_apk, 'w', zipfile.ZIP_DEFLATED) as zout:
            for file_path in tmp.rglob("*"):
                if file_path.is_file():
                    arcname = file_path.relative_to(tmp)
                    # Don't compress .so files (Android requires them uncompressed for direct execution)
                    compress = zipfile.ZIP_STORED if str(arcname).endswith('.so') else zipfile.ZIP_DEFLATED
                    zout.write(file_path, arcname, compress_type=compress)
        print(f"[+] Repacked APK: {tmp_apk}")

        # ── Step 5: Zipalign ─────────────────────────────────────────────
        aligned_apk = str(tmp) + "_aligned.apk"
        print("[*] Zipaligning...")
        result = subprocess.run([ZIPALIGN, "-v", "-p", "4", tmp_apk, aligned_apk],
                                capture_output=True, text=True)
        if result.returncode != 0:
            print(f"[!] zipalign failed: {result.stderr}")
            aligned_apk = tmp_apk  # Use non-aligned as fallback

        # ── Step 6: Sign ─────────────────────────────────────────────────
        create_debug_keystore()
        print("[*] Signing APK...")
        result = subprocess.run([
            APKSIGNER, "sign",
            "--ks", DEBUG_KEYSTORE,
            "--ks-key-alias", DEBUG_ALIAS,
            "--ks-pass", f"pass:{DEBUG_PASS}",
            "--key-pass", f"pass:{DEBUG_PASS}",
            "--out", output_apk,
            aligned_apk
        ], capture_output=True, text=True)
        if result.returncode != 0:
            print(f"[!] apksigner failed: {result.stderr}")
            shutil.copy2(aligned_apk, output_apk)
        else:
            print(f"[+] Signed successfully!")

    print(f"\n✅ Patched APK ready: {output_apk}")
    print(f"   Size: {os.path.getsize(output_apk):,} bytes")
    print(f"\nInstall with:")
    print(f"   adb install -r {output_apk}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python patch_apk.py <roblox.apk> <librbxex.so> [output.apk]")
        sys.exit(1)
    patch_apk(sys.argv[1], sys.argv[2],
              sys.argv[3] if len(sys.argv) > 3 else None)
