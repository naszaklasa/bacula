#
#
#  Makefile for LaTeX  
#
# To build everything do
#    make tex
#    make links
#    make all
#    make web
#    make html
#    make pdf
#
# or simply
#
#    make
#

IMAGES=../images

#  bacula  -- special case below
#  running -- special case below
#  developers -- special case below

#
# Note, these are all parts of the manual not in any
#  particular order (mostly alphabetic). The order they
#  appear in the manual is defined in bacula.tex
#
# Note also that certain of these chapters are in the 
#  bacula.tex manual, and others in the developers.tex 
#  manual.
#
MANUAL = \
   autochangers bootstrap bugs catalog \
   catmaintenance configure consoleconf console critical \
   daemonprotocol dirdconf director disk faq filedconf file \
   firewalls gpl install internaldb kaboom lesser \
   license messagesres monitorconf mysql oldfileset pools \
   porting postgresql progs projects quickstart recycling \
   regression rescuefloppy rescue restore rpm-faq \
   security spooling sqlite state storage \
   storedconf strategies stunnel \
   requirements supportedoses supporteddrives tapetesting \
   thanks tips vars verify win32 \
   daemonprotocol gui-interface \
   supportedchangers \
   md5 mediaformat mempool netprotocol porting smartall


first_rule: bacula developers

bacula: tex web html dvipdf

# Note, assume bacula manual was built first
#  I've removed devhtml so that the resulting files
#  (images) do not conflict with the main manual
developers: devtex devweb devpdf

.SUFFIXES:     .tex .html
.PHONY:
.DONTCARE:


tex:
	@cp -fp ${IMAGES}/hires/*.eps .
	@touch baculai-dir.tex baculai-fd.tex baculai-sd.tex \
	     baculai-console.tex baculai-general.tex
	-latex -interaction=batchmode bacula.tex
	makeindex bacula.idx -o bacula.ind 2>/dev/null
	makeindex bacula.ddx -o bacula.dnd >/dev/null 2>/dev/null
	makeindex bacula.fdx -o bacula.fnd >/dev/null 2>/dev/null
	makeindex bacula.sdx -o bacula.snd >/dev/null 2>/dev/null
	makeindex bacula.cdx -o bacula.cnd >/dev/null 2>/dev/null
	-latex -interaction=batchmode bacula.tex
	@rm -f *.eps *.old


devtex:
	@cp -fp ${IMAGES}/hires/*.eps .
	-latex -interaction=batchmode developers.tex
	makeindex developers.idx >/dev/null 2>/dev/null
	-latex -interaction=batchmode developers.tex
	@rm -f *.eps *.old

pdf:
	@echo "Making pdfm"
	@cp -fp ${IMAGES}/hires/*.eps .
	dvipdfm -p a4 bacula.dvi
	@rm -f *.eps *.old

dvipdf:
	@echo "Making dvi to pdf"
	@cp -fp ${IMAGES}/hires/*.eps .
	dvipdf bacula.dvi bacula.pdf
	@rm -f *.eps *.old


devpdf:
	@echo "Making developers pdf"
	@cp -fp ${IMAGES}/hires/*.eps .
	dvipdf developers.dvi developers.pdf
	@rm -f *.eps *.old

devpdfm:
	@echo "Making pdfm"
	@cp -fp ${IMAGES}/hires/*.eps .
	dvipdfm -p a4 developers.dvi
	@rm -f *.eps *.old

html:
	@echo "Making html"
	@cp -fp ${IMAGES}/*.eps .
	latex2html -white -no_subdir -split 0 -toc_stars -white -notransparent \
		bacula >/dev/null
	./translate_images.pl bacula.html
	@rm -f *.eps *.gif *.jpg

devhtml:
	@echo "Making developers html"
	@cp -fp ${IMAGES}/*.eps .
	latex2html -white -no_subdir -split 0 -toc_stars -white -notransparent \
		developers >/dev/null
	./translate_images.pl developers.html
	@rm -f *.eps *.gif *.jpg *.old


web:
	@echo "Making web"
	@mkdir -p bacula
	@rm -rf bacula/*
	@cp -fp ${IMAGES}/*.eps .
	@rm -f next.eps next.png prev.eps prev.png up.eps up.png
#	@cp -fp ${IMAGES}/*.eps  ${IMAGES}/*.png *.txt bacula
	@cp -fp ${IMAGES}/*.eps *.txt bacula
	@rm -f bacula/next.eps bacula/next.png bacula/prev.eps bacula/prev.png bacula/up.eps bacula/up.png
	latex2html -split 4 -local_icons -t "Bacula User's Guide" -long_titles 4 \
		-toc_stars -contents_in_nav -white -notransparent bacula >/dev/null
	./translate_images.pl bacula/Bacula_Users_Guide.html
	@cp -f bacula/Bacula_Users_Guide.html bacula/index.html
	@cp -f bacula/Bacula_Freque_Asked_Questi.html bacula/faq.html 
	@rm -f *.eps *.gif *.jpg bacula/*.eps *.old

devweb:
	@echo "Making developers web"
	@mkdir -p developers
	@rm -f developers/*
	@cp -fp ${IMAGES}/*.eps .
	@rm -f next.eps next.png prev.eps prev.png up.eps up.png
	@cp -fp ${IMAGES}/*.eps ${IMAGES}/*.png *.txt developers
	@rm -f developers/next.eps developers/next.png developers/prev.eps developers/prev.png developers/up.eps developers/up.png
	latex2html -split 5 -local_icons -t "Developer's Guide" -auto_nav -long_titles 4 \
		-contents_in_nav -toc_stars -white -notransparent developers >/dev/null
	./translate_images.pl developers/Developers_Guide.html
	@cp -f developers/Developers_Guide.html developers/index.html
	@rm -f *.eps *.gif *.jpg developers/*.eps  *.old

texcheck:
	./check_tex.pl bacula.tex
	./check_tex.pl developers.tex

main_configs:
	pic2graph -density 100 <main_configs.pic >main_configs.png

clean:
	@rm -f 1 2 3
	@rm -f *.png *.gif *.jpg *.eps
	@rm -f *.pdf *.aux *.cp *.fn *.ky *.log *.pg
	@rm -f *.html *.backup *.pdf *.ps *.dvi *.ilg *.lof *.lot
	@rm -f *.cdx *.cnd *.ddx *.ddn *.fdx *.fnd *.ind *.sdx *.snd
	@rm -f *.dnd
	@rm -f *.old WARNINGS *.out *.toc *.idx
	@rm -f images.pl labels.pl internals.pl
	@rm -rf bacula developers
	@rm -f baculai-dir.tex baculai-fd.tex baculai-sd.tex \
	     baculai-console.tex baculai-general.tex images.tex developersi.tex


distclean:  clean
	@rm -f bacula.html bacula.pdf developers.html developers.pdf
