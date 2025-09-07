#!/bin/bash
WEB_DOMAIN="www.iyxsh.com"
CERT_DIR="/root/.acme.sh/${WEB_DOMAIN}_ecc"
SERVER_DIR="/home/lifan/app/LibreofficeFun/server"
if [ -d "$CERT_DIR" ]; then
    rm -rf "$CERT_DIR"
fi

cd ~
/root/.acme.sh/acme.sh --issue --dns dns_ali -d "$WEB_DOMAIN"

if [ -f "$CERT_DIR/fullchain.cer" ] && [ -f "$CERT_DIR/${WEB_DOMAIN}.key" ]; then
    cp "$CERT_DIR/fullchain.cer" "$SERVER_DIR/bin/server.crt"
    cp "$CERT_DIR/${WEB_DOMAIN}.key" "$SERVER_DIR/bin/server.key"
    cp "$CERT_DIR/ca.cer" "$SERVER_DIR/bin/ca.crt"
fi