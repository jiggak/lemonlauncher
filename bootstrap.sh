test ! -f config.h.in && autoheader
aclocal -I m4
test ! -d config && mkdir config
automake --gnu --add-missing --copy
autoconf
