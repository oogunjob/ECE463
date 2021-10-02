# The code is subject to Purdue University copyright policies.
# DO NOT SHARE, DISTRIBUTE, OR POST ONLINE
#

import sys
import time
from switch import Switch
from link import Link
from client import Client
from packet import Packet


class STPswitch(Switch):
    """MAC learning and forwarding implementation."""

    def __init__(self, addr, heartbeatTime):
        Switch.__init__(self, addr, heartbeatTime)  # initialize superclass - don't remove
        """TODO: add your own class fields and initialization code here"""


    def handlePacket(self, port, packet):
        """TODO: process incoming packet"""
        # default implementation sends packet back out the port it arrived
        self.send(port, packet)


    def handleNewLink(self, port, endpoint, cost):
        """TODO: handle new link"""
        pass


    def handleRemoveLink(self, port, endpoint):
        """TODO: handle removed link"""
        pass


    def handlePeriodicOps(self, currTimeInMillisecs):
        """TODO: handle periodic operations. This method is called every heartbeatTime.
        You can change the value of heartbeatTime in the json file."""
        pass

