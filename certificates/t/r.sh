#!/bin/bash
function use_echo()
{
    openssl req \
            -sha1 \
            -x509 \
            -newkey rsa:2048 \
            -keyout private_key.pem \
            -out certificate.pem \
            -days 99999 \
            -nodes  \
            -subj "/C=BR/ST=DF/L=Distrito Federal/O=Brasilia/CN=Company name:88022130000176" \
            -extensions san \
            -config <(echo "[req]";echo distinguished_name=req;echo "[san]";echo extendedKeyUsage=clientAuth,emailProtection;echo subjectAltName=email:company@company.com.br, otherName:2.16.76.1.3.3\;UTF8:88022130000176;)
}

function use_file()
{
    openssl req \
            -sha1 \
            -x509 \
            -newkey rsa:2048 \
            -keyout private_key.pem \
            -out certificate.pem \
            -days 99999 \
            -nodes  \
            -subj "/C=US/ST=WA/L=Seattle/O=Brasilia/CN=T-Mobile USA:88022130000176" \
            -extensions san \
            -config myconfig.conf
}

function shaun_thing()
{
    openssl req \
        -sha256 \
        -nodes \
        -x509 \
        -extensions san

}

use_file
openssl x509 -text -in certificate.pem
