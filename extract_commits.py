#!/usr/bin/env python3
"""
Extract all commits from the last 2 months into separate directories.
Each commit gets a directory named after its short hash (first 6 chars).
"""

import subprocess
import os
import shutil
from datetime import datetime, timedelta

# Configuration
COMMITS_DIR = "./commits"
MONTHS_BACK = 2

def run_command(cmd):
    """Run a shell command and return output."""
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    return result.stdout.strip()

def get_commits_since(months_back):
    """Get list of commits from the last N months."""
    # Calculate date N months ago
    since_date = datetime.now() - timedelta(days=months_back * 30)
    since_str = since_date.strftime("%Y-%m-%d")

    # Get commit hashes with dates
    cmd = f'git log --since="{since_str}" --pretty=format:"%H|%h|%ci|%s" --all'
    output = run_command(cmd)

    if not output:
        return []

    commits = []
    for line in output.split('\n'):
        if line:
            parts = line.split('|')
            if len(parts) >= 4:
                full_hash, short_hash, date, message = parts[0], parts[1], parts[2], '|'.join(parts[3:])
                commits.append({
                    'full_hash': full_hash,
                    'short_hash': short_hash,
                    'date': date,
                    'message': message
                })

    return commits

def extract_commit_to_directory(commit_hash, target_dir):
    """Extract a specific commit's files to a directory."""
    # Create target directory
    os.makedirs(target_dir, exist_ok=True)

    # Get list of files in this commit
    cmd = f'git ls-tree -r --name-only {commit_hash}'
    files = run_command(cmd).split('\n')

    # Extract each file
    extracted_count = 0
    for file_path in files:
        if not file_path:
            continue

        # Skip binary files and build artifacts
        if any(skip in file_path for skip in ['.png', '.PNG', '.jpg', '.exe', '.dll', '.o', '.a',
                                                 'build_', '.profraw', '.profdata', '__pycache__',
                                                 '.git/', 'commits/']):
            continue

        try:
            # Get file content from this commit
            cmd = f'git show {commit_hash}:{file_path}'
            content = subprocess.run(cmd, shell=True, capture_output=True, text=True)

            if content.returncode == 0:
                # Create subdirectories if needed
                output_path = os.path.join(target_dir, file_path)
                os.makedirs(os.path.dirname(output_path), exist_ok=True)

                # Write file
                with open(output_path, 'w', encoding='utf-8', errors='ignore') as f:
                    f.write(content.stdout)

                extracted_count += 1
        except Exception as e:
            print(f"  Warning: Could not extract {file_path}: {e}")

    return extracted_count

def main():
    print("=" * 80)
    print("Git Commit Extractor - Last 2 Months")
    print("=" * 80)

    # Clean up old commits directory if it exists
    if os.path.exists(COMMITS_DIR):
        print(f"\nRemoving old {COMMITS_DIR} directory...")
        shutil.rmtree(COMMITS_DIR)

    # Create commits directory
    os.makedirs(COMMITS_DIR, exist_ok=True)
    print(f"\nCreated {COMMITS_DIR} directory")

    # Get commits from last 2 months
    print(f"\nFetching commits from last {MONTHS_BACK} months...")
    commits = get_commits_since(MONTHS_BACK)

    if not commits:
        print("No commits found in the specified time range.")
        return

    print(f"Found {len(commits)} commits")

    # Create index file
    index_path = os.path.join(COMMITS_DIR, "INDEX.md")
    with open(index_path, 'w') as idx:
        idx.write("# Commit Index - Last 2 Months\n\n")
        idx.write(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n")
        idx.write("| Short Hash | Date | Message | Directory |\n")
        idx.write("|------------|------|---------|----------|\n")

        # Extract each commit
        for i, commit in enumerate(commits, 1):
            short_hash = commit['short_hash']
            full_hash = commit['full_hash']
            date = commit['date'][:10]  # Just the date part
            message = commit['message'][:60]  # Truncate long messages

            print(f"\n[{i}/{len(commits)}] Extracting {short_hash}: {message[:50]}...")

            # Create directory for this commit
            commit_dir = os.path.join(COMMITS_DIR, short_hash)

            # Extract files
            file_count = extract_commit_to_directory(full_hash, commit_dir)

            print(f"  ✓ Extracted {file_count} files to {commit_dir}")

            # Write to index
            idx.write(f"| `{short_hash}` | {date} | {message} | `{short_hash}/` |\n")

            # Create commit info file
            info_path = os.path.join(commit_dir, "COMMIT_INFO.txt")
            with open(info_path, 'w') as info:
                info.write(f"Commit: {full_hash}\n")
                info.write(f"Short: {short_hash}\n")
                info.write(f"Date: {commit['date']}\n")
                info.write(f"Message: {commit['message']}\n")
                info.write(f"\nFiles extracted: {file_count}\n")

    print("\n" + "=" * 80)
    print(f"✓ Extraction complete!")
    print(f"  Total commits: {len(commits)}")
    print(f"  Location: {COMMITS_DIR}/")
    print(f"  Index file: {index_path}")
    print("=" * 80)

if __name__ == "__main__":
    main()
