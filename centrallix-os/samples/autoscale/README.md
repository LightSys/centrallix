# Autoscale visual test harness

A set of pages for eyeballing Centrallix's responsive/autoscaling behaviour.

Centrallix normally lays a page out **on the server**, at whatever size the
browser reported when the page was requested. Resizing the window afterwards is
handled entirely by client-side code working from the CSS the server emitted.
This harness exists to make the difference between those two visible.

Open **`/samples/autoscale/index.app`** and use the nav strip at the top.

Everything runs off the CSV files in this directory, so no database is needed.

## The pages

| Page | What it covers |
|---|---|
| `index.app` | Pure-geometry references: concentric rings, fixed-size edge markers, a centering cross. Plus links to existing sample apps. |
| `layout.app` | The layout engine itself, built only from panes: equal grid, flex ladder, spacer threshold, minimum width, hbox/vbox/autolayout, pane border styles. |
| `containers.app` | Tab controls (three strip positions), scrollpane, scrollbars, component instance, childwindow variants. |
| `forms.app` | A working form bound to `rows.csv`, plus dropdowns, checkboxes, datetime, textarea, radio panel, hints, file upload. |
| `data.app` | Table, treeview, two charts, and a server-expanded repeat. |
| `misc.app` | Labels, buttons, images, inline HTML, menus, and live clock/timer/alerter widgets. |
| `legacy.app` | Quarantined legacy and low-use widgets: multiscroll, terminal, map, objcanvas, calendar. Failures here are low priority by design. Designed at 1000x900, not 1000x700. |

## How to use it

1. **Drag the window edge slowly.** Watch for anything that jumps, overlaps,
   clips, or stops lining up with its neighbors.

2. **Separate client-side scaling from server-side layout.** This is the main
   workflow, and it is what the "Render at" row in the nav strip is for:

   - Click a fixed size, say `800x600`. The server lays the page out as though
     the browser were 800x600, and the client-side code then scales that to
     your actual window.
   - Resize the window to roughly some other size.
   - Click `browser`. The server now lays the page out at your real window size.

   The two results should look the same. **Where they differ, the client-side
   scaling is wrong** — the server render is the reference.

3. **`design` renders at the authored size** (1000x700) and ignores the browser
   entirely. That is the layout as it was drawn, and the baseline for what
   everything is supposed to look like.

### Reading the geometry note

Every page carries the same two lines at the top right:

```
server render: 1024 x 768
design size: 1000 x 700
```

`server render` is the geometry the server was handed for this render, decoded
out of the `cx__geom` URL parameter. This matters because **`cx__geom` is
stripped from the address bar once the page loads**, so after the fact the URL
tells you nothing about which size was used. The note is the only readout.

It has to come from the URL rather than from the page widget itself: a
`runserver()` expression is evaluated while the widget tree is being parsed,
before `wgtrVerify()` runs the layout, and the server-side evaluation context
holds only `this` (the `widget/parameter` list), so `:pagename:width` binds to
NULL and the whole label comes out blank.

It deliberately does **not** track window resizing. Comparing it against your
current window size tells you how far the client-side code has had to stretch
the layout, which is exactly the quantity that makes scaling bugs appear.

### Fixed geometries

`cx__geom` is 20 hex digits: width, height, char width, char height, paragraph
height — four digits each. The char metrics used throughout are `7/16/16`,
which are the same defaults the server applies for `cx__geom=design`, so all
the fixed renders stay directly comparable. Omitting `cx__geom` makes the
server ask the browser to measure itself, which is the normal path.

You can hit any of these directly, e.g.
`/samples/autoscale/layout.app?cx__geom=04000300000700100010` for 1024x768.

## Reading the test content

All long strings are filler and there is nothing to read in them. They are
shaped so you can measure with them:

- `0....5...10...15...` is a character ruler. Count to the point where it stops
  to find exactly where a widget clips.
- `FILLER-Rnn ....... END-Rnn`, `ROW nn ... END-nn`, `EVT PLUSnn` all carry
  their own row number at both ends, so a truncated or misaligned row is
  obvious and identifiable.
