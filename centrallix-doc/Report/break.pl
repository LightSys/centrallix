#!/usr/bin/perl -w

use strict;

my $filename=shift @ARGV;
my ($header,$footer);
my $status=0;


open FILE,"$filename";

while(<FILE>)
{
	my($file) = /<!-- PAGE BREAK: '(.*)' -->/;

	if($file && $file eq 'toc.html') { $status=1; next; }
	if($file && $file eq 'END') { $status=2; next; }

	if($status==0) { $header.=$_; }
	if($status==2) { $footer.=$_; }
}

close FILE;

open FILE,"$filename";

my $fileopen;

while(<FILE>)
{
	my($file) = /<!-- PAGE BREAK: '(.*)' -->/;

	if($file && $file eq 'toc.html') { $status=1; }
	if($file && $file eq 'END') { $status=2; }

	if($file && $status==1 )
	{
		if($fileopen) { print OUTFILE $footer; close OUTFILE; }
		open OUTFILE,">$file";
		print OUTFILE $header;
		$fileopen=$file;
	}
	if($fileopen && $fileopen eq 'toc.html')
	{
		s!(<td><a href=")\#(.*?)(">.*?</a></td>)!$1$2.html$3!;
	}
	print OUTFILE if($status==1);
}

close OUTFILE;
close FILE;




