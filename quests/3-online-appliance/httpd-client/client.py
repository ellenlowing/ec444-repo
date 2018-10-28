from __future__ import print_function
from __future__ import unicode_literals
from future import standard_library
standard_library.install_aliases()
from builtins import str
import http.client
import argparse
import time

def put_handler(ip, espport, nodeport):
    # Establish HTTP connection
    print("Connecting to => " + ip + ":" + espport)
    sess = http.client.HTTPConnection(ip + ":" + espport, timeout = 15)
    node = http.client.HTTPConnection(ip + ":" + nodeport, timeout = 15)

    while True:
        # Get button state from node server: "1" means LED ON, "0" means OFF
        try:
            node.request("GET", url="/ctrl")
            resp = node.getresponse()
            btnState = resp.read()
            # Put current button state to esp session
            sess.request("PUT", url="/ctrl", body=btnState)
            resp = sess.getresponse()
            resp.read()
        except KeyboardInterrupt:
            print("Quitting . . .")

    # Close HTTP connection
    sess.close()

if __name__ == '__main__':
    # Configure argument parser
    parser = argparse.ArgumentParser(description='Run HTTPd Test')
    parser.add_argument('IP'  , metavar='IP'  ,    type=str, help='Server IP')
    parser.add_argument('espport', metavar='espport',    type=str, help='ESP port')
    parser.add_argument('nodeport', metavar='nodeport',    type=str, help='Node port')
    args = vars(parser.parse_args())

    # Get arguments
    ip   = args['IP']
    espport = args['espport']
    nodeport = args['nodeport']

    # Call PUT handler
    put_handler (ip, espport, nodeport)
