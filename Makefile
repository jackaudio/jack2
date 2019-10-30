.PHONY: all
all:
	CC=${QNX_HOST}/usr/bin/aarch64-unknown-nto-qnx7.0.0-gcc CXX=${QNX_HOST}/usr/bin/aarch64-unknown-nto-qnx7.0.0-g++ AR=${QNX_HOST}/usr/bin/aarch64-unknown-nto-qnx7.0.0-ar LDFLAGS="-L${INSTALL_ROOT_nto}/aarch64le/lib -L${INSTALL_ROOT_nto}/aarch64le/usr/lib -L${QNX_TARGET}/aarch64le/lib -L${QNX_TARGET}/aarch64le/usr/lib" PKG_CONFIG_LIBDIR=${INSTALL_ROOT_nto}/aarch64le/usr/lib/pkgconfig ./waf configure --platform=qnx --prefix=/usr --libdir=/usr/lib64
	./waf build
	./waf install --destdir=${INSTALL_ROOT_nto}/aarch64le

.PHONY: install
install: all

.PHONY:clean
clean:
	# ignore error codes otherwise it would for example fail if clean is called before configure
	-./waf clean

