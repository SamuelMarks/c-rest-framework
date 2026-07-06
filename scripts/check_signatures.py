import os
import re
import sys


def main():
    has_errors = False

    # Matches: return_type function_name(args)
    func_regex = re.compile(
        r"^(?:static\s+)?(?:inline\s+)?([a-zA-Z_][a-zA-Z0-9_\s\*]+?)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\([^;]*?\{",
        re.MULTILINE,
    )

    for root, _, files in os.walk("."):
        for file in files:
            if file.endswith(".c") or file.endswith(".h"):
                path = os.path.join(root, file)
                if (
                    "build" in path
                    or ".git" in path
                    or "_deps" in path
                    or "emsdk" in path
                    or "examples" in path
                    or "tests" in path
                    or "compliance-openapi" in path
                ):
                    continue

                with open(path, "r", encoding="utf-8") as f:
                    content = f.read()

                content = re.sub(r"/\*.*?\*/", "", content, flags=re.DOTALL)
                content = re.sub(r"//.*", "", content)

                for match in func_regex.finditer(content):
                    ret_type = match.group(1).strip()
                    func_name = match.group(2).strip()

                    if func_name in [
                        "if",
                        "while",
                        "for",
                        "switch",
                        "return",
                        "sizeof",
                    ]:
                        continue

                    # Remove calling conventions
                    ret_type = re.sub(r"\b__stdcall\b", "", ret_type).strip()
                    ret_type = re.sub(r"\b__cdecl\b", "", ret_type).strip()
                    ret_type = re.sub(r"\b__fastcall\b", "", ret_type).strip()
                    ret_type = re.sub(r"\b__declspec\([^\)]+\)\b", "", ret_type).strip()

                    valid_rets = ["c_rest_error_t", "enum c_rest_error", "void"]
                    is_valid = False
                    for v in valid_rets:
                        if ret_type.endswith(v):
                            is_valid = True
                            break

                    if not is_valid:
                        if func_name in ["mock_send"]:
                            continue
                        print(
                            f"{path}: Function {func_name} returns '{ret_type}', which is not an error enum or void."
                        )
                        has_errors = True

    if has_errors:
        sys.exit(1)


if __name__ == "__main__":
    main()
