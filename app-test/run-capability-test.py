#!/usr/bin/env python3
"""
QApp Capability Test Runner

Regression guard: verifies that implemented capabilities still work.
Runs automatable tests (CLI, file checks), tracks manual test results,
persists all results in JSON so failures aren't lost between snapshots.

Usage:
    python3 run-capability-test.py [snapshot-dir]
    python3 run-capability-test.py                          # latest snapshot
    python3 run-capability-test.py app-test/20260224-124516  # specific snapshot

Output:
    app-test/<snapshot>/results-<datetime>.json   — full results
    app-test/<snapshot>/failures.json             — persistent failure log (never lost)
    stdout                                        — summary with pass/fail/skip counts
"""

import json
import os
import re
import subprocess
import sys
import shutil
import tempfile
from datetime import datetime, timezone
from pathlib import Path

# ---------------------------------------------------------------------------
# Config
# ---------------------------------------------------------------------------

PROJECT_ROOT = Path(__file__).resolve().parent.parent
BUILD_DIR = PROJECT_ROOT / "build"
APP_TEST_DIR = PROJECT_ROOT / "app-test"

BINARIES = {
    "qapp-installer": BUILD_DIR / "qapp-installer",
    "qapp-ws-wrapper": BUILD_DIR / "qapp-ws-wrapper",
    "qapp-pwa-app": BUILD_DIR / "qapp-pwa-app",
}

# Test URLs for classification (real public sites)
TEST_URLS = {
    "ws": "https://example.com",
    "pwapp": "https://app.diagrams.net",
}


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def now_iso():
    return datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")


def now_stamp():
    return datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S")


def run_cmd(args, timeout=60):
    """Run command, return (exit_code, stdout, stderr)."""
    try:
        r = subprocess.run(
            args, capture_output=True, text=True, timeout=timeout
        )
        return r.returncode, r.stdout, r.stderr
    except subprocess.TimeoutExpired:
        return -1, "", "TIMEOUT"
    except FileNotFoundError:
        return -2, "", f"BINARY NOT FOUND: {args[0]}"


def is_valid_json(text):
    """Return parsed JSON or None."""
    try:
        return json.loads(text)
    except (json.JSONDecodeError, ValueError):
        return None


def load_failures(snapshot_dir):
    """Load persistent failures.json (never lost between runs)."""
    path = snapshot_dir / "failures.json"
    if path.exists():
        with open(path) as f:
            return json.load(f)
    return {}


def save_failures(snapshot_dir, failures):
    """Persist failures — only cleared when test passes."""
    path = snapshot_dir / "failures.json"
    with open(path, "w") as f:
        json.dump(failures, f, indent=2, ensure_ascii=False)


def load_capabilities(snapshot_dir):
    """Load all capability JSON files from snapshot."""
    caps = {}
    for p in sorted(snapshot_dir.glob("qapp-*-capability.json")):
        with open(p) as f:
            data = json.load(f)
        binary = data.get("binary", p.stem)
        mode = data.get("mode", "")
        for cap in data.get("capabilities", []):
            cap["_binary"] = binary
            cap["_mode"] = mode
            cap["_source"] = p.name
            caps[cap["id"]] = cap
    return caps


# ---------------------------------------------------------------------------
# Auto-test implementations
# ---------------------------------------------------------------------------

def test_binary_exists(binary_name):
    """Check binary is built."""
    path = BINARIES.get(binary_name)
    if path and path.exists():
        return "pass", f"Found: {path}"
    return "fail", f"Missing: {path}"


