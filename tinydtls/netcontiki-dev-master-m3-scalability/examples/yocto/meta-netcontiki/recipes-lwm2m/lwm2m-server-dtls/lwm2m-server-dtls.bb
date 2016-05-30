DESCRIPTION = "LWM2M Server"
SECTION = "applications"
LICENSE = "CLOSED"
DEPENDS = "tinydtls"

SRC_URI = "git://git@github.com/YanziNetworks/wakaama.git;protocol=ssh;branch=IPSO"

SRCREV = "${AUTOREV}"

PACKAGES = "${PN}"
INHIBIT_PACKAGE_DEBUG_SPLIT = '1'

APP_BUILD_NAME="lwm2mserver"
APPLICATION="lwm2mserver-dtls"
INSTALL_ROOT="/opt/netcontiki"
SBINDIR="${INSTALL_ROOT}/sbin"

FILES_${PN} = "${SBINDIR} \
               ${SBINDIR}/${APPLICATION} \
              "

S = "${WORKDIR}/git/tests/server"

inherit cmake

do_configure() {
    cmake -D DTLS=YES ${S}
}

do_install() {
    install -d 0755 ${D}/${INSTALL_ROOT}
    install -d 0755 ${D}/${SBINDIR}
#    install -m 0755 ${S}/${APP_BUILD_NAME} ${D}/${SBINDIR}/${APPLICATION}
    install -m 0755 ${WORKDIR}/build/${APP_BUILD_NAME} ${D}/${SBINDIR}/${APPLICATION}
}

