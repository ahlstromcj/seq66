"******************************************************************************
"  syncolor.vim
"------------------------------------------------------------------------------
"
" Language:    N/A
" Maintainer:  Chris Ahlstrom <ahlstromcj@gmail.com>
" Last Change: 08/04/2006-03/05/2024
" Project:     XPCC library project
" Usage:
"
" Vim syntax-color add-on file for c.vim and cpp.vim add-ons
"
"  Do a ":set runtimepath" command in vim to see what it has in it.
"  The first entry is usually "~/.vim", so create that directory, then
"  add an "after" and "syntax" directory, so that you end up with this
"  directory:
"
"              ~/.vim/after/syntax
"
"  Then copy the present file (syncolor.vim) to 
"
"              ~/.vim/after/syntax/syncolor.vim
"
"  Verify that your code now highlights comments in dark cyan, not bright
"  cyan.
"
"  I think I prefer light green to light red for XPC C/C++ data types.
"  Let's try blue though
"
"------------------------------------------------------------------------------

if &background == "dark"

   highlight Comment cterm=NONE term=NONE ctermfg=DarkCyan
   highlight XPCC term=bold cterm=bold ctermfg=LightBlue ctermbg=NONE gui=NONE guifg=Orange guibg=NONE
   highlight tjspkeyword term=bold cterm=bold ctermfg=LightGreen ctermbg=NONE gui=NONE guifg=Orange guibg=NONE
   highlight todo term=standout cterm=NONE ctermfg=DarkGreen ctermbg=NONE gui=NONE guifg=DarkGreen guibg=Yellow

else

   highlight Comment cterm=NONE term=NONE ctermfg=DarkBlue
   highlight XPCC term=bold cterm=bold ctermfg=DarkBlue ctermbg=NONE gui=NONE guifg=SlateBlue guibg=NONE
   highlight tjspkeyword term=bold cterm=bold ctermfg=DarkGreen ctermbg=NONE gui=NONE guifg=SlateBlue guibg=NONE
   highlight todo term=standout cterm=NONE ctermfg=DarkGreen ctermbg=NONE gui=NONE guifg=DarkGreen guibg=Yellow

endif

   highlight ERR term=bold cterm=bold ctermfg=Red ctermbg=NONE gui=NONE guifg=Red guibg=NONE

"------------------------------------------------------------------------------
" syncolor.vim
"------------------------------------------------------------------------------
" vim: ts=3 sw=3 et ft=vim
"------------------------------------------------------------------------------
