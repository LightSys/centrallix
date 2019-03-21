#!/usr/bin/perl -w
#use strict;

sub usage {
    print "Usage: $0 <output-root> <java-package-name> <header-files...>\n\n" .
    "  This script converts all of the C structs within a given header file into Java interfaces.\n" .
    "  It should be run from the root directory of where you want the Java code to be placed.\n" .
    "  One .java file will be created for each interface, and package subdirectories will be\n" .
    "  created automatically under the root output directory.\n\n" .
    "  - output-root - path of the root output directory\n" .
    "  - java-package-name - fully qualified package name (e.g. org.lightsys.centrallix.module.name)\n" .
    "  - header-files... - a space separated list of header files";
    exit(1);
}

my $rootDirectory = shift @ARGV || usage();
my $packageName = shift @ARGV || usage();
my $outputPath = $packageName;
$outputPath =~ s/\./\//g;
$outputPath = $rootDirectory . "/" . $outputPath;
mkdirs($outputPath);

# mappings like _WN => WgtrNode
my %shortToLongNames = loadShortNames();

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
            $type = fixTypeName($1);
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
            push @lines, "package $packageName;\n\n";
            push @lines, uniq(@imports);
            push @lines, "\n\n";
            push @lines, "public interface $structName {\n";
            push @lines, @members;
            push @lines, "}\n";
            dumpLines($structName, $outputPath);
            $inStruct = 0;
            @members = ();
            @imports = ();
        }
    }
    elsif(/^typedef struct([ \t]+([__a-zA-Z][__a-zA-Z0-9]*))?/){
        $inStruct = 1;
        $structName = $1;
    }
}

sub dumpLines {
    my $name = shift;
    my $path = shift;
    my $file = "$path/$name.java";
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
    elsif($type =~ /^(_[A-Z]+)\*/) { $type = $shortToLongNames{$1} || $1; }
    elsif($type =~ /^(_[A-Z]+)/) { $type = $shortToLongNames{$1} || $1; }
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

sub loadShortNames {
    open(my $fh, "<", "struct-short-names.txt") || die "Cannot open support file!";
    my @lines = grep { /./ } <$fh>; # remove empty lines
    my %h = map { chomp $_; split / /; } @lines;
    close($fh);
#    for my $key (sort keys %h){
#        print "$key => " . $h{$key} . "\n";
#    }
    return %h
}

sub mkdirs {
    my $path = shift;
    my @pathParts = split /\//, $path;
    my $currentPath = "";
    for $pathPart (@pathParts){
        $currentPath .= $pathPart;
        if( ! -d $currentPath ){
            mkdir $currentPath;
        }
        $currentPath .= "/";
    }
}
