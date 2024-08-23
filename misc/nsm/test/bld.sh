#!/bin/bash

RTS_VERSION="1.0.0.0"
RTS_NSM_ITERATION="2"
RTS_PRODUCTS_ROOT="/mnt/hgfs/Global/github/rallytac/products/src"
RTS_BUILD_ROOT="/mnt/hgfs/Global/github/rallytac/products/src"

function package_nsm()
{
    function _internal_create_container_nsm()
    {
        echo "TODO: _internal_create_container_nsm"
    }

	function _internal_package_nsm()
	{
		rm -rf *.${1}
		fpm -s dir \
			-t ${1} \
			-v ${RTS_VERSION} \
			-m "support@rallytac.com" \
			--iteration ${RTS_NSM_ITERATION} \
			--license MIT \
			--vendor "Rally Tactical Systems, Inc." \
			--url "https://www.rallytac.com" \
			--description "The Network State Machine from Rally Tactical Systems." \
			--rpm-summary "Network State Machine" \
			--verbose \
			--before-install ${RTS_PRODUCTS_ROOT}/fpmscripts/nsm-before-install.sh \
			--before-remove ${RTS_PRODUCTS_ROOT}/fpmscripts/nsm-before-remove.sh \
			--after-install ${RTS_PRODUCTS_ROOT}/fpmscripts/nsm-after-install.sh \
            --before-upgrade ${RTS_PRODUCTS_ROOT}/fpmscripts/nsm-before-upgrade.sh \
            --after-upgrade ${RTS_PRODUCTS_ROOT}/fpmscripts/nsm-after-upgrade.sh \
			--deb-no-default-config-files \
			-n nsm \
            ${2} \
			./
	}

	CURRDIR=`pwd`
    WRKDIR=`mktemp -d`
    ARTDIR="${RTS_BUILD_ROOT}/artifacts"

    cd "${WRKDIR}"

	mkdir -p etc/nsm
    cp "${RTS_PRODUCTS_ROOT}/nsm/_nsm.py" etc/nsm
	cp "${RTS_PRODUCTS_ROOT}/nsm/nsm_conf.json" etc/nsm
    cp "${RTS_PRODUCTS_ROOT}/nsm/beforeActive.sh" etc/nsm
    cp "${RTS_PRODUCTS_ROOT}/nsm/beforeGoingActive.sh" etc/nsm
    cp "${RTS_PRODUCTS_ROOT}/nsm/onActive.sh" etc/nsm
    cp "${RTS_PRODUCTS_ROOT}/nsm/onGoingActive.sh" etc/nsm
    cp "${RTS_PRODUCTS_ROOT}/nsm/onIdle.sh" etc/nsm

	mkdir -p etc/systemd/system
	cp "${RTS_PRODUCTS_ROOT}/nsm/nsm.service" etc/systemd/system

	mkdir -p usr/sbin
	cp "${RTS_PRODUCTS_ROOT}/nsm/nsm" usr/sbin
    
	mkdir -p usr/share/man/man8
	gzip -c "${RTS_PRODUCTS_ROOT}/nsm/doc/nsm.8" > usr/share/man/man8/nsm.8.gz

	_internal_package_nsm "rpm" "${RTS_PKG_EXTRA_RPM}"
    echo "" | setsid rpm --addsign *.rpm
    cp *.rpm "${ARTDIR}"
    rm -f *.rpm

	_internal_package_nsm "deb" "${RTS_PKG_EXTRA_DEB}"
	cp *.deb "${ARTDIR}"
    rm -f *.deb

    _internal_create_container_nsm

	cd ${CURRDIR}
	rm -rf "${WRKDIR}"
}

package_nsm