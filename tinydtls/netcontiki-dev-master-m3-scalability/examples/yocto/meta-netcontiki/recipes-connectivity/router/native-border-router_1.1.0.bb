DESCRIPTION = "Native Border Router for IPv6"
SECTION = "applications"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/BSD;md5=3775480a712fc46a69647678acb234cb"

# Source-dir is in the git directory.
S = "${WORKDIR}/git"

# git://git@git.yanzi.se/netcontiki.git;protocol=ssh
SRC_URI = "git://git@github.com/YanziNetworks/netcontiki.git;protocol=ssh;branch=master \
	   file://startBorderRouter.sh \
	   file://99-br.rules \
	  "
SRCREV = "${AUTOREV}"

RDEPENDS_${PN} = "kernel-module-cdc-acm bash python"

INHIBIT_PACKAGE_DEBUG_SPLIT = '1'

APPLICATION="border-router.native"
INSTALL_ROOT="/opt/netcontiki"
SBINDIR="${INSTALL_ROOT}/sbin"
TOOLDIR = "/tools/yanzi"
DEMODIR = "/examples/galileo/demo"
TESTDIR = "/examples/galileo/tests"
NBRDIR = "/yanzi/products/yanzi-border-router"
OLDNBRDIR = "/examples/ipv6/native-border-router"

FILES_${PN} = "${SBINDIR} \
	       ${SBINDIR}/${APPLICATION} \
	       ${SBINDIR}/startBorderRouter.sh \
	      "
#	       ${UDEVRULES}
#	       ${UDEVRULES}/99-br.rules

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

# To build NBR for SPI SLIP radio, use this line
#    make CC="$CC" LD="$CC" V=1 WITH_SPI=1
#
# To build NBR for USB SLIP radio, use this line
    make CC="$CC" LD="$CC" V=1 

}

do_install() {
	install -m 0755 -d	${D}${INSTALL_ROOT} \
				${D}${SBINDIR} \
				${D}${SBINDIR}${DEMODIR}/www \
				${D}${SBINDIR}${TESTDIR} \
				${D}${SBINDIR}${TOOLDIR}/tlvscripts

	# Installation of border router binary and platform shell scripts
	install -m 0755 ${S}${NBRDIR}/border-router.native	${D}${SBINDIR}
	install -m 0774 ${S}${OLDNBRDIR}/*.sh			${D}${SBINDIR}

	# Installation of Netcontiki demo scripts and web files for demo
	install -m 0774 ${S}${DEMODIR}/*.py			${D}${SBINDIR}${DEMODIR}
	install -m 0664 ${S}${DEMODIR}/www/*			${D}${SBINDIR}${DEMODIR}/www

	# Installation of Netcontiki test scripts
	install -m 0774 ${S}${TESTDIR}/*			${D}${SBINDIR}${TESTDIR}

	# Installation of Netcontiki tool scripts and files
	install -m 0774 ${S}${TOOLDIR}/*.py			${D}${SBINDIR}${TOOLDIR}
	install -m 0774 ${S}${TOOLDIR}/*.sh			${D}${SBINDIR}${TOOLDIR}
	install -m 0664 ${S}${TOOLDIR}/*.jar			${D}${SBINDIR}${TOOLDIR}
	install -m 0774 ${S}${TOOLDIR}/tlvscripts/*.py		${D}${SBINDIR}${TOOLDIR}/tlvscripts
	install -m 0664 ${S}${TOOLDIR}/tlvscripts/README	${D}${SBINDIR}${TOOLDIR}/tlvscripts
}
