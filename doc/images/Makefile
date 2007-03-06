#
#
#  Makefile for converting images from png to pdf
#
#
# If you don't like the scaling of an image, change it in the
# image_scales file.  This is the file where image_convert.pl
# keeps its list of how images should be scaled, and scale
# factors in this file will be used preferentially (There was a
# bug so it didn't do this until now).	If you change an image
# and want it automatically rescaled, delete it's line from the
# image_scales file and do 'make converteps'.
#

# png images
IMAGES =  bacula-applications \
	  bacula-objects \
	  Bacula-tray-monitor \
	  bimagemgr1 \
	  bimagemgr2 \
	  bimagemgr3 \
	  Conf-Diagram \
	  win32-finish \
	  win32-location \
	  win32-nsis \
	  win32-pkg \
	  win32-service-ok \
	  win32-service \
	  win32-start \
	  win32-welcome \
	  access-is-denied \
	  confirm \
	  flow \
	  menu \
	  philosophical-gnu-sm \
	  properties-security-advanced-owner \
	  properties-security \
	  tray-icon \
	  view-only \
	  back \
	  bacula-logo \
	  bclogo \
	  error \
	  file \
	  home \
	  idle \
	  md5 \
	  next \
	  running \
	  smartall \
	  thanks

#	  xp-control-panel  
#	  xp-windows-firewall-advanced \
#	  xp-windows-firewall-advanced-settings-after \
#	  xp-windows-firewall-advanced-settings-before \
#	  xp-windows-firewall-advanced-settings-service-details \
#	  xp-windows-firewall


# jpg images
JPGIMAGES = xp-control-panel \
	  xp-windows-firewall-advanced \
	  xp-windows-firewall-advanced-settings-after \
	  xp-windows-firewall-advanced-settings-before \
	  xp-windows-firewall-advanced-settings-service-details \
	  xp-windows-firewall  

# gif images
GIFIMAGES = back \
	    bacula-logo \
	    bclogo \
	    error \
	    file \
	    home \
	    idle \
	    md5 \
	    next \
	    running \
	    smartall \
	    thanks  




first_rule: all

.SUFFIXES:     .texi .html .pdf
.PHONY:

all:	convert

# Convert png images to pdf (texinfo needs both)
convertpdf:	  
	(for i in ${IMAGES} ; do \
	   pngtopnm $${i}.png | pnmtops -scale=0.85 --noturn -nosetpage | epstopdf --filter >$${i}.pdf ; \
	 done)

# Convert any png images to eps (for LaTeX)
converteps:
	(for i in ${IMAGES} ; do \
	   pngtopnm $${i}.png | pnmtops -scale=0.65 --noturn -nosetpage >$${i}.eps; \
	   ./image_convert.pl --scalefile=image_scales --defaultscale=0.65 --resolution=140 --maxx=5 --maxy=8 $${i}.png hires/$${i}.eps; \
	 done)
   
# Convert any JPEG images to png
convertjpg:	  
	(for i in ${JPGIMAGES} ; do \
	   jpegtopnm $${i}.jpg | pnmtopng >$${i}.png ; \
	 done)

# Convert any GIF images to png
convertgif:
	(for i in ${GIFIMAGES} ; do \
	   giftopnm $${i}.gif | pnmtopng >$${i}.png ; \
	 done)
 
clean:
	@rm -f 1 2 3

distclean:  clean
