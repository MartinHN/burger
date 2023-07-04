#!/usr/bin/python3

import http.server, ssl
import os

import subprocess

rootPath =os.path.abspath( os.path.dirname(__file__)+"/../")
cliPath = os.path.abspath(rootPath+"/cli")
# p1 = subprocess.run(rootPath+"/scripts/gen_cert.sh")

print(rootPath,cliPath)
os.chdir(cliPath)

# server_address = ('13.13.13.223', 8000)
server_address = ('0.0.0.0', 8000)
httpd = http.server.HTTPServer(server_address, http.server.SimpleHTTPRequestHandler)
httpd.socket = ssl.wrap_socket(httpd.socket,
                                server_side=True,
                                certfile='server.pem',
                                ssl_version=ssl.PROTOCOL_TLSv1_2
)

print ("serving "+ "https://"+str(server_address[0])+":"+str(server_address[1]))
httpd.serve_forever()
