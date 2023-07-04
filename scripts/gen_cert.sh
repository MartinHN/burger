#!/bin/bash
openssl req -new -x509 -keyout cli/server.pem -out cli/server.pem -days 365 -nodes -config scripts/ssl.cnf
