#!/usr/bin/perl -w
# include this script in your loginfo as:
# <modulename>   /path/to/loginfo.pl sender@domain recipient@domain %{sVv}
#
# Copyright (c) 1999, 2000 Sascha Schumann <sascha@schumann.cx>

# This makes some basic assumptions -- you are only checking
# in to a single CVS module.

# This also doesn't like files or directories with spaces in them.

use strict;

use Socket;
use POSIX;

$SIG{PIPE} = 'IGNORE';

my $smtpserver = "127.0.0.1";
my $smtpport   = 25;
my $cvs        = "/usr/bin/cvs";
my $cvsroot    = $ENV{CVSROOT}."/";
# remove double trailing slash
$cvsroot =~ s/\/\/$/\//;
my $cvsusers   = "$cvsroot/CVSROOT/cvsusers";
my $last_file  = "$cvsroot/CVSROOT/tmp/lastfile";
my $summary    = "$cvsroot/CVSROOT/tmp/summary";

# get the id of this process group for use in figuring out
# whether this is the last directory with checkins or not
my $id = getpgrp();

# the command line looks something like this for a normal commit:
#  ("user@example.com", "cvsuser",
#   "module changedfile,1.1,1.2 addedfile,NONE,1.1 removedfile,1.1,NONE")
my $mailfrom = shift;
my $mailto = $mailfrom;
my $envaddr = $mailto;

my $cvsuser = shift;
my @args = split(" ", $ARGV[0]);
my $directory = shift @args;

# extract just the module name from the directory
my $module = $directory;
$module =~ s/\/.+$//;

# bail when this is a new directory
&bail if $args[0] eq '-' && "$args[1] $args[2]" eq 'New directory';

# bail if this is an import
&bail if $args[0] eq '-' && $args[1] eq 'Imported';

# find out the last directory being processed
open FC, "$last_file.$id"
	or die "last file does not exist";
my $last_directory = <FC>;
chop $last_directory;
close FC;
# remove the cvsroot from the front
$last_directory =~ s/^$cvsroot//;

# add our changed files to the summary
open(FC, ">>$summary.$id") || die "cannot open summary file";
foreach my $arg (@args) {
	print FC "$directory/$arg\n";
}
close(FC);

# is this script already in the last changed directory?

# exit if this isn't the last directory
&bail if($last_directory ne $directory);

# get the log message and tag -- we throw away everything from STDIN
# before a line that begins with "Log Message"
my ($logmsg,$tag) = &get_log_message();

# now we fork off into the background and generate the email
exit 0 if(fork() != 0);

$| = 1;

#print "Reading summary file\n";

open(FC, "<$summary.$id");

my (@added_files, @removed_files, @modified_files, @modified_files_info);
while (<FC>) {
	chop;
	my ($file, $old, $new) = split(",");
	if($old eq "NONE") {
		push @added_files, $file;
	} elsif($new eq "NONE") {
		push @removed_files, $file;
	} else {
		push @modified_files, $file;
		push @modified_files_info, [ $file, $old, $new ];
	}
}
close FC;

#print "Unlinking helper files\n";

# clean up a little bit

unlink("$summary.$id");
unlink("$last_file.$id");

#print "Running rdiff\n";

# build a diff (and new files) if necessary
my $diffmsg = '';

foreach my $info (@modified_files_info) {
	my ($file, $old, $new) = @$info;
	open(LOG, "$cvs -Qn rdiff -r $old -r $new -u $file|") || die;
	while(<LOG>) { s/\r\n/\n/; $diffmsg .= $_; }
	close(LOG);
}

# add the added files

foreach my $file (@added_files) {
	next if $file =~ /\.(gif|jpe|jpe?g|pdf|png|exe|class|tgz|tar.gz|jar)$/i
		or $file !~ /\./;
	$diffmsg .= "\nIndex: $file\n+++ $file\n";
	open(LOG, "$cvs -Qn checkout -p -r1.1 $file |") || die;
	while(<LOG>) { s/\r\n/\n/; $diffmsg .= $_; }
	close(LOG);
}

#print "Building commit email\n";

my $subj_tag = $tag ? "($tag)" : '';
my $body_tag = $tag ? "(Branch: $tag)" : '';

# build our email
my $msg = "";
if($#added_files ne -1) {
	$msg .= "\n  Added files:                 $body_tag";
	$msg .= &build_list(@added_files);
	$body_tag = '';
}
if($#removed_files ne -1) {
	$msg .= "\n  Removed files:               $body_tag";
	$msg .= &build_list(@removed_files);
	$body_tag = '';
}
if($#modified_files ne -1) {
	$msg .= "\n  Modified files:              $body_tag";
	$msg .= &build_list(@modified_files);
	$body_tag = '';
}

my $subj = "";
my %dirfiles;
my @dirs = &get_dirs(@added_files, @removed_files, @modified_files);

foreach my $dir (@dirs) {
    $subj .= "$dir @{ $dirfiles{$dir} }  ";
}

