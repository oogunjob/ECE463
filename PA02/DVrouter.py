# The code is subject to Purdue University copyright policies.
# DO NOT SHARE, DISTRIBUTE, OR POST ONLINE
#

import sys
from collections import defaultdict
from router import Router
from packet import Packet
from json import dumps, loads


class DVrouter(Router):
    """Distance vector routing and forwarding implementation"""

    def __init__(self, addr, heartbeatTime, infinity):
        Router.__init__(self, addr, heartbeatTime)  # initialize superclass - don't remove
        self.infinity = infinity
        """add your own class fields and initialization code here"""
        self.graph = {}

        '''
        key(DEST ID): (cost, next hop, full path)
        '''

        self.graph[self.addr] = (0, self.addr)

        # for port, link in self.links.items():
        #     e2 = link.get_e2(self.addr)
        #     self.graph[e2] = (self.get_cost(), e2, [self.addr, e2])



    def handlePacket(self, port, packet):
        """process incoming packet"""
        # default implementation sends packet back out the port it arrived
        # you should replace it with your implementation
        if packet.isData():
            for port, link in self.links.items():
                transferpacket = Packet(1, self.graph[packet.srcAddr][1], packet.dstAddr, packet.content)
                self.send(port, transferpacket)
            
        else:
            flag = 0
            currentPacket = loads(packet.content)

            for router, cost_nextHop in currentPacket.items():
                if router not in self.graph:
                    if cost_nextHop[1] != self.addr:
                        self.graph[router] = [self.get_link_cost_helper(packet.srcAddr) + cost_nextHop[0], packet.srcAddr]
                        flag = 1
                else:
                    if cost_nextHop[1] != self.addr:
                        new_cost = self.get_link_cost_helper(packet.srcAddr) + cost_nextHop[0]
                        #print(new_cost, self.graph[router][0])
                        if new_cost < self.graph[router][0]:
                            new_info = (new_cost, packet.srcAddr)
                            self.graph[router] = new_info
                            flag = 1

                if flag == 1:
                    for port, link in self.links.items():
                        newpacket = Packet(2, packet.dstAddr, link.get_e2(packet.dstAddr), dumps(self.graph))
                        self.send(port, newpacket)
                        pass

    
    def handleNewLink(self, port, endpoint, cost):
        """a new link has been added to switch port and initialized, or an existing
        link cost has been updated. Implement any routing/forwarding action that
        you might want to take under such a scenario"""

        '''
        
        '''

        pass


    def handleRemoveLink(self, port, endpoint):
        """an existing link has been removed from the switch port. Implement any 
        routing/forwarding action that you might want to take under such a scenario"""
        pass


    def handlePeriodicOps(self):
        """handle periodic operations. This method is called every heartbeatTime.
        You can change the value of heartbeatTime in the json file"""

        for port, link in self.links.items():
            neighbour = link.get_e2(self.addr)
            if neighbour not in self.graph and neighbour.isdigit():
                self.graph[neighbour] = (link.cost, neighbour)

        for port, link in self.links.items():
            packet = Packet(2, self.addr, link.get_e2(self.addr), dumps(self.graph))
            self.send(port, packet)

        pass

    def get_link_cost_helper(self, destination):
        for port, link in self.links.items():
            if link.get_e2(self.addr) == destination:
                return link.cost
        return 0
