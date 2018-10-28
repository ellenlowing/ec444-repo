#!/usr/bin/env python
#
# Copyright 2018 Espressif Systems (Shanghai) PTE LTD
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from __future__ import print_function
from __future__ import unicode_literals
from future import standard_library
standard_library.install_aliases()
from builtins import str
import http.client
import argparse

def test_get_handler(ip, port):
    # verbose_print(verbosity, "========  GET HANDLER TEST =============")
    print("========  GET HANDLER TEST =============")
    # Establish HTTP connection
    # verbose_print(verbosity, "Connecting to => " + ip + ":" + port)
    print("Connecting to => " + ip + ":" + port)
    sess = http.client.HTTPConnection(ip + ":" + port, timeout = 15)

    uri = "/hello?query1=value1&query2=value2&query3=value3"
    # GET hello response
    test_headers = {"Test-Header-1":"Test-Value-1", "Test-Header-2":"Test-Value-2"}
    # verbose_print(verbosity, "Sending GET to URI : ", uri)
    # verbose_print(verbosity, "Sending additional headers : ")
    print("Sending GET to URI : " + uri)
    print("Sending additional headers : ")

    for k, v in test_headers.items():
        # verbose_print(verbosity, "\t", k, ": ", v)
        print("\t" + k + ": " + v)
    sess.request("GET", url=uri, headers=test_headers)
    resp = sess.getresponse()
    resp_hdrs = resp.getheaders()
    resp_data = resp.read().decode()
    try:
        if resp.getheader("Custom-Header-1") != "Custom-Value-1":
            return False
        if resp.getheader("Custom-Header-2") != "Custom-Value-2":
            return False
    except:
        return False

    # verbose_print(verbosity, "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv")
    # verbose_print(verbosity, "Server response to GET /hello")
    # verbose_print(verbosity, "Response Headers : ")
    print("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv")
    print("Server response to GET /hello")
    print("Response Headers : ")
    for k, v in resp_hdrs:
        # verbose_print(verbosity, "\t", k, ": ", v)
        print("\t" + k + ": " + v)
    # verbose_print(verbosity, "Response Data : " + resp_data)
    # verbose_print(verbosity, "========================================\n")
    print("Response Data : " + resp_data)
    print("========================================\n")

    # Close HTTP connection
    sess.close()
    return (resp_data == "Hello World!")

def test_post_handler(ip, port, msg):
    # verbose_print(verbosity, "========  POST HANDLER TEST ============")
    print("========  POST HANDLER TEST ============")
    # Establish HTTP connection
    # verbose_print(verbosity, "Connecting to => " + ip + ":" + port)
    print("Connecting to => " + ip + ":" + port)
    sess = http.client.HTTPConnection(ip + ":" + port, timeout = 15)

    # POST message to /echo and get back response
    sess.request("POST", url="/echo", body=msg)
    resp = sess.getresponse()
    resp_data = resp.read().decode()
    # verbose_print(verbosity, "Server response to POST /echo (" + msg + ")")
    # verbose_print(verbosity, "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv")
    # verbose_print(verbosity, resp_data)
    # verbose_print(verbosity, "========================================\n")
    print("Server response to POST /echo (" + msg + ")")
    print("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv")
    print(resp_data)
    print("========================================\n")

    # Close HTTP connection
    sess.close()
    return (resp_data == msg)

def test_put_handler(ip, port):
    # verbose_print(verbosity, "========  PUT HANDLER TEST =============")
    print("========  PUT HANDLER TEST =============")
    # Establish HTTP connection
    # verbose_print(verbosity, "Connecting to => " + ip + ":" + port)
    print("Connecting to => " + ip + ":" + port)
    sess = http.client.HTTPConnection(ip + ":" + port, timeout = 15)

    # PUT message to /ctrl to disable /hello URI handler
    # verbose_print(verbosity, "Disabling /hello handler")
    print("Disabling /hello handler")
    sess.request("PUT", url="/ctrl", body="0")
    resp = sess.getresponse()
    resp.read()

    sess.request("GET", url="/hello")
    resp = sess.getresponse()
    resp_data1 = resp.read().decode()
    # verbose_print(verbosity, "Response on GET /hello : " + resp_data1)
    print("Response on GET /hello : " + resp_data1)

    # PUT message to /ctrl to enable /hello URI handler
    # verbose_print(verbosity, "Enabling /hello handler")
    print("Enabling /hello handler")
    sess.request("PUT", url="/ctrl", body="1")
    resp = sess.getresponse()
    resp.read()

    sess.request("GET", url="/hello")
    resp = sess.getresponse()
    resp_data2 = resp.read().decode()
    # verbose_print(verbosity, "Response on GET /hello : " + resp_data2)
    print("Response on GET /hello : " + resp_data2)

    # Close HTTP connection
    sess.close()
    return ((resp_data2 == "Hello World!") and (resp_data1 == "This URI doesn't exist"))

def test_custom_uri_query(ip, port, query):
    # verbose_print(verbosity, "========  GET HANDLER TEST =============")
    print("========  GET HANDLER TEST =============")
    # Establish HTTP connection
    # verbose_print(verbosity, "Connecting to => " + ip + ":" + port)
    print("Connecting to => " + ip + ":" + port)
    sess = http.client.HTTPConnection(ip + ":" + port, timeout = 15)

    uri = "/hello?" + query
    # GET hello response
    # verbose_print(verbosity, "Sending GET to URI : ", uri)
    print("Sending GET to URI : " + uri)
    sess.request("GET", url=uri, headers={})
    resp = sess.getresponse()
    resp_data = resp.read().decode()

    # verbose_print(verbosity, "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv")
    # verbose_print(verbosity, "Server response to GET /hello")
    # verbose_print(verbosity, "Response Data : " + resp_data)
    # verbose_print(verbosity, "========================================\n")
    print("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv")
    print("Server response to GET /hello")
    print("Response Data : " + resp_data)
    print("========================================\n")

    # Close HTTP connection
    sess.close()
    return (resp_data == "Hello World!")

if __name__ == '__main__':
    # Configure argument parser
    parser = argparse.ArgumentParser(description='Run HTTPd Test')
    parser.add_argument('IP'  , metavar='IP'  ,    type=str, help='Server IP')
    parser.add_argument('port', metavar='port',    type=str, help='Server port')
    # parser.add_argument('msg',  metavar='message', type=str, help='Message to be sent to server')
    args = vars(parser.parse_args())

    # Get arguments
    ip   = args['IP']
    port = args['port']
    # msg  = args['msg']

    if not test_get_handler (ip, port):
        # Utility.console_log("Failed!")
        print("Failed!")
    # if not test_post_handler(ip, port, msg):
    #     # Utility.console_log("Failed!")
    #     print("Failed!")
    if not test_put_handler (ip, port):
        # Utility.console_log("Failed!")
        print("Failed!")
