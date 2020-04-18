unamestr=`uname`
os=`echo $unamestr | tr "A-Z" "a-z"`

cd ./deps/curl
# sudo apt install libssl-dev -y
./configure --prefix=`pwd`/builds/"$os" --with-ssl # --disable-shared
make
make install
cd ../..
