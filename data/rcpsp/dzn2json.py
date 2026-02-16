import json
import re
import sys
import os


def parse_int_list(text):
    return [int(x) for x in re.findall(r"-?\d+", text)]


def parse_rr(block):
    rows = []
    for line in block.strip().split("|"):
        line = line.strip()
        if line:
            rows.append(parse_int_list(line))
    return rows


def parse_suc(block):
    groups = []
    sets = re.findall(r"\{([^}]*)\}", block)
    for s in sets:
        if s.strip() == "":
            groups.append([])
        else:
            groups.append([int(x) - 1 for x in re.findall(r"\d+", s)])
    return groups


def dzn_to_dict(dzn_text):
    data = {}

    data["n_res"] = int(re.search(r"n_res\s*=\s*(\d+)", dzn_text).group(1))
    data["n_tasks"] = int(re.search(r"n_tasks\s*=\s*(\d+)", dzn_text).group(1))

    data["rc"] = parse_int_list(
        re.search(r"rc\s*=\s*\[(.*?)\];", dzn_text, re.S).group(1)
    )

    data["d"] = parse_int_list(
        re.search(r"d\s*=\s*\[(.*?)\];", dzn_text, re.S).group(1)
    )

    rr_block = re.search(r"rr\s*=\s*\[\|(.*?)\|\];", dzn_text, re.S).group(1)
    data["rr"] = parse_rr(rr_block)

    suc_block = re.search(r"suc\s*=\s*\[(.*?)\];", dzn_text, re.S).group(1)
    data["suc"] = parse_suc(suc_block)

    return data


def compact_inner_lists(match):
    content = match.group(0)
    content = re.sub(r"\s+", " ", content)
    return content


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python dzn2json.py input.dzn")
        sys.exit(1)

    input_path = sys.argv[1]
    base, _ = os.path.splitext(input_path)
    output_path = base + ".json"

    with open(input_path, "r") as f:
        dzn_text = f.read()

    data = dzn_to_dict(dzn_text)

    # Pretty print JSON
    output = json.dumps(data, indent=4, ensure_ascii=False)

    # Compact ONLY innermost lists
    output = re.sub(
        r"\[\s*(?:-?\d+(?:\s*,\s*-?\d+)*)?\s*\]",
        compact_inner_lists,
        output
    )

    with open(output_path, "w") as f:
        f.write(output)

    print(f"JSON saved to {os.path.abspath(output_path)}")
