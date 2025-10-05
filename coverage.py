#!/usr/bin/env python3
"""
Run NodeGraph visibly/offscreen N times loading an XML file,
optionally collect LLVM coverage and generate an HTML report.

Usage (from WSL, in your repo):
  python3 tools/run_graph.py --xml ../tests_large.xml --count 5
  python3 tools/run_graph.py --xml ../tests_large.xml --count 5 --coverage
  python3 tools/run_graph.py --xml ../tests_large.xml --count 3 --offscreen
  python3 tools/run_graph.py --xml ../tests_large.xml --coverage --timeout 8

Defaults assume:
  - Visible run (requires Windows X server; DISPLAY=:0)
  - Binary is build_llvm/NodeGraph, or build_llvm_cov/NodeGraph when --coverage
"""

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

def which_llvm_tool(name: str):
    """Prefer version-suffixed tools first (e.g., -18..-14), else unsuffixed."""
    for v in map(str, range(20, 11, -1)):  # try 20 down to 12
        cand = shutil.which(f"{name}-{v}")
        if cand:
            return cand
    return shutil.which(name)

def run(cmd, *, env=None, cwd=None, check=True):
    print("+", " ".join(map(str, cmd)))
    return subprocess.run(cmd, env=env, cwd=cwd, check=check)

def main():
    parser = argparse.ArgumentParser(description="Run NodeGraph N times and optionally collect LLVM coverage.")
    parser.add_argument("--xml", required=True, help="Path to XML file to load (relative to CWD).")
    parser.add_argument("--count", type=int, default=1, help="How many times to run the app.")
    parser.add_argument("--bin", default=None, help="Path to NodeGraph binary. Defaults to build_llvm(_cov)/NodeGraph.")
    parser.add_argument("--coverage", action="store_true", help="Enable LLVM source-based coverage and generate HTML.")
    parser.add_argument("--offscreen", action="store_true", help="Run with QT_QPA_PLATFORM=offscreen (no visible window).")
    parser.add_argument("--timeout", type=int, default=0, help="Seconds to allow each run before killing it (0=disabled).")
    parser.add_argument("--html-dir", default="coverage-html", help="Output dir for HTML coverage.")
    parser.add_argument("--profile-file", default=None, help="LLVM_PROFILE_FILE pattern. Default: ./default.%p.profraw")
    args = parser.parse_args()

    repo = Path.cwd()
    xml_path = (repo / args.xml).resolve()
    if not xml_path.exists():
        print(f"ERROR: XML not found: {xml_path}", file=sys.stderr)
        sys.exit(2)

    # Choose binary
    if args.bin:
        bin_path = Path(args.bin).resolve()
    else:
        bin_dir = repo / ("build_llvm_cov" if args.coverage else "build_llvm")
        bin_path = (bin_dir / "NodeGraph").resolve()

    if not bin_path.exists():
        print(f"ERROR: Binary not found: {bin_path}\n"
              f"Hint: build first (e.g., ./build.sh {'coverage' if args.coverage else 'debug'})",
              file=sys.stderr)
        sys.exit(2)

    # Environment
    env = os.environ.copy()

    # Visible vs offscreen
    if args.offscreen:
        env["QT_QPA_PLATFORM"] = "offscreen"
    else:
        # visible; ensure DISPLAY is set
        env.setdefault("DISPLAY", ":0")

    # Coverage setup
    profraw_glob = []
    profdata = Path("default.profdata").resolve()
    html_dir = Path(args.html_dir).resolve()
    if args.coverage:
        # default profile file pattern
        if args.profile_file:
            profpat = args.profile_file
        else:
            profpat = str((repo / "default.%p.profraw").resolve())
        # pick llvm tools
        llvm_profdata = which_llvm_tool("llvm-profdata")
        llvm_cov = which_llvm_tool("llvm-cov")
        if not llvm_profdata or not llvm_cov:
            print("ERROR: llvm-profdata/llvm-cov not found on PATH. Install LLVM tools.", file=sys.stderr)
            sys.exit(3)
        print(f"Using llvm tools:\n  llvm-profdata: {llvm_profdata}\n  llvm-cov     : {llvm_cov}")
    else:
        llvm_profdata = llvm_cov = None
        profpat = None

    # Run loop
    for i in range(1, args.count + 1):
        print(f"\n=== Run #{i}/{args.count} ===")

        local_env = env.copy()
        if args.coverage:
            # unique profraw per run (includes PID)
            # e.g., /path/default.3.%p.profraw
            local_env["LLVM_PROFILE_FILE"] = profpat
        cmd = [str(bin_path), "--load", str(xml_path)]

        if args.timeout and args.timeout > 0:
            # Use timeout if available (GNU coreutils). If not, just run.
            if shutil.which("timeout"):
                cmd = ["timeout", f"{args.timeout}s"] + cmd
            else:
                print("WARN: timeout not found; running without a timeout.", file=sys.stderr)

        try:
            run(cmd, env=local_env, cwd=bin_path.parent)
        except subprocess.CalledProcessError as e:
            print(f"Run #{i} exited with code {e.returncode}", file=sys.stderr)
            # continue to next run but remember failure
            # You can sys.exit(1) here if you'd rather fail fast.

    # Coverage post-processing
    if args.coverage:
        # Collect all .profraw created in repo root
        profraws = sorted(Path(repo).glob("default.*.profraw"))
        if not profraws:
            print("ERROR: No .profraw files found. Did the runs execute under coverage?", file=sys.stderr)
            sys.exit(4)
        print("\nMerging profiles…")
        merge_cmd = [llvm_profdata, "merge", "-sparse"] + [str(p) for p in profraws] + ["-o", str(profdata)]
        run(merge_cmd)

        print("\nCoverage summary:")
        run([llvm_cov, "report", str(bin_path), f"-instr-profile={profdata}", "-use-color"])

        print("\nGenerating HTML…")
        if html_dir.exists():
            shutil.rmtree(html_dir)
        show_cmd = [
            llvm_cov, "show", str(bin_path),
            f"-instr-profile={profdata}",
            "-format=html", "-output-dir", str(html_dir),
            r"-ignore-filename-regex=(Qt|moc_|/usr/include/)"
        ]
        run(show_cmd)
        print(f"\nHTML report: {html_dir}/index.html")

if __name__ == "__main__":
    main()
