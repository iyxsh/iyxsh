#!/bin/bash
# 生成CA证书
openssl req -x509 -newkey rsa:4096 -nodes -keyout ca.key -out ca.crt -days 3650 \
  -subj "/CN=MyLocalCA"

# 生成服务器证书
openssl req -newkey rsa:4096 -nodes -keyout server.key -out server.csr \
  -subj "/CN=internal-server" \
  -addext "subjectAltName=DNS:internal-server,DNS:localhost,IP:127.0.0.1"
  
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
  -out server.crt -days 365 -extfile <(printf "subjectAltName=DNS:internal-server,DNS:localhost,IP:127.0.0.1")

# 设置权限
sudo chmod 600 *.key
sudo chown $USER:$USER *.crt


# Generate client certificate
openssl req -newkey rsa:2048 -nodes -keyout client.key -out client.csr -subj "/CN=Client"
openssl x509 -req -in client.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out client.crt -days 365