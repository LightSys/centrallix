.PHONY: multipage clean

HTML := ${subst index.html,,${wildcard *.html}}

multipage: onepage.html
	./break.pl onepage.html

onepage.html: report.xml report.xsl
	xsltproc report.xsl report.xml > onepage.html

website.sql: report.xml report-website.xsl
	xsltproc report-website.xsl report.xml > website.sql

clean:
	rm -f ${HTML}
