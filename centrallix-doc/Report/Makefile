.PHONY: multipage clean

HTML := ${subst index.html,,${wildcard *.html}}

multipage: onepage.html
	./break.pl onepage.html

onepage.html: report.xml report.xsl
	xsltproc report.xsl report.xml > onepage.html

clean:
	rm -f ${HTML}
