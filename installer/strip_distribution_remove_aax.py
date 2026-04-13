#!/usr/bin/env python3
"""Remove AAX choice and pkg-ref from distribution.xml when AAX component is not built."""
import sys
import xml.etree.ElementTree as ET


def main() -> None:
    if len(sys.argv) != 3:
        print("Usage: strip_distribution_remove_aax.py <src.xml> <dst.xml>", file=sys.stderr)
        sys.exit(2)
    src, dst = sys.argv[1], sys.argv[2]
    tree = ET.parse(src)
    root = tree.getroot()
    for outline in root.findall("choices-outline"):
        for line in list(outline):
            if line.get("choice") == "aax":
                outline.remove(line)
    for choice in list(root.findall("choice")):
        if choice.get("id") == "aax":
            root.remove(choice)
    for pref in list(root.findall("pkg-ref")):
        if pref.get("id") == "com.kaizenstrategicai.choroboros.aax":
            root.remove(pref)
    tree.write(dst, encoding="UTF-8", xml_declaration=True)


if __name__ == "__main__":
    main()
