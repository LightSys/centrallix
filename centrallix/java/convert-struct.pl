#!/usr/bin/perl -w
#use strict;

my $inStruct = 0;
my $structName = 0;

my @members = ();
my @imports = ();
while(<>){
    chomp;
    if($inStruct){
        # array field
        if(/XArray[ \t]+([_a-zA-Z][_a-zA-Z0-9]*);/){
            $fieldName = ucfirst($1);
            push @members, "    List get$fieldName();\n";
            push @imports, "import java.util.List;";
        }
        # string field
        elsif(/char[ \t]+([_a-zA-Z][_a-zA-Z0-9]*)\[(\d+)\];/){
            $fieldName = ucfirst($1);
            $size = $2;
            push @members, "    String get$fieldName(); // size $size\n";
        }
        # regular field
        elsif(/([_a-zA-Z][_a-zA-Z0-9]*)[ \t]+([_a-zA-Z][_a-zA-Z0-9]*);/){
            $type = $1;
            $fieldName = ucfirst($2);
            push @members, "    $type get$fieldName();\n";
        }
        # object field
        elsif(/void\*[ \t]+([_a-zA-Z][_a-zA-Z0-9]*);/){
            $fieldName = ucfirst($1);
            push @members, "    Object $fieldName();\n";
        }
        # object method
        elsif(/void\*[ \t]+\(\*([_a-zA-Z][_a-zA-Z0-9]*)\)\(\);/){
            $methodName = $1;
            push @members, "    Object $methodName();\n";
        }
        # string method
        elsif(/char\*[ \t]+\(\*([_a-zA-Z][_a-zA-Z0-9]*)\)\(\);/){
            $methodName = $1;
            push @members, "    String $methodName();\n";
        }
        # regular method
        elsif(/([_a-zA-Z][_a-zA-Z0-9]*)[ \t]+\(\*([_a-zA-Z][_a-zA-Z0-9]*)\)\(\);/){
            $returnType = $1;
            $methodName = $2;
            push @members, "    $returnType $methodName();\n";
        }
        elsif(/}/){
        }
        elsif(/^[ \t]*([_a-zA-Z][_a-zA-Z0-9]*), .*;/){
            $structName = $1;
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

sub uniq {
    my %seen;
    grep !$seen{$_}++, @_;
}
