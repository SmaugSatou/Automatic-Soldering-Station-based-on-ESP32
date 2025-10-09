import re
from collections import defaultdict

def parse_drl_file(filepath):
    """
    Parses an NC drill (.DRL) file and returns a structured dictionary.
    """
    with open(filepath, 'r', encoding='utf-8') as f:
        lines = [line.strip() for line in f if line.strip()]

    data = {
        "units": None,
        "tools": {},
        "holes": defaultdict(list),
    }

    current_tool = None
    coord_pattern = re.compile(r'X([-+]?[0-9]*\.?[0-9]*)Y([-+]?[0-9]*\.?[0-9]*)', re.IGNORECASE)
    tool_pattern = re.compile(r'T(\d+)(?:C([\d\.]+))?', re.IGNORECASE)

    for line in lines:
        # Skip comment lines
        if line.startswith(';'):
            continue

        # Detect unit system
        if 'METRIC' in line.upper():
            data["units"] = "mm"
        elif 'INCH' in line.upper():
            data["units"] = "inch"

        # Define tool size
        # ex. T01C0.70104
        tool = tool_pattern.match(line)
        if tool and tool.group(2):
            tool_num = f"T{tool.group(1)}"
            size = float(tool.group(2))
            data["tools"][tool_num] = size
            continue

        # Switch to tool
        # ex. T01
        if re.fullmatch(r'T\d+', line):
            current_tool = line
            continue

        # Match coordinates
        # ex. X23.25Y-11.0
        match_coord = coord_pattern.match(line)
        if match_coord and current_tool:
            x = float(match_coord.group(1))
            y = float(match_coord.group(2))
            data["holes"][current_tool].append((x, y))
            continue

        # End of file
        if line == 'M30':
            break

    return data


if __name__ == "__main__":
    parsed = parse_drl_file("Drill_PTH_Through.DRL")
    print("Units:", parsed["units"])
    print("Tools:", parsed["tools"])
    print("Holes (by tool):")
    for tool, coords in parsed["holes"].items():
        print(f"{tool} ({parsed['tools'].get(tool, '?')} mm): \n{"\n".join([f"{coord}" for coord in coords])}")
