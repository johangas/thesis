DESCRIPTION = "6LoWPAN ND (Neighbor Discovery)"
SECTION = "applications"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/BSD;md5=3775480a712fc46a69647678acb234cb"

# Source-dir is in the git directory.
S = "${WORKDIR}/git"

# git://git@git.yanzi.se/netcontiki.git;protocol=ssh
SRC_URI = "git://git@github.com/YanziNetworks/netcontiki.git;protocol=ssh;branch=dev/master-m3-scalability \
	  "
SRCREV = "${AUTOREV}"

RDEPENDS_${PN} = "kernel-module-cdc-acm bash python"

INHIBIT_PACKAGE_DEBUG_SPLIT = '1'

APPLICATION="6lowpan-nd-br"
INSTALL_ROOT="/opt/netcontiki"
SBINDIR="${INSTALL_ROOT}/sbin"
NBRDIR = "/yanzi/products/6lowpan-nd-br"

FILES_${PN} = "${SBINDIR} \
	       ${SBINDIR}/${APPLICATION} \
	      "

do_compile () {
    cd ${S}${NBRDIR}
    sed -i \
	-e 's@/usr/local/include@../usr/local/include/non-existant@' \
	../../../cpu/native/Makefile.native
    sed -i  \
	-e 's@CFLAGS+=@CFLAGS+= -I../../../core -D__UCLIBC_HAS_CONTEXT_FUNCS__  -D__LITTLE_ENDIAN__@' \
	-e 's/CC=/CC\?=/' \
	-e 's/LD=/LD\?=/' \
	Makefile

    make CC="$CC" LD="$CC" V=1 WITH_6LOWPAN_ND=1

## The generate executable by the Makefile is 'border-router.native'
	mv border-router.native ${APPLICATION}
}

do_install() {
	install -m 0755 -d	${D}${INSTALL_ROOT} \
				${D}${SBINDIR} 

	# Installation of 6LoWPAN ND border router binary 
	install -m 0755 ${S}${NBRDIR}/${APPLICATION}	${D}${SBINDIR}/${APPLICATION}
}
