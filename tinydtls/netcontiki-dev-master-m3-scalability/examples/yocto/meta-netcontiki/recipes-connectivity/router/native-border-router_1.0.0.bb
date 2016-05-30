DESCRIPTION = "Native Border Router for IPv6"
SECTION = "applications"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://LICENSE;md5=b2276b027815460f098d51494e2ff4f1"

# git://git@git.yanzi.se/netcontiki.git;protocol=ssh
SRC_URI = "git://git@github.com/YanziNetworks/netcontiki.git;protocol=ssh;branch=master \
	   file://startBorderRouter.sh \
	   file://99-br.rules \
	  "
SRCREV = "${AUTOREV}"

PACKAGES = "${PN}"
INHIBIT_PACKAGE_DEBUG_SPLIT = '1'

APPLICATION="border-router.native"
INSTALL_ROOT="/opt/netcontiki"
SBINDIR="${INSTALL_ROOT}/sbin"
UDEVRULES="/etc/udev/rules.d"
FILES_${PN} = "${SBINDIR} \
	       ${SBINDIR}/${APPLICATION} \
	       ${SBINDIR}/startBorderRouter.sh \
	      "
#	       ${UDEVRULES}
#	       ${UDEVRULES}/99-br.rules

# Source-dir is in the git directory.
S = "${WORKDIR}/git"

NBR = "${S}/examples/ipv6/native-border-router"

do_compile () {
    cd ${NBR}
    sed -i \
    	-e 's@/usr/local/include@../usr/local/include/non-existant@' \
	../../../cpu/native/Makefile.native
    sed -i  \
    	-e 's@CFLAGS+=@CFLAGS+= -I../../../core -D__UCLIBC_HAS_CONTEXT_FUNCS__  -D__LITTLE_ENDIAN__@' \
    	-e 's/CC=/CC\?=/' \
    	-e 's/LD=/LD\?=/' \
	Makefile

# To build NBR for SPI SLIP radio, use this line
    make CC="$CC" LD="$CC" V=1 WITH_SPI=1
#
# To build NBR for USB SLIP radio, use this line
#    make CC="$CC" LD="$CC" V=1 

}

do_install() {
    install -d 0755 ${D}/${INSTALL_ROOT}
    install -d 0755 ${D}/${SBINDIR}
    install -m 0755 ${NBR}/${APPLICATION} ${D}/${SBINDIR}/${APPLICATION}

    install -d 0755 ${D}/${SBINDIR}
    install -m 0744 ${WORKDIR}/startBorderRouter.sh ${D}/${SBINDIR}/startBorderRouter.sh

    install -m 0755 ${NBR}/intel_galileo_gpio_mux.sh ${D}/${SBINDIR}/intel_galileo_gpio_mux.sh

#    install -d 0755 ${D}/${UDEVRULES}
#    install -m 0644 ${WORKDIR}/99-br.rules ${D}/${UDEVRULES}/99-br.rules
}
