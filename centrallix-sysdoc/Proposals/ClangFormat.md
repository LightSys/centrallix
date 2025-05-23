# clang-format in Centrallix
Author: Alison Blomenberg

Date: May 2022

## Rationale
Centrallix currently uses a mix of code styles in different files, sometimes within the same file (for instance, mixed tabs and spaces). This can distract from the logic of the code and make it difficult to understand diffs on GitHub and other platforms. In a project that involves so many short-term contributors, ease of understanding and contributing code is especially important.

clang-format is a tool for automatically formatting C code (and code in a few other languages too, including JavaScript) that was created by the LLVM project and is used by many large C projects, including the [Linux kernel](https://github.com/torvalds/linux/blob/master/.clang-format). You can run it and automatically format files just on the command line, but since it's so widely used, there are also many integrations for editors like VSCode and continuous integration systems to confirm pushed code is formatted properly. Wherever you run it from, clang-format uses a file called `.clang-format` to define the code style to use. One `.clang-format` in the root of a repository can define the styling to use for the whole codebase, for multiple tools.

Using clang-format to 1) reformat all existing Centrallix C and JavaScript code, and then 2) enforce styling on new code that's pushed to GitHub would be a comparatively easy way to remove some barriers to understanding and working with Centrallix code. The main questions/challenges with such a project are:

1. What `.clang-format` style options to use
2. How to initially reformat the repository in a way that won't make Git blames unusable

## Choosing a .clang-format file
Different versions of clang-format support different style options. Here is a sample `.clang-format` for C and JavaScript which works with newer versions of clang-format, such as the one bundled with the VSCode C extension. It's adapted from the centrallix-sysdoc/BeeleyCodingStyle.md docs.

```
---
Language: Cpp
BasedOnStyle: LLVM
ColumnLimit: 120
MaxEmptyLinesToKeep: 10
UseCRLF: false
UseTab: Never
IndentWidth: 4
BreakBeforeBraces: Linux
AlwaysBreakAfterReturnType: All
PointerAlignment: Left
IndentCaseLabels: true
SortIncludes: Never
IndentGotoLabels: false
```

However, this `.clang-format` doesn't work with the older version of clang-format available in repositories for the version of CentOS LightSys VMs are currently running on. Here's a sample `.clang-format` which does work on this older version, but doesn't adhere to the Beeley coding style specification:

```
---
BasedOnStyle: WebKit
ColumnLimit: 120
MaxEmptyLinesToKeep: 10
UseTab: Never
IndentWidth: 4
BreakBeforeBraces: Linux
IndentCaseLabels: true
```

These are just suggestions for a `.clang-format` that could be changed before doing a major reformat. However, once a significant reformat is done, we should stick to one `.clang-format`.

## Applying reformatting to repository
The classic way to make a large change like this to an entire Git repository is to use [git filter-branch](https://git-scm.com/docs/git-filter-branch), which has now been superseded by [git filter-repo](https://github.com/newren/git-filter-repo). This essentially edits all of history so it looks like the files were *always* formatted properly.

Here are some specific examples I found of using filter-repo for a reformat like this:
- https://github.com/newren/git-filter-repo/blob/main/Documentation/converting-from-filter-branch.md#cheat-sheet-additional-conversion-examples
- https://github.com/newren/git-filter-repo/blob/main/contrib/filter-repo-demos/lint-history

However, a filter-repo is a major, slow operation, especially on a repository with as much history as Centrallix. The reformatting command needs to run (and run correctly) on *all* versions of *all* files in history. One potential alternative is to use an ignore revs file. [Newer versions of git support passing a file to `git blame` with a list of revisions to ignore](https://git-scm.com/docs/git-blame#Documentation/git-blame.txt---ignore-revs-fileltfilegt), and [GitHub just recently started automatically doing this in its web blame view if you have a file called `.git-blame-ignore-revs` in your repo](https://docs.github.com/en/repositories/working-with-files/using-files/viewing-a-file#ignore-commits-in-the-blame-view).

In this case, you would just have to do one reformat of all current files in one commit, then put the commit hash in a file called `.git-blame-ignore-revs`. GitHub would automatically ignore the reformatting commit when doing a blame, and if you ran a blame yourself from the command line, you could include the `--ignore-revs-file` option with the path to that file.
