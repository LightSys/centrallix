
./get-struct-names.pl ../include/*.h ../../centrallix-lib/include/datatypes.h | sort > struct-short-names.txt

cat header-files.txt | grep -v '#' | while read package header; do
  ./convert-struct.pl src/main/java $package $header
done