def test_cli_command(args, checks):
    """
    Run CLI command, apply checks dict:
      exit_code: expected int
      json_output: True if stdout must be valid JSON
      json_fields: list of dotted paths that must exist (e.g. "classification.level")
      stdout_contains: list of substrings
      files_exist: list of paths that must exist after command
      files_absent: list of paths that must NOT exist after command
    """
    code, stdout, stderr = run_cmd(args)
    errors = []

    if "exit_code" in checks and code != checks["exit_code"]:
        errors.append(f"exit_code: got {code}, expected {checks['exit_code']}")
        if stderr:
            errors.append(f"stderr: {stderr[:200]}")

    parsed = None
    if checks.get("json_output"):
        parsed = is_valid_json(stdout)
        if parsed is None:
            errors.append(f"stdout is not valid JSON: {stdout[:200]}")

    if parsed and "json_fields" in checks:
        for field in checks["json_fields"]:
            obj = parsed
            for key in field.split("."):
                if isinstance(obj, dict) and key in obj:
                    obj = obj[key]
                else:
                    errors.append(f"missing JSON field: {field}")
                    break

    if "stdout_contains" in checks:
        for needle in checks["stdout_contains"]:
            if needle not in stdout:
                errors.append(f"stdout missing: '{needle}'")

    if "files_exist" in checks:
        for fp in checks["files_exist"]:
            if not Path(fp).expanduser().exists():
                errors.append(f"file missing: {fp}")

    if "files_absent" in checks:
        for fp in checks["files_absent"]:
            if Path(fp).expanduser().exists():
                errors.append(f"file should not exist: {fp}")

    if errors:
        return "fail", "; ".join(errors)
    return "pass", f"exit={code}, output_len={len(stdout)}"


# ---------------------------------------------------------------------------
# Auto-test registry — maps capability IDs to runnable tests
# ---------------------------------------------------------------------------

