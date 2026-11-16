#!/usr/bin/env python3
# build.py - Generate config.json from the Markdown rules in rules/.
#
# Each rules/<id>.md file holds one rule. Its file name is the rule id, and
# its front matter lists the globs the rule applies to:
#
#   ---
#   scope:
#     - "**/*.c"
#   ---
#   Rule text in Markdown.
#
# Run after editing any rule: python3 .greptile/build.py
# With --check, exit with status 1 if config.json is out of date.

import json
import os
import sys

here = os.path.dirname(os.path.abspath(__file__))


# parse_rule - Split a rule file into its scope globs and rule text.
def parse_rule(path):
    with open(path) as f:
        text = f.read()

    # Separate the front matter from the body.
    if not text.startswith("---\n"):
        sys.exit("Failed to parse '%s': missing front matter." % path)
    head, sep, body = text[4:].partition("\n---\n")
    if not sep:
        sys.exit("Failed to parse '%s': unterminated front matter." % path)

    # Collect the scope list items.
    scope = []
    for line in head.splitlines():
        line = line.strip()
        if line.startswith("- "):
            scope.append(line[2:].strip().strip('"'))
        elif line != "scope:":
            sys.exit("Failed to parse '%s': unexpected front matter line '%s'." % (path, line))
    if not scope:
        sys.exit("Failed to parse '%s': no scope globs." % path)

    return scope, body.strip("\n")


def main():
    if sys.argv[1:] not in ([], ["--check"]):
        sys.exit("Usage: %s [--check]" % sys.argv[0])
    rules_dir = os.path.join(here, "rules")
    rules = []

    # Build one rule per Markdown file.
    for name in sorted(os.listdir(rules_dir)):
        if not name.endswith(".md"):
            continue
        scope, rule = parse_rule(os.path.join(rules_dir, name))
        rules.append({"id": name[:-3], "rule": rule, "scope": scope})

    config = json.dumps({"rules": rules}, indent=2, ensure_ascii=False) + "\n"
    config_path = os.path.join(here, "config.json")

    # Compare against the committed config.
    if sys.argv[1:] == ["--check"]:
        with open(config_path) as f:
            if f.read() != config:
                sys.exit("Failed check: '%s' is out of date; run python3 .greptile/build.py." % config_path)
        return

    # Write the config Greptile reads.
    with open(config_path, "w") as f:
        f.write(config)


main()
