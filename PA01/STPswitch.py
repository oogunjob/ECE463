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
        self.root = 0
        self.cost = 0
        self.original = 0
        self.hop = 0

    def handlePacket(self, port, packet):
        """TODO: process incoming packet"""
        # default implementation sends packet back out the port it arrived
        print(self.addr, packet.content)

        self.root = self.addr
        self.original = self.addr
        self.hop = self.addr

        packet_temp = [str(self.root), str(self.cost), str(self.original), str(self.hop)]
        control = Packet(2, packet.srcAddr,packet.dstAddr,",".join(packet_temp))


        self.send(port, control)


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

