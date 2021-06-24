export MACOSX_DEPLOYMENT_TARGET="10.6"

unamestr=`uname`
os=`echo $unamestr | tr "A-Z" "a-z"`

cd ./deps/curl
chmod +x ./buildconf ./configure
./buildconf
# sudo apt install libssl-dev -y
./configure --prefix=`pwd`/builds/"$os" --with-darwinssl --disable-shared --enable-static --disable-thread --without-libidn2 --without-brotli --without-librtmp --without-nghttp2 --disable-ldap --disable-ldaps
make
make install
cd ../..

mkdir -p ./lib
cp -rfp ./deps/curl/builds/"$os"/lib/libcurl.a ./lib/libcurl.a