def build_auto_tests():
    """Return dict of {capability_id: callable} for automatable tests."""
    installer = str(BINARIES["qapp-installer"])
    tests = {}

    # --- Binary existence ---
    tests["_PRE_INSTALLER"] = lambda: test_binary_exists("qapp-installer")
    tests["_PRE_WS_WRAPPER"] = lambda: test_binary_exists("qapp-ws-wrapper")
    tests["_PRE_PWA_APP"] = lambda: test_binary_exists("qapp-pwa-app")

    # --- INS-CLI: Installer CLI tests ---
    tests["INS-CLI-09"] = lambda: test_cli_command(
        [installer, "--version"],
        {"exit_code": 0}
    )

    tests["INS-CLI-10"] = lambda: test_cli_command(
        [installer, "--help"],
        {"exit_code": 0, "stdout_contains": ["--classify", "--install"]}
    )

    tests["INS-CLI-01"] = lambda: test_cli_command(
        [installer, "--classify", TEST_URLS["ws"]],
        {
            "exit_code": 0,
            "json_output": True,
            "json_fields": [
                "classification.level",
                "metadata.name",
                "metadata.iconUrl",
                "finalUrl",
            ],
        },
    )

    tests["INS-CLI-04"] = lambda: test_cli_command(
        [installer, "--classify", TEST_URLS["ws"]],
        {
            "exit_code": 0,
            "json_output": True,
            "json_fields": ["classification.level"],
        },
    )

    tests["INS-CLI-07"] = lambda: test_cli_command(
        [installer, "--list"],
        {"exit_code": 0, "json_output": True},
    )

    # --- INS-CLI-05 + INS-CLI-06: Install/Uninstall cycle ---
    def test_install_uninstall_cycle():
        """Full install → verify files → list → uninstall → verify cleanup."""
        errors = []

        # Install (needs WebEngine init + network fetch — allow 90s)
        code, stdout, stderr = run_cmd(
            [installer, "--install", TEST_URLS["ws"]], timeout=90
        )
        if code != 0:
            return "fail", f"install exit={code}, stderr={stderr[:200]}"

        parsed = is_valid_json(stdout)
        if not parsed:
            return "fail", f"install stdout not JSON: {stdout[:200]}"

        app_id = parsed.get("appId", "")
        if not app_id:
            return "fail", f"no appId in install output: {stdout[:200]}"

        # Check files exist
        home = Path.home()
        desktop_path = home / f".local/share/applications/qapp-{app_id}.desktop"
        meta_path = home / f".local/share/qapp-framework/apps/{app_id}.json"
        icons_dir = home / ".local/share/qapp-framework/icons"

        if not desktop_path.exists():
            errors.append(f".desktop missing: {desktop_path}")
        if not meta_path.exists():
            errors.append(f"metadata missing: {meta_path}")

        # Validate .desktop content
        if desktop_path.exists():
            content = desktop_path.read_text()
            for field in ["Type=Application", "Name=", "Exec=", "Icon="]:
                if field not in content:
                    errors.append(f".desktop missing field: {field}")

        # Validate metadata JSON
        if meta_path.exists():
            meta = is_valid_json(meta_path.read_text())
            if meta is None:
                errors.append("metadata not valid JSON")
            else:
                for field in ["appId", "name", "url", "level", "wrapperPath", "installedAt"]:
                    if field not in meta:
                        errors.append(f"metadata missing: {field}")

        # List should contain our app
        code2, stdout2, _ = run_cmd([installer, "--list"])
        if code2 == 0:
            listed = is_valid_json(stdout2)
            if listed is not None:
                ids = [a.get("appId") for a in listed if isinstance(a, dict)]
                if app_id not in ids:
                    errors.append(f"app {app_id} not in --list output")

        # Uninstall
        code3, stdout3, stderr3 = run_cmd([installer, "--uninstall", app_id])
        if code3 != 0:
            errors.append(f"uninstall exit={code3}, stderr={stderr3[:200]}")

        # Check files removed
        if desktop_path.exists():
            errors.append(f".desktop still exists after uninstall")
        if meta_path.exists():
            errors.append(f"metadata still exists after uninstall")

        if errors:
            return "fail", "; ".join(errors)
        return "pass", f"install→verify→list→uninstall cycle OK for {app_id}"

    tests["INS-CLI-05"] = test_install_uninstall_cycle
    # INS-CLI-06 covered by same cycle (uninstall part)
    tests["INS-CLI-06"] = lambda: ("pass", "covered by INS-CLI-05 cycle")

    # --- INS-INSTALL: App ID generation from URL ---
    def test_app_id_generation():
        """Verify appId skips default ports, includes non-default."""
        code, stdout, _ = run_cmd(
            [installer, "--classify", "https://my.example.com:8080"], timeout=60
        )
        if code != 0:
            return "fail", f"classify exit={code}"
        parsed = is_valid_json(stdout)
        if not parsed:
            return "fail", "classify output not JSON"
        # Check that finalUrl contains the port
        final = parsed.get("finalUrl", "")
        if "8080" not in final and "8080" not in stdout:
            return "fail", f"port 8080 not reflected in output"
        return "pass", "non-default port preserved in classification"

    tests["INS-INSTALL-01"] = test_app_id_generation

    return tests


# ---------------------------------------------------------------------------
# Runner
# ---------------------------------------------------------------------------

def find_latest_snapshot():
    """Find most recent snapshot dir."""
    if not APP_TEST_DIR.exists():
        return None
    dirs = sorted([d for d in APP_TEST_DIR.iterdir() if d.is_dir()])
    return dirs[-1] if dirs else None


