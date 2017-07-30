;;; oak-mode.el --- major mode for editing oak source files

;; Copyright (c) 2017 "KrokodileGlue"

;; Author: KrokodileGlue <KrokodileGlue@outlook.com>
;; Created: 30 Jul 2017
;; Keywords: languages oak
;; Homepage: https://github.com/KrokodileGlue/oak

;; This file is not part of GNU Emacs.

;; Permission is hereby granted, free of charge, to any person obtaining a copy
;; of this software and associated documentation files (the "Software"), to deal
;; in the Software without restriction, including without limitation the rights
;; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
;; copies of the Software, and to permit persons to whom the Software is
;; furnished to do so, subject to the following conditions:

;; The above copyright notice and this permission notice shall be included in all
;; copies or substantial portions of the Software.

;; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
;; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
;; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
;; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
;; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
;; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
;; SOFTWARE.

;;; Commentary:
;; This package provides a major mode for the syntax highlighting, indentation,
;; and code formatting of the oak programming language.

;;; Code:

(defvar oak-highlights nil "Define the font-faces for the functions, constants, and operators of `oak-mode'.")
(setq oak-highlights
	'(("type\\|sayln\\|say" . font-lock-function-name-face)
	 ("pi" . font-lock-constant-face)))

(font-lock-add-keywords 'oak-mode
	'(("\\<\\(println\\|print\\|type\\|sayln\\|say\\|for\\|while\\|do\\|fn\\)\\>" . font-lock-keyword-face)))

(defun oak-indent-line ()
	"Indent a line of oak code."
	(interactive)
	(beginning-of-line))

(defvar oak-mode-syntax-table nil "Define the syntax table for `oak-mode'.")
(setq oak-mode-syntax-table
	(let ( (syn-table (make-syntax-table)))
	 (modify-syntax-entry ?# "<" syn-table)
	 (modify-syntax-entry ?\/ "> 12b" syn-table)
	 (modify-syntax-entry ?\n "> b" syn-table)
	syn-table))

(define-derived-mode oak-mode fundamental-mode "oak"
	"Major mode for editing oak source files."
	(setq font-lock-defaults '(oak-highlights)))

(provide 'oak-mode)

;;; oak-mode.el ends here
