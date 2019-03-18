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
        if(/XString[ \t]+([_a-zA-Z][_a-zA-Z0-9]*);/){
            $fieldName = removePtr(ucfirst($1));
            push @members, "    String get$fieldName();\n";
        }
        # string field
        elsif(/char[ \t]+([_a-zA-Z][_a-zA-Z0-9]*)\[(\d+)\];/){
            $fieldName = ucfirst($1);
            $size = $2;
            push @members, "    String get$fieldName(); // size $size\n";
        }
        # int field
        elsif(/int[ \t]+([_a-zA-Z][_a-zA-Z0-9]*);/){
            $fieldName = ucfirst($1);
            push @members, "    int get$fieldName();\n";
        }
        # regular field
        elsif(/([_a-zA-Z][_a-zA-Z0-9]*)[ \t]+([_a-zA-Z][_a-zA-Z0-9]*);/){
            $type = $1;
            if($type eq "XString"){
                $type = "String";
            }
            $type = fixTypeName($type);
            $fieldName = removePtr(ucfirst($2));
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
        # int method
        elsif(/int[ \t]+\(\*([_a-zA-Z][_a-zA-Z0-9]*)\)\(\);/){
            $methodName = $1;
            push @members, "    int $methodName();\n";
        }
        # struct-returning method
        elsif(/struct\*[ \t]+(_[A-Z]+)[ \t]+([_a-zA-Z][_a-zA-Z0-9]*);/){
            $type = $1;
            $methodName = $2;
            push @members, "    $type $methodName();\n";
        }
        # regular method
        elsif(/([_a-zA-Z][_a-zA-Z0-9]*)[ \t]+\(\*([_a-zA-Z][_a-zA-Z0-9]*)\)\(\);/){
            $type = $1;
            if($type eq "XString"){
                $type = "String";
            }
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
    my $a = shift;
    if($a =~ /^p[A-Z]/){
        $a = substr($a, 1)
    }
    ucfirst($a)
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
