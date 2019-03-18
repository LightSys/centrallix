#!/usr/bin/perl -w
#use strict;

my $inStruct = 0;
my $structName = 0;

sub lcFirst {
    my $a = shift;
    return ucfirst $a;
}

my @lines = ();
while(<>){
    chomp;
    if($inStruct){
        # array field
        if(/XArray[ \t]+([_a-zA-Z][_a-zA-Z0-9]*);/){
            $fieldName = lcFirst($1);
            push @lines, "    List get$fieldName();\n";
        }
        # string field
        elsif(/char[ \t]+([_a-zA-Z][_a-zA-Z0-9]*)\[(\d+)\];/){
            $fieldName = lcFirst($1);
            $size = $2;
            push @lines, "    String get$fieldName(); // size $size\n";
        }
        # regular field
        elsif(/([_a-zA-Z][_a-zA-Z0-9]*)[ \t]+([_a-zA-Z][_a-zA-Z0-9]*);/){
            $type = $1;
            $fieldName = lcFirst($2);
            push @lines, "    $type get$fieldName();\n";
        }
        # object field
        elsif(/void\*[ \t]+([_a-zA-Z][_a-zA-Z0-9]*);/){
            $fieldName = lcFirst($1);
            push @lines, "    Object $fieldName();\n";
        }
        # object method
        elsif(/void\*[ \t]+\(\*([_a-zA-Z][_a-zA-Z0-9]*)\)\(\);/){
            $methodName = $1;
            push @lines, "    Object $methodName();\n";
        }
        # string method
        elsif(/char\*[ \t]+\(\*([_a-zA-Z][_a-zA-Z0-9]*)\)\(\);/){
            $methodName = $1;
            push @lines, "    String $methodName();\n";
        }
        # regular method
        elsif(/([_a-zA-Z][_a-zA-Z0-9]*)[ \t]+\(\*([_a-zA-Z][_a-zA-Z0-9]*)\)\(\);/){
            $returnType = $1;
            $methodName = $2;
            push @lines, "    $returnType $methodName();\n";
        }
        elsif(/}/){
            push @lines, "}\n\n";
        }
        elsif(/^[ \t]*([_a-zA-Z][_a-zA-Z0-9]*), .*;/){
            $structName = $1;
            unshift @lines, "public interface $structName {\n";
            for $line (@lines){
                print $line;
            }
            $inStruct = 0;
            @lines = ();
        }
    }
    elsif(/^typedef struct[ \t]+([__a-zA-Z][__a-zA-Z0-9]*)/){
        $inStruct = 1;
        $structName = $1;
    }
}
