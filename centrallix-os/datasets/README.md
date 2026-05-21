# Datasets Folder

The datasets folder is intended for storing datasets used in testing that must not be committed to GitHub.  Datasets in this folder may contain sensitive data, including production data or patterns from production data.  This data _must_ be treated with care, even if it is obfuscated.  Thus, this directory should be "git-ignored".  If you ever see files in this directory listed as changes when you run `git status` or use similar commands or tools, imedately remove these changes and investigate.   Data in this directory must _never_ be committed or pushed!

## Modifying this README.md
If you need to modify this readme file, you may need to pass `-f` when adding it because the directory is part of the `.gitignore` (e.g. `git add -f README.md`).  Obviously, be careful _not to `git add` any other files_ in this directory.
