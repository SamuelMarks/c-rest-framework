---
name: Release Checklist
about: Checklist for releasing a new version of c-rest-framework
title: 'Release: vX.Y.Z'
labels: 'release'
assignees: ''
---

## Pre-Release Checks
- [ ] Ensure all CI workflows pass on `main` branch.
- [ ] Run AddressSanitizer (ASAN) and ThreadSanitizer (TSAN) tests locally.
- [ ] Verify MSVC `/analyze` static analysis reports zero warnings.
- [ ] Confirm `doxygen` runs with `WARN_IF_UNDOCUMENTED = YES` and reports no warnings.
- [ ] Verify 100% test coverage using GCC/Clang `gcov` or `llvm-cov`.
- [ ] Run `clang-format` on the entire codebase.

## Version Bump
- [ ] Update version number in `CMakeLists.txt`.
- [ ] Update version number in `vcpkg.json` (if applicable).
- [ ] Add an entry for the new version in `CHANGELOG.md`.

## Build & Test Matrix Verification
- [ ] MSVC 2005 (DOS/Win95 target compatibility).
- [ ] OpenWatcom (DOS native).
- [ ] MSVC 2022/2026.
- [ ] MinGW-w64.
- [ ] Cygwin.
- [ ] GCC (Ubuntu/Alpine).
- [ ] Clang (macOS/*BSD).

## Finalization
- [ ] Create a signed Git tag for the release (`git tag -s vX.Y.Z -m "Release vX.Y.Z"`).
- [ ] Push tag to GitHub (`git push origin vX.Y.Z`).
- [ ] Draft and publish the GitHub Release with compiled binaries (if any) and source code tarball.
- [ ] Update `vcpkg` registry downstream (optional/subsequent).
