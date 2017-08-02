" Vim syntax file
" Language: oak
" Maintainer: KrokodileGlue <https://github.com/KrokodileGlue/>
" Latest Revision: 30 July 2017

if exists("b:current_syntax")
	finish
endif

syn keyword oak_keywords class fn for if while do var
syn match   oak_number '\d\+'
syn region  oak_string start="\"" end="\""
syn region  oak_interp_string start="'" end="'"
syn region  oak_region start='{' end='}' fold transparent contains=oak_keywords
syn keyword oak_todo TODO FIXME HACK NOTE VOLATILE XXX
syn match   oak_comment '#.*$' contains=oak_todo

let b:current_syntax = "oak"

hi def link oak_todo          Todo
hi def link oak_comment       Comment
hi def link oak_string        Constant
hi def link oak_interp_string Constant
hi def link oak_number        Constant
hi def link oak_keywords      Statement