- Small coloured squares in container corners are fixed-size markers
  (`fl_width=0; fl_height=0`). They must stay the same pixel size and stay
  pinned to their corner at every window size. They show where a container
  thinks its client area is.
- The red bar at the far right of each page, and the one at the right of the
  nav strip, mark the page's and the component's right edges respectively.

## Thresholds worth knowing

From `centrallix/include/apos.h`, and probed directly on `layout.app`:

- `APOS_MINSPACE` (20) — a gap of 20px or less between two widgets is treated
  as a spacer and pinned at flex 0, so it must never grow. 21px is the first
  gap allowed to grow.
- `APOS_MINWIDTH` (30) — the narrowest a widget may shrink to.

Each widget driver also sets its own default `fl_width`/`fl_height` when the
structure file does not (pane 100/100, label 1/0, dropdown 10/1, textbutton
5/1, treeview 30/50, and so on), so most of the harness leaves them unset in
order to test those defaults, and sets them explicitly only where a test needs
a specific value.

Note that **`fl_x` and `fl_y` do nothing** — the layout engine reads only
`fl_width` and `fl_height`.

## Data files

| File | Used by |
|---|---|
| `rows.csv` / `.spec` | The bound form, the table, and the treeview's leaf labels. 48 rows. |
| `series.csv` / `.spec` | The charts. Three deliberately simple shapes: a straight rise, a straight fall, and one triangular bump — so a chart that redraws wrongly stops looking like them. |
| `bands.csv` / `.spec` | The treeview's middle level and the repeat widget. |
| `events.csv` / `.spec` | The calendar. Stores **day offsets, not dates**; the query turns them into real dates with `dateadd('day', :offset, getdate())` so there is always something in the current month. |
| `tree.qyt` | The treeview source. Three branches: a real query tree over the CSVs, an eight-deep static nest, and a set of short labels that must *not* truncate. |
| `ruler.shl` | Terminal backend on the legacy page. Prints a ruler grid and exits. |

## Known non-bugs

Things that look wrong but are not caused by scaling:

- **`textbutton` ignores `font_size` entirely.** Its driver never reads the
  property, so button text never scales. (`tab_features.app` sets it too, with
  no effect.)
- **`checkbox` is always 13x13** and **`formbar` is always 240x30**, hardcoded
  in their drivers regardless of what the structure file says.
- **`fileupload` reads no geometry at all**, so it flows inline wherever it
  lands rather than sitting where it was placed.
- **`widget/menu` registers no `Select` event server-side**, so menu clicks on
  the misc page are not wired to anything.
- **The alerter discards the `Confirm` result**, so only the dialog itself is
  testable.
- **`widget/table` has no `mode` property.** Nothing in `htdrv_table.c` or
  `wgtdrv_table.c` reads one, despite `widgets.xml` showing `mode="static"` in
  its example. The real property is `data_mode` (`rows` / `properties`).
- **The calendar's `minpriority` does nothing.** `htdrv_calendar.c:136` reads
  the `displaymode` *string* into the `int minpriority` variable — so priority
  filtering never works, and the read writes a pointer-sized value into an int.
  This dates from 2004 (commit `2505c6e1`), long before this branch, and is why
  the calendar is quarantined on the Legacy page. It is not a scaling bug.
- Structure files accept `//` comments only — `/* */` is a parse error.

## Widgets not included

`widget/spinner` (requires Netscape 4 DOM; errors out on any current browser),
`widget/window` (opens a separate browser window and registers no actions, so
nothing can trigger it), `widget/frameset` (renders each frame as its own
document), and `widget/execmethod` (CSV objects expose no methods).

Also absent, because they are nonvisual or declarative and so have no geometry
to scale: `widget/rule`, `widget/variable`, `widget/template`, `widget/hints`
(exercised indirectly on the forms page), `widget/parameter` and
`widget/connector` (used throughout, but never as visible widgets).

The reasons are repeated on the Legacy page.
