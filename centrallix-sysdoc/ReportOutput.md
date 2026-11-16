# Report output formats

Author:	    Israel Fuller

Date:	    November 13, 2026

## Overview
A report's content goes through a prtmgmt formatter to an output driver, picked by the requested `document_format`:

| Format | Content type | Formatter | Output driver |
|---|---|---|---|
| PDF | `application/pdf` | strict | PostScript, piped through `ps2pdf` |
| PNG | `image/png` | strict | PostScript, piped through `gs` (first page only) |
| HTML | `text/html`, `text/vnd.cx.paginated+html` | HTML | none |
| CSV | `text/csv` | CSV | none |
| Text | `text/plain` | strict | text |
| PostScript | `application/postscript` | strict | PostScript |
| PCL, Epson FX | `text/x-hp-pcl`, `text/x-epson-escp-24` | strict | PCL, Epson FX |

## Which formats matter
- **PDF** is by far the most used format, and the default in Kardia's report windows.
- **HTML** is for viewing reports in the browser and for email.
- **CSV and text** are for exporting data.
- **PostScript, PCL, and Epson FX** are rarely requested directly.

The PostScript driver matters because PDF and PNG are built from its output, not because users download PostScript. Judge changes to it by PDF and PNG output, measured before and after, not by the size of the PostScript.

## Ghostscript
Ghostscript reads `-` one byte per `read()` call, which makes large inputs very slow. Give it `/dev/stdin` instead, through `cat |`, because the driver writes to it over a socket, which `/dev/stdin` cannot open.

## Known issues
- **Charts make large PostScript files.** The PostScript driver writes images as uncompressed hex, 6 bytes per pixel, so `samples/chart.rpt` is 21 MB at 300 dpi and 335 MB at 1200 dpi. PDF and PNG files stay small. This is not fixed because direct PostScript is rarely used. PR #253 compressed these images (50 to 145 times smaller) and was closed as not worth the review; reopen it and restore its `postscript-compression` branch if large PostScript files become a problem.
