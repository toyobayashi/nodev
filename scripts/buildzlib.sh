cd ./deps/zlib
chmod +x ./configure
./configure
make
cd ../..
mkdir -p ./lib
cp -rfp ./deps/zlib/libz.a ./lib/libz.a
cd ./deps/zlib
make clean
git checkout .
cd ../..
