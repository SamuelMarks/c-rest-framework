import json
import glob
import sys

def main():
    try:
        with open('build_cov/coverage.json') as f:
            cov = json.load(f)
    except Exception as e:
        print("Error opening build_cov/coverage.json:", e)
        return

    files = cov.get('files', [])
    cov_map = {}
    for file_obj in files:
        filename = file_obj['file']
        # Only consider project source files
        if not (filename.startswith('src/') or filename.startswith('examples/') or filename.startswith('tests/')):
            continue
        cov_map[filename] = file_obj

    results = []
    for d in ['src', 'examples', 'tests']:
        for path in sorted(glob.glob(f'{d}/**/*.c', recursive=True)):
            if path.endswith('test_graphql_ext.c'):
                continue
            if path in cov_map:
                file_obj = cov_map[path]
                lines_total = len(file_obj['lines'])
                lines_exec = sum(1 for l in file_obj['lines'] if l['count'] > 0)
                line_cov = (lines_exec / lines_total * 100.0) if lines_total > 0 else 100.0

                branches_total = sum(len(l.get('branches', [])) for l in file_obj['lines'])
                branches_exec = sum(sum(1 for b in l.get('branches', []) if b['count'] > 0) for l in file_obj['lines'])
                branch_cov = (branches_exec / branches_total * 100.0) if branches_total > 0 else 100.0

                functions = file_obj.get('functions', [])
                funcs_total = len(functions)
                funcs_exec = sum(1 for fn in functions if fn['execution_count'] > 0)
                func_cov = (funcs_exec / funcs_total * 100.0) if funcs_total > 0 else 100.0

                is_100 = (line_cov >= 100.0 and branch_cov >= 100.0 and func_cov >= 100.0)
                results.append({
                    'file': path,
                    'line_cov': line_cov,
                    'lines_exec': lines_exec,
                    'lines_total': lines_total,
                    'branch_cov': branch_cov,
                    'branches_exec': branches_exec,
                    'branches_total': branches_total,
                    'func_cov': func_cov,
                    'funcs_exec': funcs_exec,
                    'funcs_total': funcs_total,
                    'is_100': is_100
                })
            else:
                results.append({
                    'file': path,
                    'line_cov': 0.0,
                    'lines_exec': 0,
                    'lines_total': 0,
                    'branch_cov': 0.0,
                    'branches_exec': 0,
                    'branches_total': 0,
                    'func_cov': 0.0,
                    'funcs_exec': 0,
                    'funcs_total': 0,
                    'is_100': False
                })

    results.sort(key=lambda x: x['file'])

    src_under = [r for r in results if r['file'].startswith('src/') and not r['is_100']]
    src_covered = [r for r in results if r['file'].startswith('src/') and r['is_100']]

    examples_under = [r for r in results if r['file'].startswith('examples/') and not r['is_100']]
    examples_covered = [r for r in results if r['file'].startswith('examples/') and r['is_100']]

    tests_under = [r for r in results if r['file'].startswith('tests/') and not r['is_100']]
    tests_covered = [r for r in results if r['file'].startswith('tests/') and r['is_100']]

    total_files = len(results)
    undercovered_files = sum(1 for r in results if not r['is_100'])
    covered_files = sum(1 for r in results if r['is_100'])

    print(f"Total files: {total_files}")
    print(f"Undercovered files: {undercovered_files}")
    print(f"Fully covered files: {covered_files}")

    out = [
        "# Undercovered Files (< 100% Coverage)",
        "",
        "This document tracks all files in `c-rest-framework` with less than **100% test coverage** across **functions**, **lines**, or **branches**.",
        "",
        "> **Coverage Methodology:** Measured using `gcov` and `gcovr` with `--coverage` enabled across all test suite executables.",
        "",
        f"- **Total Files Evaluated:** {total_files}",
        f"- **Undercovered Files (< 100%):** {undercovered_files}",
        f"- **Fully Covered Files (100%):** {covered_files}",
        "",
        "---",
        "",
        f"## 1. Core Source Files (`src/`) — {len(src_under)} Undercovered",
        "",
    ]

    for r in src_under:
        note = " (Not compiled in current target)" if r['lines_total'] == 0 else ""
        out.append(f"- [ ] `{r['file']}` (Lines: {r['line_cov']:.1f}%, Functions: {r['func_cov']:.1f}%, Branches: {r['branch_cov']:.1f}%){note}")

    out.extend([
        "",
        "---",
        "",
        f"## 2. Example Applications (`examples/`) — {len(examples_under)} Undercovered",
        "",
    ])

    for r in examples_under:
        note = " (Not compiled in current target)" if r['lines_total'] == 0 else ""
        out.append(f"- [ ] `{r['file']}` (Lines: {r['line_cov']:.1f}%, Functions: {r['func_cov']:.1f}%, Branches: {r['branch_cov']:.1f}%){note}")

    out.extend([
        "",
        "---",
        "",
        f"## 3. Test Suite Source Files (`tests/`) — {len(tests_under)} Undercovered",
        "",
    ])

    for r in tests_under:
        note = " (Not compiled in current target)" if r['lines_total'] == 0 else ""
        out.append(f"- [ ] `{r['file']}` (Lines: {r['line_cov']:.1f}%, Functions: {r['func_cov']:.1f}%, Branches: {r['branch_cov']:.1f}%){note}")

    out.extend([
        "",
        "---",
        "",
        f"## 4. Fully Covered Files (100% Functions, Lines, and Branches) — {covered_files} Files",
        "",
        f"### Core Source Files (`src/`) — {len(src_covered)} Files",
        "",
    ])
    for r in src_covered:
        out.append(f"- [x] `{r['file']}` (Lines: 100.0%, Functions: 100.0%, Branches: 100.0%)")

    out.extend([
        "",
        f"### Example Applications (`examples/`) — {len(examples_covered)} Files",
        "",
    ])
    for r in examples_covered:
        out.append(f"- [x] `{r['file']}` (Lines: 100.0%, Functions: 100.0%, Branches: 100.0%)")

    out.extend([
        "",
        f"### Test Suite Source Files (`tests/`) — {len(tests_covered)} Files",
        "",
    ])
    for r in tests_covered:
        out.append(f"- [x] `{r['file']}` (Lines: 100.0%, Functions: 100.0%, Branches: 100.0%)")

    with open('UNDERCOVERED.md', 'w') as f:
        f.write("\n".join(out) + "\n")
    print("Updated UNDERCOVERED.md successfully.")

if __name__ == '__main__':
    main()
