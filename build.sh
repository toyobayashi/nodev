type="Release"
dll="OFF"
test="false"

until [ $# -eq 0 ]
do
if [ "$1" == "Release" ]; then type="$1"; fi
if [ "$1" == "Debug" ]; then type="$1"; fi
if [ "$1" == "dll" ]; then dll="ON"; fi
if [ "$1" == "test" ]; then test="true"; fi
shift
done

unamestr=`uname`
os=`echo $unamestr | tr "A-Z" "a-z"`

mkdir -p "./build/$os/$type"
cd "./build/$os/$type"
echo "cmake -DBUILD_SHARED_LIBS=$dll -DCCPM_BUILD_TEST=$test -DBUILD_TESTING="$test" -DCMAKE_BUILD_TYPE=$type -DCCPM_UNAME=$os ../../.."
cmake -DBUILD_SHARED_LIBS="$dll" -DCCPM_BUILD_TEST="$test" -DBUILD_TESTING="$test" -DCMAKE_BUILD_TYPE=$type -DCCPM_UNAME=$os ../../..
cmake --build .
cd ../../..

if [ "$type" == "Release" ]; then
  src_dir="./build/$os/Release"
  dest_dir="dist/$os/bin"
  # mkdir -p "dist/$os/lib"
  mkdir -p $dest_dir
  # cp "$src_dir"/*.a "dist/$os/lib"
  # cp "$src_dir"/{*.so,*.dylib} $dest_dir

  cp -rfp "$src_dir/nodev" "$dest_dir/nodev-$os"

  # for f in `find $src_dir -maxdepth 1 -type f`;
  # do
  #   if test -x $f; then cp $f $dest_dir; fi
  # done
fi
