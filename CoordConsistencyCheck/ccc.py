# Check the diffs in a HighwayData repository to ensure that if any points
# have been relocated, any other files that contain the same point
# have also been updated accordingly
#
# Jim Teresco, assisted by Gemini, 8/16/2026

import argparse
import os
import re
import subprocess
import sys
from collections import defaultdict

# Matches OSM-style URLs containing lat/lon query parameters
URL_PATTERN = re.compile(
    r"https?://(?:www\.)?openstreetmap\.org/\?lat=(-?\d+\.\d+)&lon=(-?\d+\.\d+)"
)


def run_cmd(cmd, cwd=None):
    """Executes a shell command and returns stdout."""
    result = subprocess.run(
        cmd, capture_output=True, text=True, check=True, cwd=cwd
    )
    return result.stdout


def parse_wpt_content(text):
    """Parses .wpt content preserving exact 1-based line numbers.

    Returns:
        tuple: (list_of_records, set_of_coord_tuples)
    """
    records = []
    coords_set = set()

    lines = text.splitlines()
    for idx, line in enumerate(lines, start=1):
        raw_line = line.strip()
        if not raw_line:
            continue
        match = URL_PATTERN.search(raw_line)
        if match:
            lat, lon = match.group(1), match.group(2)
            labels_part = raw_line[: match.start()].strip()
            primary_label = labels_part.split()[0] if labels_part else "UNLABELED"

            records.append(
                {
                    "primary_label": primary_label,
                    "full_labels": labels_part,
                    "lat": lat,
                    "lon": lon,
                    "raw_line": raw_line,
                    "line_num": idx,
                }
            )
            coords_set.add((lat, lon))

    return records, coords_set


def read_file_safe(filepath):
    """Safely reads file content handling potential encoding issues."""
    try:
        with open(filepath, "r", encoding="utf-8") as f:
            return f.read()
    except UnicodeDecodeError:
        with open(filepath, "r", encoding="latin-1", errors="replace") as f:
            return f.read()


def write_file_safe(filepath, content):
    """Safely writes content back to a file using utf-8."""
    with open(filepath, "w", encoding="utf-8", newline="\n") as f:
        f.write(content)


def get_all_repository_points(real_data_dir):
    """Scans all working tree .wpt files under real_data_dir and indexes points by coordinate."""
    coord_to_files = defaultdict(list)

    for root, _, files in os.walk(real_data_dir):
        if "/." in root or "\\." in root:
            continue
        for file in files:
            if file.endswith(".wpt"):
                filepath = os.path.join(root, file)
                try:
                    content = read_file_safe(filepath)
                    records, _ = parse_wpt_content(content)
                    for rec in records:
                        coord_to_files[(rec["lat"], rec["lon"])].append(
                            (filepath, rec)
                        )
                except Exception as e:
                    print(f"Error reading {filepath}: {e}")

    return coord_to_files


def determine_git_target(explicit_target=None):
    """Determines what git reference to compare against (HEAD vs base branch)."""
    if explicit_target:
        return explicit_target

    base_ref = os.environ.get("GITHUB_BASE_REF")
    if base_ref:
        return f"origin/{base_ref}"

    return "HEAD"


def pair_coordinate_changes(target_records, working_records, removed_coords, added_coords):
    """Accurately pairs old removed coordinates with their updated replacement coordinates."""
    pairs = []
    removed_recs = [r for r in target_records if (r["lat"], r["lon"]) in removed_coords]
    added_recs = [r for r in working_records if (r["lat"], r["lon"]) in added_coords]

    # Strategy 1: Match by primary label
    added_by_label = defaultdict(list)
    for ar in added_recs:
        added_by_label[ar["primary_label"]].append(ar)

    unmatched_removed = []

    for rr in removed_recs:
        label = rr["primary_label"]
        if label in added_by_label and added_by_label[label]:
            ar = added_by_label[label].pop(0)
            pairs.append((rr, ar))
        else:
            unmatched_removed.append(rr)

    # Strategy 2: Fallback to positional sequence match for remaining edits
    remaining_added = [ar for recs in added_by_label.values() for ar in recs]
    for rr, ar in zip(unmatched_removed, remaining_added):
        pairs.append((rr, ar))

    return pairs


def apply_fix_to_file(filepath, line_num, old_line, new_line):
    """Replaces a specific line in a .wpt file with the updated coordinate line."""
    content = read_file_safe(filepath)
    lines = content.splitlines(keepends=True)

    if 1 <= line_num <= len(lines):
        target_line = lines[line_num - 1]
        # Preserve original line ending (\n or \r\n)
        ending = "\r\n" if target_line.endswith("\r\n") else "\n"
        if old_line in target_line.strip():
            lines[line_num - 1] = new_line + ending
            write_file_safe(filepath, "".join(lines))
            return True
    return False


