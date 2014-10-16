set tabstop=4
set softtabstop=4
set shiftwidth=4
set expandtab

set colorcolumn=110
highlight ColorColumn ctermbg=darkgray

augroup project
    autocmd!
    autocmd BufRead,BufNewFile *.h,*.c set filetype=c.doxygen
augroup END

let &path.="src/include,/usr/include/AL,"

" for include rename (like for java)
" set includeexpr=substitute(v:fname,'\\.','/','g')

set makeprg=make\ -C\ ../build/\ -j9
"nnoremap <F4> :make!<cr>
nnoremap <silent> <F4> :w<CR>:!make<CR>:!./"%:t:r"<CR><CR>
"nnoremap <silent> <F4> :w<CR>:Shell make -C build -j9<CR>:!./"%:t:r"<CR><CR>
