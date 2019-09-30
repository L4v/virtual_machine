(setq user-init-file (or load-file-name (buffer-file-name)))
(setq user-emacs-directory (file-name-directory user-init-file))

(package-initialize)

(require 'package)
(add-to-list 'package-archives
	     '("marmalade" .
	       "https://marmalade-repo.org/packages/"))


(when (>= emacs-major-version 24)
  (require 'package)
  (add-to-list
   'package-archives
   '("melpa" . "https://melpa.milkbox.net/packages/")
   t))

(require 'package)
(add-to-list 'package-archives '("melpa" . "http://melpa.org/packages/"))

;; ;; Bootstrap `use-package'
;; ;; http://www.lunaryorn.com/2015/01/06/my-emacs-configuration-with-use-package.html
;; ;; use-package autoloads will make sure it get pulled in at the right time
;; ;; read "package autoloads":  http://www.lunaryorn.com/2014/07/02/autoloads-in-emacs-lisp.html
;; (unless (package-installed-p 'use-package)
;;   (package-refresh-contents)
;;   (package-install 'use-package))

;; (custom-set-variables
;;  ;; custom-set-variables was added by Custom.
;;  ;; If you edit it by hand, you could mess it up, so be careful.
;;  ;; Your init file should contain only one such instance.
;;  ;; If there is more than one, they won't work right.
;;  '(package-selected-packages (quote (use-package))))
;; (custom-set-faces
;;  ;; custom-set-faces was added by Custom.
;;  ;; If you edit it by hand, you could mess it up, so be careful.
;;  ;; Your init file should contain only one such instance.
;;  ;; If there is more than one, they won't work right.
;;  )

;; (drag-stuff-global-mode 1)
;; (when (version<= "26.0.50" emacs-version )
;;   (global-display-line-numbers-mode))

(set-face-attribute 'region nil :background "#666" :foreground "#ffffff")

;; TODOs, NOTEs and such
(setq fixme-modes '(c++-mode c-mode emacs-lisp-mode))
(make-face 'font-lock-fixme-face)
(make-face 'font-lock-note-face)
(make-face 'font-lock-important-face)
(make-face 'font-lock-study-face)
(mapc (lambda (mode)
	      (font-lock-add-keywords
	       mode
	       '(("\\<\\(TODO\\)" 1 'font-lock-fixme-face t)
		 ("\\<\\(NOTE\\)" 1 'font-lock-note-face t)
		 ("\\<\\(IMPORTANT\\)" 1 'font-lock-important-face t)
		 ("\\<\\(STUDY\\)" 1 'font-lock-study-face t))))
      fixme-modes)
(modify-face 'font-lock-fixme-face "Red" nil nil t nil t nil nil)
(modify-face 'font-lock-note-face "Dark Green" nil nil t nil t nil nil)
(modify-face 'font-lock-important-face "Orange" nil nil t nil t nil nil)
(modify-face 'font-lock-study-face "Orange" nil nil t nil t nil nil)

;; Disable toolbar
(tool-bar-mode 0)

;; Find .h / .cpp equivalent of file using C-c o
(global-set-key  (kbd "<C-tab>") 'ff-find-other-file)

;; Run compile script
(setq compile-command "./build.sh")
(global-set-key (kbd "<f5>") 'compile)

;; Show line numbers
(when (version<= "26.0.50" emacs-version )
  (global-display-line-numbers-mode))
