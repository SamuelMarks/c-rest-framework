$files = Get-ChildItem -Path "src", "include", "tests", "examples" -Include "*.c", "*.h" -Recurse
foreach ($file in $files) {
    $content = Get-Content -Raw $file.FullName
    
    # 1. Clang-format off/on exactly once
    $offCount = [regex]::Matches($content, "clang-format off").Count
    $onCount = [regex]::Matches($content, "clang-format on").Count
    if ($offCount -gt 1 -or $onCount -gt 1) {
        Write-Output "$($file.Name): multiple clang-format off/on"
    } elseif ($offCount -eq 0 -or $onCount -eq 0) {
        if ($content -match "#include") {
            Write-Output "$($file.Name): missing clang-format off/on"
        }
    }
    
    # 2. No windows.h
    if ($content -match "<windows\.h>") {
        Write-Output "$($file.Name): includes <windows.h>"
    }
    
    # 3. cplusplus wrappers (for headers)
    if ($file.Extension -eq ".h") {
        $cxxCount = [regex]::Matches($content, "#ifdef __cplusplus").Count
        if ($cxxCount -gt 1) {
            Write-Output "$($file.Name): multiple __cplusplus"
        } elseif ($cxxCount -eq 0) {
            Write-Output "$($file.Name): missing __cplusplus"
        }
    }
}
