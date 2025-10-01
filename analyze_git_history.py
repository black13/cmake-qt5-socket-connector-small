#!/usr/bin/env python3
"""
Git History Analyzer - Search for JavaScript embedding patterns in C++ code
Analyzes how JavaScript was integrated into C++ and identifies potential issues.
"""

import sys
import re
from pathlib import Path
from collections import defaultdict
from datetime import datetime

try:
    import git
except ImportError:
    print("ERROR: GitPython not installed. Run: pip install GitPython")
    sys.exit(1)


class GitHistoryAnalyzer:
    def __init__(self, repo_path="."):
        self.repo = git.Repo(repo_path)
        self.js_patterns = {
            'raw_string_literal': r'R"\((.*?)\)"',  # C++ raw string literals
            'heredoc_style': r'QString.*=.*R"',  # QString with raw strings
            'qjs_evaluate': r'evaluate\s*\(',  # QJSEngine::evaluate calls
            'js_multiline': r'QString.*\n.*console\.log',  # Multi-line JS strings
            'script_embedding': r'(testScript|jsScript).*=',  # Script variable assignments
        }

    def search_commits(self, search_terms, max_commits=100):
        """Search commit messages and diffs for specific terms"""
        results = []

        print(f"\n🔍 Searching last {max_commits} commits for: {', '.join(search_terms)}")
        print("=" * 80)

        for i, commit in enumerate(self.repo.iter_commits('HEAD', max_count=max_commits)):
            if i % 10 == 0:
                print(f"Progress: {i}/{max_commits} commits scanned...", end='\r')

            # Search commit message
            message_match = any(term.lower() in commit.message.lower() for term in search_terms)

            # Search diff
            diff_matches = []
            try:
                if commit.parents:
                    diffs = commit.parents[0].diff(commit, create_patch=True)
                    for diff in diffs:
                        if diff.diff:
                            diff_text = diff.diff.decode('utf-8', errors='ignore')
                            for term in search_terms:
                                if term.lower() in diff_text.lower():
                                    diff_matches.append((term, diff.a_path or diff.b_path))
            except Exception as e:
                pass

            if message_match or diff_matches:
                results.append({
                    'commit': commit,
                    'message_match': message_match,
                    'diff_matches': diff_matches
                })

        print(f"\n✅ Found {len(results)} matching commits")
        return results

    def analyze_js_embedding(self, commit):
        """Analyze how JavaScript was embedded in a specific commit"""
        patterns_found = defaultdict(list)

        try:
            if not commit.parents:
                return patterns_found

            diffs = commit.parents[0].diff(commit, create_patch=True)

            for diff in diffs:
                if not diff.diff:
                    continue

                # Only analyze C++ files
                path = diff.a_path or diff.b_path
                if not path or not (path.endswith('.cpp') or path.endswith('.h')):
                    continue

                diff_text = diff.diff.decode('utf-8', errors='ignore')

                # Check each pattern
                for pattern_name, pattern_regex in self.js_patterns.items():
                    matches = re.findall(pattern_regex, diff_text, re.MULTILINE | re.DOTALL)
                    if matches:
                        patterns_found[pattern_name].append({
                            'file': path,
                            'matches': len(matches),
                            'examples': matches[:2]  # Keep first 2 examples
                        })

        except Exception as e:
            print(f"Warning: Error analyzing commit {commit.hexsha[:8]}: {e}")

        return patterns_found

    def find_js_related_commits(self, max_commits=200):
        """Find all commits related to JavaScript integration"""
        js_terms = [
            'javascript', 'qjs', 'evaluate', 'script',
            'test', 'embed', 'R"(', 'console.log'
        ]

        return self.search_commits(js_terms, max_commits)

    def print_results(self, results):
        """Print analysis results in readable format"""
        print("\n📊 ANALYSIS RESULTS")
        print("=" * 80)

        for i, result in enumerate(results[:20], 1):  # Show top 20
            commit = result['commit']

            print(f"\n{i}. Commit: {commit.hexsha[:8]}")
            print(f"   Date: {datetime.fromtimestamp(commit.committed_date).strftime('%Y-%m-%d %H:%M:%S')}")
            print(f"   Author: {commit.author.name}")
            print(f"   Message: {commit.message.split(chr(10))[0][:70]}")

            if result['diff_matches']:
                print(f"   Matches: {len(result['diff_matches'])} diff matches")
                for term, path in result['diff_matches'][:3]:
                    print(f"      - '{term}' in {path}")

            # Analyze JS embedding patterns
            patterns = self.analyze_js_embedding(commit)
            if patterns:
                print(f"   JS Patterns:")
                for pattern_name, occurrences in patterns.items():
                    total = sum(o['matches'] for o in occurrences)
                    files = [o['file'] for o in occurrences]
                    print(f"      - {pattern_name}: {total} matches in {len(files)} file(s)")

    def analyze_failures(self, results):
        """Analyze potential failure patterns"""
        print("\n⚠️  POTENTIAL FAILURE PATTERNS")
        print("=" * 80)

        failure_indicators = {
            'revert': r'revert|undo|rollback',
            'fix': r'fix|bug|error|crash',
            'refactor': r'refactor|cleanup|remove',
            'test_removal': r'remove.*test|delete.*test',
        }

        failures = defaultdict(list)

        for result in results:
            commit = result['commit']
            msg_lower = commit.message.lower()

            for indicator, pattern in failure_indicators.items():
                if re.search(pattern, msg_lower):
                    failures[indicator].append({
                        'commit': commit.hexsha[:8],
                        'date': datetime.fromtimestamp(commit.committed_date).strftime('%Y-%m-%d'),
                        'message': commit.message.split('\n')[0][:60]
                    })

        for indicator, commits in failures.items():
            if commits:
                print(f"\n{indicator.upper()} ({len(commits)} commits):")
                for c in commits[:5]:
                    print(f"  [{c['date']}] {c['commit']}: {c['message']}")

    def extract_js_code_examples(self, results):
        """Extract actual JavaScript code examples from history"""
        print("\n💡 JAVASCRIPT CODE EXAMPLES FROM HISTORY")
        print("=" * 80)

        examples = []

        for result in results[:30]:
            commit = result['commit']

            try:
                if not commit.parents:
                    continue

                diffs = commit.parents[0].diff(commit, create_patch=True)

                for diff in diffs:
                    if not diff.diff:
                        continue

                    path = diff.a_path or diff.b_path
                    if not path or not path.endswith('.cpp'):
                        continue

                    diff_text = diff.diff.decode('utf-8', errors='ignore')

                    # Look for raw string literals with JavaScript
                    raw_string_matches = re.findall(
                        r'R"\((.*?)\)"',
                        diff_text,
                        re.MULTILINE | re.DOTALL
                    )

                    for js_code in raw_string_matches:
                        if 'console.log' in js_code or 'Graph.' in js_code:
                            examples.append({
                                'commit': commit.hexsha[:8],
                                'date': datetime.fromtimestamp(commit.committed_date).strftime('%Y-%m-%d'),
                                'file': path,
                                'code': js_code[:200]  # First 200 chars
                            })

            except Exception as e:
                pass

        # Print unique examples
        seen = set()
        for ex in examples[:10]:
            code_key = ex['code'][:50]
            if code_key not in seen:
                seen.add(code_key)
                print(f"\n[{ex['date']}] {ex['commit']} - {ex['file']}")
                print(f"```javascript")
                print(ex['code'])
                print(f"```")


def main():
    print("🔬 Git History Analyzer - JavaScript Embedding Analysis")
    print("=" * 80)

    try:
        analyzer = GitHistoryAnalyzer()
    except git.InvalidGitRepositoryError:
        print("ERROR: Not a git repository")
        sys.exit(1)

    # Find JavaScript-related commits
    results = analyzer.find_js_related_commits(max_commits=300)

    if not results:
        print("No JavaScript-related commits found")
        return

    # Print detailed results
    analyzer.print_results(results)

    # Analyze failure patterns
    analyzer.analyze_failures(results)

    # Extract code examples
    analyzer.extract_js_code_examples(results)

    print("\n" + "=" * 80)
    print("✅ Analysis complete!")


if __name__ == "__main__":
    main()
