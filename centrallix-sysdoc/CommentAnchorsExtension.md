<!-------------------------------------------------------------------------->
<!-- Centrallix Application Server System                                 -->
<!-- Centrallix Core                                                      -->
<!--                                                                      -->
<!-- Copyright (C) 1998-2012 LightSys Technology Services, Inc.           -->
<!--                                                                      -->
<!-- This program is free software; you can redistribute it and/or modify -->
<!-- it under the terms of the GNU General Public License as published by -->
<!-- the Free Software Foundation; either version 2 of the License, or    -->
<!-- (at your option) any later version.                                  -->
<!--                                                                      -->
<!-- This program is distributed in the hope that it will be useful,      -->
<!-- but WITHOUT ANY WARRANTY; without even the implied warranty of       -->
<!-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        -->
<!-- GNU General Public License for more details.                         -->
<!--                                                                      -->
<!-- You should have received a copy of the GNU General Public License    -->
<!-- along with this program; if not, write to the Free Software          -->
<!-- Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA             -->
<!-- 02111-1307  USA                                                      -->
<!--                                                                      -->
<!-- A copy of the GNU General Public License has been included in this   -->
<!-- distribution in the file "COPYING".                                  -->
<!--                                                                      -->
<!-- File:        CommentAnchorsExtension.md                              -->
<!-- Author:      Israel Fuller                                           -->
<!-- Creation:    January 30th, 2026                                      -->
<!-- Description: Documentation of how to use the optional comment        -->
<!--              anchors VSCode extension to make navigating code and    -->
<!--              comments faster and easier.                             -->
<!-------------------------------------------------------------------------->

# The Comment Anchors Extension <!-- ANCHOR[id=intro] -->
A couple of files in Centrallix use the [Comment Anchors VSCode extension](https://marketplace.visualstudio.com/items?itemName=ExodiusStudios.comment-anchors) from Starlane Studios. This extension allows developers to use certain text in comments to create links to other files, or other locations within the same file. This extension is _optional_ and developing in Centrallix without it is fine. If you don't intend to use it, you can safely ignore `LINK`, `ANCHOR`, and similar comments intended for it, although please try not to break them.


## Table of Contents <!-- ANCHOR[id=contents] -->
- [The Comment Anchors Extension](#the-comment-anchors-extension)
  - [Table of Contents](#table-of-contents)
  - [Basic Features](#basic-features)
    - [Links](#links)
    - [Anchors](#anchors)
  - [Quick Warning](#quick-warning)
  - [Informing other developers](#informing-other-developers)


## Basic Features
Below is a quick summary of the primary features from the extension that are used in Centrallix. This is intended to help get developers up to speed easily without needing to sift through other features that aren't currently in use.

_For more information, visit the [extension page](https://marketplace.visualstudio.com/items?itemName=ExodiusStudios.comment-anchors)._


### Links
Create a link by writing `LINK <destination>` at the start of a line in a documentation comment. Control click the link to jump to the destination in the editor. A link might look like:
```c
/*** To read the license, click the link below:
 *** LINK LICENSE
 ***/
```
- Use `:line-number` to link to a specific line number.
- Use `#<id>` to link to an [anchor](#anchors) with that id.
- Intra-file links don't need to specify a file (e.g. `LINK #example`).

Note: Invalid or broken links to a location in a file will sometimes default to the first line of the file.


### Anchors
Create an anchor by writing `ANCHOR[id=<ID>]` at the start of a line in a documentation comment, like this:
```c
/*** ANCHOR[id=intro]
 *** This file will teach you how to never write a memory error in C ever again...
 ***/
```
Add `#ID` to the end of a link to target the anchor with that ID (useful for navigating long files, such as `objdrv_cluster.c`).


## Quick Warning
The comment anchors extension is a little less robust than I would like. Sometimes, placing characters before or after links or anchors on the same line will cause them to break. The extension is sometimes a little finicky.


## Informing other developers
When using the comment anchors extension, include something closely resembling the following snippet at the top of the file, just below the copywrite notice and include statements:

**.c or .h file**
```c
/*** This file uses the optional Comment Anchors VSCode extension, documented
 *** with CommentAnchorsExtension.md in centrallix-sysdoc.
 ***/
```

**.md file**
```md
<!-- This file uses the optional Comment Anchors VSCode extension, documented
 !-- with CommentAnchorsExtension.md in centrallix-sysdoc.
 !-->
```