my $msgid = "Message-ID: <cvs$cvsuser".time()."\@cvsserver>\n";

my $from;
if (open FD, $cvsusers) {
	while(<FD>) {
		chop;
		if (m/^$cvsuser:(.+?):(.+)$/) {
			$from = "\"$1\" <$2>";
		}
	}
	close(FD);
}

$from ||= "$cvsuser <$mailfrom>";

# "Reply-to: $mailto\n".
# "Date: ".localtime()."\n".
my (@DAYABBR) = qw(Sun Mon Tue Wed Thu Fri Sat);
my (@MONABBR) = qw(Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec);

my (@gmtime) = gmtime();
my $rfc822date = sprintf("Date: %s, %02d %s %d %02d:%02d:%02d -0000\n",
        $DAYABBR[$gmtime[6]], $gmtime[3], $MONABBR[$gmtime[4]],
        $gmtime[5] + 1900, $gmtime[2], $gmtime[1], $gmtime[0]);

no strict; # quiet warnings after here

my $email;
my $common_header = "".
	"From: $from\n".
	"To: $mailto\n".
	$msgid.
	$rfc822date.
	"Subject: cvs: $module$subj_tag $subj\n";

my $common_body = "".
	"$cvsuser\t\t".localtime()." EDT\n".
	"$msg".
	"  Log:\n".
	&indent($logmsg,2)."\n";

my $boundary = $cvsuser.time();

if (length($diffmsg) > 8000) {
	my $now = POSIX::strftime("%Y%m%d%H%M%S", localtime);
	$email = $common_header.
		"MIME-Version: 1.0\n".
		"Content-Type: multipart/mixed; boundary=\"$boundary\"\n".
		"\n".
		"This is a MIME encoded message\n\n".
		"--$boundary\n".
		"Content-Type: text/plain\n".
		"\n".
		$common_body.
		"--$boundary\n".
		"Content-Type: text/plain\n".
		"Content-Disposition: attachment; filename=\"$cvsuser-$now.txt\"\n".
		"\n".
		"$diffmsg\n".
		"--$boundary--\n";
} else {
	$email = $common_header.
		"\n".
		$common_body.
		"$diffmsg\n";
}

$email =~ s/\r//g;
$email =~ s/\n/\r\n/g;

# send our email

print "Mailing the commit email to $mailto\n";

#print $email;

my $paddr = sockaddr_in($smtpport, inet_aton($smtpserver));
socket(SOCK, PF_INET, SOCK_STREAM, 0) || die "socket failed";
connect(SOCK, $paddr) || die "connect $smtpserver:$smtpport failed";
select(SOCK);
$|=1;

print "HELO cvsserver\r\n".
"MAIL FROM:<this-will-bounce\@centrallix.org>\r\n" . 
"RCPT TO:<$envaddr>\r\n" .
"DATA\r\n".
"$email\r\n".
".\r\n".
"QUIT\r\n";

while(<SOCK>) { alarm(20); };

close(SOCK);
exit 0;

sub get_log_message {
  my ($logmsg, $tag);
  while (<STDIN>) {
    $logmsg .= $_ if defined $logmsg;
    if (/^Log Message/) { $logmsg = ""; }
    if (/^\s+Tag:\s+(\w+)/) { $tag = $1; }
  }
  return ($logmsg, $tag);
}

sub build_list {
  my(@arr) = @_;
  my($curdir, $curlen, $msg);

  $msg = "";
  $curdir = "";
  foreach (@arr) {
    /^(.*)\/([^\/]+)$/;
    my $dir = $1;
    my $file = $2;
    if($dir ne $curdir) {
      $curdir = $dir;
      $msg .= "\n    /$curdir\t";
      $curlen = length($curdir) + 5;
    }
    if(($curlen + length($file)) > 70) {
      $msg .= "\n     ".sprintf("%-".length($curdir)."s", "")."\t";
      $curlen = length($curdir) + 5;
    }
    $msg .= $file." ";
    $curlen += length($file) + 1;
  }

  $msg .= "\n";

  return $msg;
}

sub get_dirs {
  my @files = sort @_;
  foreach my $file (@files) {
    (my $dir = $file) =~ s#[^/]+$##;
    $dir =~ s/^$module//;
    $dir =~ s/(.+)\//$1/;
    $file =~ s#^.+/(.+)$#$1#;
    push @{ $dirfiles{$dir} }, $file;
  } 
  return sort keys %dirfiles;
} 

sub indent {
  my ($msg,$nr) = @_;
  my $s = " " x $nr;
  $msg =~ s/\n/\n$s/g;
  return $s.$msg;
}

sub trim {
  my ($x) = @_;
  $x =~ s/^\s+//;
  $x =~ s/\s+$//;
  return $x;
}

# eat STDIN (to avoid parent getting SIGPIPE) and exit with supplied exit code
sub bail {
  my @toss = <STDIN>;
  exit @_;
}
