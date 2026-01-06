echo "This is the monkeyiq config invoker. please don't use it."
export CXXFLAGS=" -Wtemplate-body -Wdeprecated-declarations "
/Develop/libferrisstreams/configure \
--disable-stlport --enable-debug   --enable-hiddensymbols \
--prefix=/usr/local --libdir=/usr/local/lib64

