import subprocess
print("Running hook...")
p = subprocess.Popen(['bash', '-c', 'if command -v wine >/dev/null 2>&1; then bash ~/repos/c-ci/build_msvc_wine.sh -DCMAKE_DISABLE_FIND_PACKAGE_SQLite3=ON -DCMAKE_DISABLE_FIND_PACKAGE_ZLIB=ON -DCMAKE_DISABLE_FIND_PACKAGE_Brotli=ON -DCMAKE_DISABLE_FIND_PACKAGE_CURL=ON; fi'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
out, err = p.communicate()
print("Finished!")
print("Exit code:", p.returncode)
