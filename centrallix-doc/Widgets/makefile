.PHONY: multipage clean all doc-report

HTML := ${subst index.html,,${wildcard *.html}}

multipage: onepage.html
	./break.pl onepage.html

onepage.html: widgets.xml widgets.xsl
	xsltproc widgets.xsl widgets.xml > onepage.html

website.sql: widgets.xml widgets-website.xsl
	xsltproc widgets-website.xsl widgets.xml > website.sql

all: multipage website.sql

doc-report:
	python3 ./doc-report/generate.py --repo-root ../..

clean:
	rm -f ${HTML}
