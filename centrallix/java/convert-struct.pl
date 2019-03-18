#!/usr/bin/perl -w
#use strict;

my $inStruct = 0;
my $structName = 0;

my @members = ();
my @imports = ();
while(<>){
    chomp;
    if($inStruct){
        # string field
        if(/char[ \t]+([_a-zA-Z][_a-zA-Z0-9]*)\[(\d+)\];/){
            $fieldName = ucfirst($1);
            $size = $2;
            push @members, "    String get$fieldName(); // size $size\n";
        }
        # struct-returning method
        elsif(/struct[ \t]+(_[A-Z]+)\*[ \t]+([_a-zA-Z][_a-zA-Z0-9]*);/){
            $type = $1;
            $methodName = $2;
            push @members, "    $type $methodName();\n";
        }
        # regular field
        elsif(/([_a-zA-Z][_a-zA-Z0-9]*\*?)[ \t]+([_a-zA-Z][_a-zA-Z0-9]*);/){
            $type = $1;
            $type = fixTypeName($type);
            $fieldName = fixFieldName($2);
            push @members, "    $type get$fieldName();\n";
        }
        # regular method
        elsif(/([_a-zA-Z][_a-zA-Z0-9]*\*?)[ \t]+\(\*([_a-zA-Z][_a-zA-Z0-9]*)\)\(\);/){
            $type = $1;
            $type = fixTypeName($type);
            $methodName = $2;
            push @members, "    $type $methodName();\n";
        }
        elsif(/}/){
        }
        elsif(/^[ \t]*([_a-zA-Z][_a-zA-Z0-9]*), .*;/){
            $structName = $1;
            if($structName eq "Object"){
                $structName = "ObjectInstance";
            }
            @lines = ();
            push @lines, "package org.lightsys.centrallix.objectsystem;\n\n";
            push @lines, uniq(@imports);
            push @lines, "\n\n";
            push @lines, "public interface $structName {\n";
            push @lines, @members;
            push @lines, "}\n";
            dumpLines();
            $inStruct = 0;
            @members = ();
            @imports = ();
        }
    }
    elsif(/^typedef struct[ \t]+([__a-zA-Z][__a-zA-Z0-9]*)/){
        $inStruct = 1;
        $structName = $1;
    }
}

sub dumpLines {
    my $name = $structName;
    my $file = "src/main/java/org/lightsys/centrallix/objectsystem/$name.java";
    print "Writing $file ...\n";
    open $out, ">", $file or die "can't open file $file";
    for $line (@lines){
        print $out $line;
    }
    close $out;
}

sub fixTypeName {
    my $type = shift;
    if($type =~ /^(int|short|float|double|char)$/) { 0; }
    elsif($type eq 'char*') { $type = 'String'; }
    elsif($type eq 'XString') { $type = 'String'; }
    elsif($type eq 'pXString') { $type = 'String'; }
    elsif($type eq 'void*') { $type = 'Object'; }
    elsif($type eq 'XArray') { 
        $type = 'List';
        push @imports, "import java.util.List;";
    }
    elsif($type =~ /^(_[A-Z])\*/) { $type = $1; }
    elsif($type =~ /^p[A-Z]/) { $type = substr($type,1); }
    else { $type = ucfirst($type); }
    return $type;
}

sub fixFieldName {
    my $name = shift;
    $name = ucfirst($name);
    return $name;
}

sub uniq {
    my %seen;
    grep !$seen{$_}++, @_;
}

sub removePtr {
    shift;
    s/Ptr$//;
    return $_;
}