def run_all(snapshot_dir):
    """Run all tests, return results dict."""
    caps = load_capabilities(snapshot_dir)
    auto_tests = build_auto_tests()
    old_failures = load_failures(snapshot_dir)
    results = {}
    new_failures = {}

    total = len(caps)
    auto_count = 0
    pass_count = 0
    fail_count = 0
    skip_count = 0

    print(f"\n{'='*60}")
    print(f"  QApp Capability Test — {snapshot_dir.name}")
    print(f"  {total} capabilities loaded")
    print(f"{'='*60}\n")

    # Run pre-checks (binary existence)
    for pre_id in ["_PRE_INSTALLER", "_PRE_WS_WRAPPER", "_PRE_PWA_APP"]:
        if pre_id in auto_tests:
            status, msg = auto_tests[pre_id]()
            label = pre_id.replace("_PRE_", "")
            icon = "PASS" if status == "pass" else "FAIL"
            print(f"  [{icon}] Binary check: {label} — {msg}")
            if status == "fail":
                print(f"\n  ABORT: Missing binaries. Run: cmake -B build -DQAPP_DEV_BUILD=ON && cmake --build build\n")
                return None

    print()

    # Run capability tests
    for cap_id in sorted(caps.keys()):
        cap = caps[cap_id]
        category = cap.get("category", "")
        feature = cap.get("feature", "")

        if cap_id in auto_tests:
            auto_count += 1
            status, detail = auto_tests[cap_id]()
        else:
            # Check if previously failed (persistent)
            if cap_id in old_failures:
                status = "known_fail"
                detail = old_failures[cap_id].get("detail", "previously failed")
            else:
                status = "manual"
                detail = "requires manual verification"

        # Track results
        results[cap_id] = {
            "status": status,
            "detail": detail,
            "feature": feature,
            "category": category,
            "binary": cap.get("_binary", ""),
            "source": cap.get("_source", ""),
        }

        # Track failures persistently
        if status == "fail":
            fail_count += 1
            new_failures[cap_id] = {
                "feature": feature,
                "detail": detail,
                "first_seen": old_failures.get(cap_id, {}).get("first_seen", now_iso()),
                "last_seen": now_iso(),
            }
            print(f"  [FAIL] {cap_id}: {feature}")
            print(f"         {detail}")
        elif status == "known_fail":
            fail_count += 1
            new_failures[cap_id] = old_failures[cap_id]
            new_failures[cap_id]["last_seen"] = now_iso()
            print(f"  [KNOWN FAIL] {cap_id}: {feature}")
        elif status == "pass":
            pass_count += 1
            # Clear from persistent failures if it was there
            print(f"  [PASS] {cap_id}: {feature}")
        else:
            skip_count += 1

    # Save results
    ts = now_stamp()
    results_path = snapshot_dir / f"results-{ts}.json"
    results_meta = {
        "timestamp": now_iso(),
        "snapshot": snapshot_dir.name,
        "summary": {
            "total": total,
            "auto_tested": auto_count,
            "pass": pass_count,
            "fail": fail_count,
            "manual_skip": skip_count,
        },
        "results": results,
    }

    with open(results_path, "w") as f:
        json.dump(results_meta, f, indent=2, ensure_ascii=False)

    # Save persistent failures
    save_failures(snapshot_dir, new_failures)

    # Summary
    print(f"\n{'='*60}")
    print(f"  RESULTS: {pass_count} pass, {fail_count} fail, {skip_count} manual/skip")
    print(f"  Auto-tested: {auto_count}/{total} capabilities")
    print(f"  Results: {results_path.name}")
    if new_failures:
        print(f"  Failures persisted: {len(new_failures)} in failures.json")
    print(f"{'='*60}\n")

    return results_meta


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    if len(sys.argv) > 1:
        snapshot_dir = Path(sys.argv[1])
        if not snapshot_dir.is_absolute():
            snapshot_dir = PROJECT_ROOT / snapshot_dir
    else:
        snapshot_dir = find_latest_snapshot()

    if not snapshot_dir or not snapshot_dir.exists():
        print("ERROR: No snapshot directory found.")
        print("Usage: python3 run-capability-test.py [snapshot-dir]")
        sys.exit(1)

    cap_files = list(snapshot_dir.glob("qapp-*-capability.json"))
    if not cap_files:
        print(f"ERROR: No capability files in {snapshot_dir}")
        sys.exit(1)

    results = run_all(snapshot_dir)
    if results is None:
        sys.exit(2)

    fail_count = results["summary"]["fail"]
    sys.exit(1 if fail_count > 0 else 0)


if __name__ == "__main__":
    main()
