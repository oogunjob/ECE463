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
        key(DEST ID): (cost, next hop)
        '''

        self.graph[self.addr] = (0, self.addr)

    def handlePacket(self, port, packet):
        """process incoming packet"""
        # default implementation sends packet back out the port it arrived
        # you should replace it with your implementation
        if packet.isData(): 
            # print(packet.srcAddr, packet.dstAddr)
            # print(self.addr, self.graph)

            if packet.dstAddr in self.graph and self.graph[packet.dstAddr][1] != "":
                # print("send")
                sending_port = 0
                for currport, link in self.links.items():
                    if link.get_e2(self.addr) == self.graph[packet.dstAddr][1]:
                        sending_port = currport
                        break
                self.send(sending_port, packet)

            # print(".......................................")

        else:
            flag = 0
            currentPacket = loads(packet.content)

            for router, cost_nextHop in currentPacket.items():
                if router not in self.graph:
                    new_cost = self.get_link_cost_helper(packet.srcAddr) + cost_nextHop[0]

                    if cost_nextHop[1] != self.addr:
                        if new_cost >= self.infinity:
                            # # self.graph[router] = (self.infinity, "")
                            # del self.graph[router]
                            pass
                        else:
                            self.graph[router] = (new_cost, packet.srcAddr)
                        flag = 1
                else:
                    if cost_nextHop[1] != self.addr:
                        new_cost = self.get_link_cost_helper(packet.srcAddr) + cost_nextHop[0]
                        #print(new_cost, self.graph[router][0])
                        if new_cost >= self.infinity:
                            # self.graph[router] = (self.infinity, "")
                            del self.graph[router]
                            flag = 1
                        elif new_cost < self.graph[router][0]:
                            new_info = (new_cost, packet.srcAddr)
                            self.graph[router] = new_info
                            flag = 1

                if flag == 1:
                    self.handlePeriodicOps()

            print(packet.srcAddr, packet.dstAddr, loads(packet.content))
            print("\n")
            print(self.addr, self.graph)
            print("\n")
            print(".......................................")
        
    def handleNewLink(self, port, endpoint, cost):
        """a new link has been added to switch port and initialized, or an existing
        link cost has been updated. Implement any routing/forwarding action that
        you might want to take under such a scenario"""

        routersToRemove = []
        for router, info in self.graph.items():
            if info[1] == endpoint:
                routersToRemove.append(router)

        for router in routersToRemove:
            del self.graph[router]


        self.graph[endpoint] = (cost, endpoint)

        for currPort, link in self.links.items():
            packet = Packet(2, self.addr, link.get_e2(self.addr), dumps(self.graph))
            self.send(currPort, packet)

        # print(self.addr)
        # self.handlePeriodicOps()
        pass


    def handleRemoveLink(self, port, endpoint):
        """an existing link has been removed from the switch port. Implement any 
        routing/forwarding action that you might want to take under such a scenario"""

        routersToRemove = []
        for router, info in self.graph.items():
            if info[1] == endpoint:
                routersToRemove.append(router)

        for router in routersToRemove:
            del self.graph[router]

        # self.graph[endpoint] = (self.infinity, "")

        for currPort, link in self.links.items():
            packet = Packet(2, self.addr, link.get_e2(self.addr), dumps(self.graph))
            self.send(currPort, packet)

        print(self.addr, endpoint, self.graph)

        # self.handlePeriodicOps()

        pass


    def handlePeriodicOps(self):
        """handle periodic operations. This method is called every heartbeatTime.
        You can change the value of heartbeatTime in the json file"""

        # for port, link in self.links.items():
        #     neighbour = link.get_e2(self.addr)

        #     if neighbour not in self.graph:
        #         self.graph[neighbour] = (link.get_cost(), neighbour)

        for port, link in self.links.items():
            packet = Packet(2, self.addr, link.get_e2(self.addr), dumps(self.graph))
            self.send(port, packet)

    # replace with self.links[port].getcost()
    def get_link_cost_helper(self, destination):
        for port, link in self.links.items():
            if link.get_e2(self.addr) == destination:
                return link.cost
        return 0
