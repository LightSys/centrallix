#!/usr/bin/perl
# insert a call to this file in your commitinfo:
# <module-name> /path/to/commitinfo.pl
#
# Copyright (c) 1999, 2000 Sascha Schumann <sascha@schumann.cx>

use strict; 

my $cvsroot    = $ENV{CVSROOT}."/";
# remove double trailing slash
$cvsroot =~ s/\/\/$/\//;

my $last_file = "$cvsroot/CVSROOT/tmp/lastfile"; 
my $id = getpgrp(); 
my $directory = $ARGV[0]; 
open(FC, ">$last_file.$id") || die "cannot open last file";
print FC "$directory\n";
close(FC); # throw away STDIN so parent doesn't get SIGPIPE
while(<>) { }
