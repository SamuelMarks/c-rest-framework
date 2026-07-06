import json
import xml.etree.ElementTree as ET
import os
import glob
import re


def get_test_coverage():
    try:
        with open("build/coverage_summary.json", "r") as f:
            data = json.load(f)
            return data["line_percent"]
    except Exception as e:
        print("Could not read test coverage:", e)
        return None


def get_doc_coverage():
    total = 0
    documented = 0
    try:
        # We look through all xml files except index
        for xml_file in glob.glob("xml/*.xml"):
            if xml_file.endswith("index.xml"):
                continue
            tree = ET.parse(xml_file)
            root = tree.getroot()
            for memberdef in root.findall(".//memberdef"):
                # Ignore private or undocumented members that we don't care about if needed, but let's count all.
                total += 1
                brief = memberdef.find("briefdescription")
                detail = memberdef.find("detaileddescription")

                has_doc = False
                if brief is not None and len(list(brief)) > 0:
                    has_doc = True
                if detail is not None and len(list(detail)) > 0:
                    has_doc = True

                if has_doc:
                    documented += 1

        if total == 0:
            return 100.0
        return (documented / total) * 100.0
    except Exception as e:
        print("Could not calculate doc coverage:", e)
        return None


def update_readme(test_cov, doc_cov):
    if not os.path.exists("README.md"):
        return

    with open("README.md", "r") as f:
        content = f.read()

    # Function to add shields if missing
    def ensure_shield(content, name, default_cov):
        pattern = (
            r"\[\!\[(?:%s)\]\(https://img\.shields\.io/badge/[^)]+\)\]\(#\)" % name
        )
        if not re.search(pattern, content):
            # Insert after License shield
            # License shield regex: \[!\[License\]\(https://img\.shields\.io/badge/license-[^)]+\)\]\([^)]+\)
            license_match = re.search(
                r"(\[\!\[License\]\(https://img\.shields\.io/badge/license-[^)]+\)\]\([^)]+\))",
                content,
            )
            if license_match:
                color = (
                    "green"
                    if default_cov >= 80
                    else ("yellow" if default_cov >= 60 else "red")
                )
                cov_str = f"{default_cov:.1f}%"
                safe_name = name.replace(" ", "_")
                shield = f"\n[![{name}](https://img.shields.io/badge/{safe_name}-{cov_str.replace('%', '%25')}-{color}.svg)](#)"
                content = (
                    content[: license_match.end()]
                    + shield
                    + content[license_match.end() :]
                )
        return content

    if test_cov is not None:
        content = ensure_shield(content, "Test Coverage", test_cov)
        test_cov_str = f"{test_cov:.1f}%"
        color = "green" if test_cov >= 80 else ("yellow" if test_cov >= 60 else "red")
        content = re.sub(
            r"\[\!\[Test Coverage\]\(https://img\.shields\.io/badge/Test_Coverage-[^%]+%25-[^.]+\.svg\)\]\(#\)",
            f"[![Test Coverage](https://img.shields.io/badge/Test_Coverage-{test_cov_str.replace('%', '%25')}-{color}.svg)](#)",
            content,
        )

    if doc_cov is not None:
        content = ensure_shield(content, "Doc Coverage", doc_cov)
        doc_cov_str = f"{doc_cov:.1f}%"
        color = "green" if doc_cov >= 80 else ("yellow" if doc_cov >= 60 else "red")
        content = re.sub(
            r"\[\!\[Doc Coverage\]\(https://img\.shields\.io/badge/Doc_Coverage-[^%]+%25-[^.]+\.svg\)\]\(#\)",
            f"[![Doc Coverage](https://img.shields.io/badge/Doc_Coverage-{doc_cov_str.replace('%', '%25')}-{color}.svg)](#)",
            content,
        )

    with open("README.md", "w") as f:
        f.write(content)


if __name__ == "__main__":
    # run doxygen
    os.system("doxygen Doxyfile > /dev/null 2>&1")

    # generate test coverage summary (this assumes the project is already built with coverage and tests run)
    cmd = r"cd build && gcovr -r .. --filter '\.\./src/' --filter '\.\./include/' --json-summary coverage_summary.json > /dev/null 2>&1"
    os.system(cmd)

    test_cov = get_test_coverage()
    doc_cov = get_doc_coverage()

    print(f"Test Coverage: {test_cov}%")
    print(f"Doc Coverage: {doc_cov}%")

    update_readme(test_cov, doc_cov)
