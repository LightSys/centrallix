#!/usr/bin/perl -w
#use strict;

# Produces a file that maps abbreviated struct names to their meaningful names, by parsing .h files
# Output of this is used by the convert-struct.pl script to get the correct names when teh abbreviated
# name is used.

my $inStruct = 0;
my $structName = undef;
my $shortName = undef;

while(<>){
    chomp;
    if($inStruct){
        if(/}/){
        }
        elsif(/^[ \t]*([_a-zA-Z][_a-zA-Z0-9]*), .*;/){
            $structName = $1;
            if($structName eq "Object"){
                $structName = "ObjectInstance";
            }
            if(defined($shortName)){
                $shortName =~ s/\s//g;
                print "$shortName $structName\n";
            }
            $structName = undef;
            $shortName = undef;
            $inStruct = 0;
        }
    }
    elsif(/^typedef struct([ \t]+([__a-zA-Z][__a-zA-Z0-9]*))?/){
        $inStruct = 1;
        $shortName = $1;
    }
}
