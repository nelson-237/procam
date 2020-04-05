# make
#
# gcc -O2  -I.   -c -o jpegmorph.o jpegmorph.c
# gcc  -o jpegmorph jpegmorph.o rdswitch.o cdjpeg.o transupp.o libjpeg.a
#
# gcc -O2  -I.   -c -o jpegremorph.o jpegremorph.c
# gcc  -o jpegremorph jpegremorph.o rdswitch.o cdjpeg.o transupp.o libjpeg.a

gcc -O2 -I. -c -o jptrsmorph.o jptrsmorph.c -lm
echo "Compiling 1:  gcc -O2 -I. -c -o jptrsmorph.o jptrsmorph.c"
gcc -o jptrsmorph jptrsmorph.o src/rdswitch.o src/cdjpeg.o src/transupp.o src/libjpeg.a -lm
echo "Compiling 2:  gcc -o jptrsmorph jptrsmorph.o src/rdswitch.o src/cdjpeg.o src/transupp.o src/libjpeg.a"