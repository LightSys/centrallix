.PHONY: multipage clean

HTML := ${subst index.html,,${wildcard *.html}}

multipage: onepage.html
	./break.pl onepage.html

onepage.html: widgets.xml widgets.xsl
	xsltproc widgets.xsl widgets.xml > onepage.html

clean:
	rm -f ${HTML}
