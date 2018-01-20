#!/usr/bin/env perl
use warnings;
use strict;
use v5.10; # for say
use File::Which;
use File::Find;
use File::Spec;
use Cwd;

# define some variables
my $project='hugin';
my $bugaddr='https://bugs.launchpad.net/hugin/';
my $copyright='Pablo dAngelo';

# safety checks
my $xgettext=which 'xgettext';
if(!defined $xgettext) {die 'ERROR: xgettext not found';}
my $wxrc=which 'wxrc';
if(!defined $wxrc) { die 'ERROR: wxrc not found'; }
my $msgmerge=which 'msgmerge';
if(!defined $msgmerge) { die 'ERROR: msgmerge not found'; }
# store current directory
my $basedir=getcwd;
say 'Found all necessary programs';

# parse all xrc files, create xrc.cpp for xgettext input
unlink 'xrc.cpp';
unlink 'infiles.list';
chdir "..";
find({wanted => sub { if (-f and /\.xrc$/)
  {
    my $filename=File::Spec->abs2rel($File::Find::name);
    if ($^O eq 'MSWin32') {$filename =~ tr#\\#/#;};
    say("Processing $filename");
    system("$wxrc -g $filename >>$basedir/xrc.cpp");
  };
}, no_chdir => 1}, getcwd);

#create array of all C++ files
my @cfiles;
my $cstartdir=getcwd;
find({wanted => sub {  if (-f) {
    my $filename=File::Spec->abs2rel($File::Find::name);
    if ($^O eq 'MSWin32') {$filename =~ tr#\\#/#;};
    push(@cfiles, $filename) if /\.(cpp|c|h)$/;
  }}, no_chdir => 1}, getcwd);
@cfiles=sort @cfiles;
# create input file list for xgettext
say 'Extracting messages';
# first write all C++ files in file
open(my $filelist, '>', "$basedir/infiles.list") or die 'ERROR: Could not open file $basedir/infiles.list';
foreach (@cfiles) {say $filelist $_;};
# then add files from POTFILES.in (e.g. tips)
open(my $potfilein, '<', "$basedir/POTFILES.in") or die 'ERROR: Could not open file $basedir/POTFILES.in';
while(<$potfilein>){say $filelist $_;};
close $potfilein;
close $filelist;

# run xgettext
chdir $basedir;
system("$xgettext --from-code=UTF-8 --c++ --keyword=_ --copyright-holder=\"$copyright\" --msgid-bugs-address=\"$bugaddr\" --files-from=infiles.list --directory=\"$basedir\" --directory=\"$cstartdir\" --output=$project.pot")==0 or die 'ERROR during running xgettext';
say 'Done extracting messages';

# update all po files with new template
say 'Merging translations';
find(sub{if (-f and /\.po$/) {
    system("msgmerge --verbose -o $_.new.po $_ $basedir/$project.pot")==0 or die 'ERROR during running msgmerge';
    rename("$_.new.po", $_);
  }}, '.');
say 'Done merging translations';
unlink 'xrc.cpp';
unlink 'infiles.list';
# unify line breaks to unix
say 'Unify line breaks';
system("perl unix_linebreaks.pl $project.pot *.po outdated/*.po");
say 'All done';
