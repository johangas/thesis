DESCRIPTION = "tinydtls -- a very basic DTLS implementation" 
SECTION = "applications"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "files://dtls.c;beginline=6;endline=24;md5=9e47ecd5adae0437851500056eff9139"

# http://sourceforge.net/projects/tinydtls/files/r5/tinydtls-0.8.2.tar.gz
SRC_URI = "http://sourceforge.net/projects/tinydtls/files/r5/tinydtls-0.8.2.tar.gz \
	   file://tinydtls-0.8.2.patch "
SRC_URI[md5sum] = "1e660e082a1d0367313e4db1e8ea7294"
SRC_URI[sha256sum] = "ccf6d8fbae03fb2e0ba32878ed8e57d8b4f73538b1064df90a3e764da5fac010"

SRCREV = "${AUTOREV}"

inherit autotools

PACKAGES = "${PN}"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"

INSTALL_ROOT="/usr"
BINDIR="${INSTALL_ROOT}/bin"
SHAREDIR="${INSTALL_ROOT}/share"
LIBDIR="${INSTALL_ROOT}/lib"
INCLUDEDIR="${INSTALL_ROOT}/include"
STATICLIB="libtinydtls.a"

FILES_${PN} = "${BINDIR} \
	       ${SHAREDIR} \
	       ${LIBDIR} \
	       ${INCLUDEDIR} \
	      "

## This does not seem to help with the statis library in non-static package issue
##FILES_${PN}--staticdev = "${LIBDIR}/${STATICLIB}"

## Disable the check of static library in a non-static package.
## This is probably not the right method to install a static-link library
INSANE_SKIP_${PN} += "staticdev"

# Source-dir is in the git directory.
S = "${WORKDIR}/tinydtls-0.8.2"
B = "${S}"

#EXTRA_OEMAKE = "'CC=${CC}' 'LD=${CC}' \
#                '-I${S}'"

do_configure() {
    cd ${S}
#    patch -p1 -i ${WORKDIR}/tinydtls-0.8.2.patch
    ./configure --prefix="${D}${INSTALL_ROOT}"
}
