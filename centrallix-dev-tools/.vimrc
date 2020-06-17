" get syntax highlighting
     let mysyntaxfile = "/usr/share/vim/vim70/syntax/cx.vim"
     syntax on

au BufRead,BufNewFile *.cmp set filetype=cx
au! Syntax cx source /usr/share/vim/vim70/syntax/cx.vim

