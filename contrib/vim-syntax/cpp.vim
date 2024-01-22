"******************************************************************************
"  cpp.vim
"------------------------------------------------------------------------------
"
"  Language:      C/C++
"  Maintainer:    Chris Ahlstrom <ahlstromcj@users.sourceforge.net>
"  Last Change:   2006-09-04 to 2024-01-21
"  Project:       XPC Suite library project
"  Usage:
"
"     This file is a Vim syntax add-on file used in addition to the installed
"     version of the cpp.vim file provided by vim.
"
"     This file is similar to c.vim, but for C++ code.  It adds
"     keywords and syntax highlighting useful to vim users.  Please note that
"     all of the keywords in c.vim also apply to C++ code, so that they
"     do not need to be repeated here.
"
"     Do a ":set runtimepath" command in vim to see what it has in it.  The
"     first entry is usually "~/.vim", so create that directory, then add an
"     "after" and "syntax" directory, so that you end up with this directory:
"
"              ~/.vim/after/syntax
"
"     Then copy the present file (cpp.vim) to 
"
"              ~/.vim/after/syntax/cpp.vim
"
"     Verify that your code now highlights the following symbols when edited.
"
"------------------------------------------------------------------------------

"------------------------------------------------------------------------------
" Our type definitions for new classes and types added by the XPCC++ library
"------------------------------------------------------------------------------

syn keyword XPCC midibytes midistring midi_message seq64 seq66 tokenization
syn keyword XPCC boolean booleans bpm buffer byte bytes bytestring ppqn pulse
syn keyword XPCC audio container ctrl meta seqspec status tag ulong ushort
syn keyword XPCC api api_list cfg cfg66 cli audio rtaudio
syn keyword XPCC midi rtl rtmidi rtl66
syn keyword XPCC seq seq66 session util xpc xpc66
syn keyword XPCC action clock clocking e_clock jack transport synch
syn keyword XPCC recmutex automutex

"------------------------------------------------------------------------------
" Our type definition for inside comments
"------------------------------------------------------------------------------

syn keyword cTodo contained cpp hpp CPP HPP krufty

"------------------------------------------------------------------------------
" Our Doxygen aliases to highlight inside of comments
"------------------------------------------------------------------------------

syn keyword cTodo contained constructor copyctor ctor
syn keyword cTodo contained defaultctor destructor dtor operator paop paoperator
syn keyword cTodo contained pure singleton virtual

"------------------------------------------------------------------------------
" Our type definitions that are basically standard C++
"------------------------------------------------------------------------------

syn keyword cType aggregation alias containment dependency inherits nested
syn keyword cType array atomic auto_ptr bad_alloc begin c_str
syn keyword cType cbegin cend clear const_iterator
syn keyword cType const_reverse_iterator cbegin cend rbegin rend
syn keyword cType difference_type iterator_category pointer
syn keyword cType empty end erase exception find first fstream future
syn keyword cType ifstream insert istream istringstream iterator
syn keyword cType length list make_pair map multimap
syn keyword cType ofstream ostream ostringstream pair promise reverse_iterator
syn keyword cType reference const_reference
syn keyword cType second set shared_ptr size size_type stack std string
syn keyword cType stringstream locale
syn keyword cType thread unique_ptr value_type vector wstring

"------------------------------------------------------------------------------
" Operators, language constants, or manipulators
"------------------------------------------------------------------------------

syn keyword cppOperator cin cout cerr dec endl hex left nothrow new npos
syn keyword cppOperator oct right setfill setw

"------------------------------------------------------------------------------
" Less common C data typedefs
"------------------------------------------------------------------------------

syn keyword cType my_data_t

"------------------------------------------------------------------------------
" Our slough of macros
"------------------------------------------------------------------------------

syn keyword cConstant xxxxxxx
syn keyword cDefine SCUZZGOZIO

"------------------------------------------------------------------------------
" cpp.vim
"------------------------------------------------------------------------------
" vim: ts=3 sw=3 et ft=vim
"------------------------------------------------------------------------------