def main():
    parser = argparse.ArgumentParser(
        description="Verify consistent coordinate updates across .wpt files."
    )
    parser.add_argument(
        "--target",
        "-t",
        help="Git ref to compare against (default: HEAD locally, origin/$GITHUB_BASE_REF in CI)",
    )
    parser.add_argument(
        "--fix",
        "-f",
        action="store_true",
        help="Automatically update un-updated files in place with corrected lines.",
    )
    args = parser.parse_args()

    data_entry = "data"
    if not os.path.exists(data_entry):
        print(
            "ERROR: 'data' directory or symlink not found in the current working directory."
        )
        print(
            "Please 'cd' to the root of the repository containing the 'data' directory/symlink and run the script from there."
        )
        sys.exit(1)

    real_data_path = os.path.realpath(data_entry)
    real_data_dir = os.path.basename(real_data_path)

    git_target = determine_git_target(args.target)
    is_ci = "GITHUB_ACTIONS" in os.environ

    print(f"Resolved data path: {real_data_dir}")
    print(f"Comparing against Git target: {git_target}")

    # 1. Check git diff for modified .wpt files under the resolved directory
    git_pattern = f"{real_data_dir}/**/*.wpt"
    try:
        diff_cmd = ["git", "diff", git_target, "--", git_pattern]
        diff_output = run_cmd(diff_cmd)
    except subprocess.CalledProcessError:
        print(
            f"Error running git diff against '{git_target}'. Ensure you are in a git repository."
        )
        sys.exit(1)

    if not diff_output.strip():
        print(f"No .wpt file changes detected under '{real_data_dir}/'.")
        sys.exit(0)

    # 2. Get list of modified .wpt files
    modified_files = run_cmd(
        ["git", "diff", "--name-only", git_target, "--", git_pattern]
    ).splitlines()

    # 3. Track coordinate shifts with accurate pairing
    coord_shifts = defaultdict(list)

    for file in modified_files:
        if not os.path.exists(file):
            continue

        try:
            target_content = run_cmd(["git", "show", f"{git_target}:{file}"])
        except subprocess.CalledProcessError:
            continue

        working_content = read_file_safe(file)

        target_records, target_coords = parse_wpt_content(target_content)
        working_records, working_coords = parse_wpt_content(working_content)

        removed_coords = target_coords - working_coords
        added_coords = working_coords - target_coords

        if removed_coords and added_coords:
            matched_pairs = pair_coordinate_changes(
                target_records, working_records, removed_coords, added_coords
            )
            for old_rec, new_rec in matched_pairs:
                old_coord = (old_rec["lat"], old_rec["lon"])
                new_coord = (new_rec["lat"], new_rec["lon"])
                coord_shifts[old_coord].append(
                    {
                        "file": file,
                        "label": new_rec["primary_label"],
                        "line_num": new_rec["line_num"],
                        "old_coord": old_coord,
                        "new_coord": new_coord,
                    }
                )

    if not coord_shifts:
        print("No coordinate changes detected in modified files.")
        sys.exit(0)

    # 4. Scan working tree under real_data_dir for remaining instances of old coordinates
    print(f"Indexing repository coordinates under '{real_data_dir}/'...")
    repo_point_map = get_all_repository_points(real_data_dir)

    errors_found = False
    fixed_count = 0

    for old_coord, shifts in coord_shifts.items():
        matches = repo_point_map.get(old_coord, [])

        if matches:
            errors_found = True
            old_lat, old_lon = old_coord

            shift = shifts[0]
            origin_file = shift["file"]
            label = shift["label"]
            line_num = shift["line_num"]
            new_lat, new_lon = shift["new_coord"]

            print("\n" + "=" * 70)
            print("INCONSISTENT COORDINATE UPDATE DETECTED:")
            print(f"  Source Change  : {origin_file} (Line {line_num}, Label: {label})")
            print(f"  Old Coordinates: lat={old_lat}, lon={old_lon}")
            print(f"  New Coordinates: lat={new_lat}, lon={new_lon}")
            print("-" * 70)
            print("The un-updated old coordinate STILL EXISTS in these files:")

            for filepath, rec in matches:
                corrected_line = f"{rec['full_labels']} http://www.openstreetmap.org/?lat={new_lat}&lon={new_lon}"

                print(f"\n  File (Line {rec['line_num']}): {filepath}")
                print(f"  Current line   : {rec['raw_line']}")
                print(f"  Corrected line : {corrected_line}")

                if args.fix:
                    if apply_fix_to_file(
                        filepath, rec["line_num"], rec["raw_line"], corrected_line
                    ):
                        print(f"  --> FIXED: Updated {filepath}")
                        fixed_count += 1
                    else:
                        print(f"  --> ERROR: Could not apply fix to {filepath}")

                if is_ci:
                    print(
                        f"::error file={filepath},line={rec['line_num']}::Inconsistent update for waypoint '{label}'. Old ({old_lat}, {old_lon}) updated at {origin_file}:{line_num} to ({new_lat}, {new_lon}). Correct line: {corrected_line}"
                    )

    if not errors_found:
        print(
            "\nSUCCESS: All coordinate updates were applied consistently across all route files."
        )
    elif args.fix:
        print(
            f"\nAUTO-FIX APPLIED: Corrected {fixed_count} file(s). Re-run the script to verify."
        )
        sys.exit(0)
    else:
        print(
            "\nFAILURE: Found route files sharing identical coordinates where updates were not applied uniformly."
        )
        print("Tip: Re-run with '--fix' or '-f' to automatically update these files.")
        sys.exit(1)


if __name__ == "__main__":
    main()
