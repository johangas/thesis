DESCRIPTION = "tinydns - DNS server"
SECTION = "applications"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://README;md5=b96ee8c257dbd13132e48a5c31e6b20c"

SRC_URI = "http://cr.yp.to/djbdns/djbdns-1.05.tar.gz"
SRC_URI[md5sum] = "3147c5cd56832aa3b41955c7a51cbeb2"
SRC_URI[sha256sum] = "3ccd826a02f3cde39be088e1fc6aed9fd57756b8f970de5dc99fcd2d92536b48"

SRCREV = "${AUTOREV}"

PACKAGES = "${PN}"

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
#INHIBIT_PACKAGE_STRIP = "1"

APPLICATION = "dnscache-conf tinydns-conf walldns-conf rbldns-conf pickdns-conf axfrdns-conf \
	dnscache tinydns walldns rbldns pickdns axfrdns \
	tinydns-get tinydns-data tinydns-edit rbldns-data pickdns-data axfr-get   \
	dnsip dnsipq dnsname dnstxt dnsmx dnsfilter random-ip dnsqr dnsq \
	dnstrace dnstracesort \
	"
DNSROOT_GLOBAL = "dnsroots.global"

ETC="/etc"
INSTALL_ROOT="/usr/local"
BINDIR="${INSTALL_ROOT}/bin"

FILES_${PN} = "${BINDIR} \
	      ${ETC} \
	      "
# Source-dir after untar and unzip.
S = "${WORKDIR}/djbdns-1.05"

do_compile () {
    # The compile and link commands for tinydns (dbjdns) are written to "conf-cc" and "conf-ld"
    echo ${CC} -O2 -include /usr/include/errno.h > conf-cc
    echo ${CCLD} > conf-ld

    make
}

do_install() {
    install -d 0755 ${D}/${INSTALL_ROOT}
    install -d 0755 ${D}/${BINDIR}
    install -d 0755 ${D}/${ETC}
    
    install -m 0644 ${S}/dnsroots.global ${D}/${ETC}/

    for i in ${APPLICATION}; do \
	install -m 0755 ${S}/${i} ${D}/${BINDIR}; \
    done
}

