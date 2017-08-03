;;; oak-mode.el --- major mode for editing oak source files.

;; Version: "0.0.1"
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

(defvar oak-highlights nil "Define the font-faces for the functions, constants, and keywords of `oak-mode'.")
(setq oak-highlights
      '(("type\\|sayln\\|say"			 .	font-lock-function-name-face)
	("pi\\|0x[[:xdigit:]]+\\|[\\.[:digit:]]+".	font-lock-constant-face)
	("println\\|print\\|for\\|while\\|if"    .	font-lock-keyword-face)
	("var\\|class\\|fn"			 .	font-lock-type-face)))

(defvar oak-mode-syntax-table nil "Define the syntax table for `oak-mode'.")
(setq oak-mode-syntax-table
      (let ( (syn-table (make-syntax-table)))

	;; comments
	(modify-syntax-entry ?#  "< b" syn-table)
	(modify-syntax-entry ?/  ". 124b" syn-table)
	(modify-syntax-entry ?*  ". 23n"   syn-table)
	(modify-syntax-entry ?\n "> b"    syn-table)

	;; allow - in words
	(modify-syntax-entry ?-  "w"      syn-table)
	;; make ' a general quotation character
	(modify-syntax-entry ?'  "|"      syn-table)
	syn-table))

(defgroup oak-mode nil
  "Emacs support for oak. <https://github.com/KrokodileGlue/oak>"
  :group 'languages
  :prefix "oak-")

;;;###autoload
(define-derived-mode oak-mode prog-mode "oak"
  "Major mode for editing oak source files."
  :syntax-table oak-mode-syntax-table
  (setq-local indent-line-function 'oak-indent-line)
  (setq-local comment-start "# ")
  (setq-local comment-end   "")
  (setq-local comment-start-skip "\\(//+\\|/\\*+\\)\\s *")
  (setq font-lock-defaults '(oak-highlights)))

;; Indentation

(defun oak-indenters-eol-regexp ()
  "Build a regexp to match an indenter at the end of a line."
  ".*\\(:\\|{\\)$")

(defun oak-current-line-empty-p ()
  "Determine the emptiness of the current line."
  (save-excursion
    (beginning-of-line)
    (looking-at "[:space:]*$")))

(require 'cl-lib)

(cl-defun oak-indent-line ()
  "Indent the current line as oak source."
  (interactive)

  (when (not (oak-current-line-empty-p))
    (return-from oak-indent-line))

  (let ((not-indented t) cur-indent)
    ;; the main indentation code
    (save-excursion
      (while not-indented
	(forward-line -1)
	(beginning-of-line)

	(if (not (oak-current-line-empty-p))
	    (progn
	      (setq not-indented nil)
	      (setq cur-indent (current-indentation))))

	(if (looking-at (oak-indenters-eol-regexp))
	    (progn
	      (setq cur-indent (+ (current-indentation) tab-width))
	      (setq not-indented nil)))

	(if (bobp)
	    (setq not-indented nil))))
    (if cur-indent
	(indent-line-to cur-indent)
      (indent-line-to 0))))

(font-lock-add-keywords 'oak-mode
			'(("\\<\\(FIXME\\):" 1
			   font-lock-warning-face t)))
(font-lock-add-keywords 'oak-mode
			'(("\\<\\(TODO\\):" 1
			   font-lock-warning-face t)))
(font-lock-add-keywords 'oak-mode
			'(("\\<\\(HACK\\):" 1
			   font-lock-warning-face t)))

;;;###autoload
(add-to-list 'auto-mode-alist '("\\.k\\'" . oak-mode))

(provide 'oak-mode)

;;; oak-mode.el ends here
