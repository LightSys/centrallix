Title:    vim tips for following the Coding Standards for the Centrallix project
Author:   Derrick Hudson
Date:     18-July-2002
License:  Copyright (C) 2002 LightSys Technology Services.  See LICENSE.txt.
-------------------------------------------------------------------------------

OVERVIEW

    This document describes some vim configuration options to make working with
    the coding style less painful ;-).


VIM OPTIONS

    Read the vim manual for a detailed explanation of what these options mean.
    For every command and option you can type ':help <option>' (where <option>
    is the name of the command or option).


    " This sets "default" options for all C files you edit.
     augroup C
        au!
        au BufNewFile *.c 0read ~/util/templates/C.c
        au FileType c set ai si sts=4 sw=4 et tw=78 fo=croq2 fenc=us-ascii
        " re-enable preprocessor autoindenting (disabled elsewhere in my .vimrc)
        au FileType c inoremap # #
    augroup END

    " This sets options specific to centrallix, which may be different from
    " other C projects you work on.  Obviously adjust the path for your system.
    augroup Centrallix
        au!
        au BufRead ~/projects/oss/centrallix/cvs/* set cinoptions=f1s{1s
        au BufRead ~/projects/oss/centrallix/cvs/* map [[ ?{<CR>w99[{
        au BufRead ~/projects/oss/centrallix/cvs/* map ]] j0[[%/{<CR>
        au BufRead ~/projects/oss/centrallix/cvs/* map ][ /}<CR>b99]}
        au BufRead ~/projects/oss/centrallix/cvs/* map [] k$][%?}<CR>
    augroup END

-------------------------------------------------------------------------------
