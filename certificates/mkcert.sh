#!/bin/bash
#
# Copyright (c) 2020 Rally Tactical Systems, Inc.
#

CA_NAME=${1}
CERT_NAME=${2}

function showSyntax()
{
    echo "usage: mkcert.sh <ca_name> <cert_name>";
}

if [[ "${CA_NAME}" == "" ]]; then
    echo "ERROR: No CA name provided"
    showSyntax
    exit 1
fi

if [[ "${CERT_NAME}" == "" ]]; then
    echo "ERROR: No cert name provided"
    showSyntax
    exit 1
fi

if [[ ! -f "${CA_NAME}.cert" ]]; then
    echo "-------------------------------------------------------------------------"
    echo "CREATING CA CERTIFICATE ${CA_NAME}.cert"
    echo "-------------------------------------------------------------------------"
    echo ""
    echo ""
    openssl genrsa -out ${CA_NAME}.key 2048
    openssl req -addext basicConstraints=critical,CA:TRUE -x509 -new -nodes -key ${CA_NAME}.key -sha256 -days 9999 -out ${CA_NAME}.cert
fi

echo "-------------------------------------------------------------------------"
echo "CREATING CERTIFICATE ${CERT_NAME}.cert"
echo "-------------------------------------------------------------------------"
echo ""
echo ""
openssl genrsa -out ${CERT_NAME}.key
openssl req -new -key ${CERT_NAME}.key -out ${CERT_NAME}.csr
openssl x509 -req -in ${CERT_NAME}.csr -CA ${CA_NAME}.cert -CAkey ${CA_NAME}.key -CAcreateserial -out ${CERT_NAME}.cert -days 9999 -sha256
rm -f *.csr
rm -f *.srl
