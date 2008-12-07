#
## clean up all bits of libretroshare
echo make clobber 
make clobber 
#
## make the standard version (PPC)
echo make librs
make librs
#
LIB=libretroshare.a
LIB_PPC=libretroshare_ppc.a
LIB_X86=libretroshare_x86.a

MAC_SCRIPT="./scripts/config-macosx.mk"

echo cp lib/$LIB lib/$LIB_PPC librs
cp lib/$LIB lib/$LIB_PPC librs

echo make clobber
make clobber

echo make "MAC_I386_BUILD=1" librs
make "MAC_I386_BUILD=1" librs

echo cp lib/$LIB lib/$LIB_X86
cp lib/$LIB lib/$LIB_X86

# magic combine trick.
echo cd lib
cd lib

echo lipo -create libretroshare_*.a  -output $LIB
lipo -create libretroshare_*.a  -output $LIB


