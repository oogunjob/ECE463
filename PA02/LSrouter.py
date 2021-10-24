# The code is subject to Purdue University copyright policies.
# DO NOT SHARE, DISTRIBUTE, OR POST ONLINE
#

import sys
from collections import defaultdict
from router import Router
from packet import Packet
from json import dumps, loads
from heapq import heappush, heappop, heapify


class PQEntry:

    def __init__(self, addr, cost, next_hop):
        self.addr = addr
        self.cost = cost
        self.next_hop = next_hop

    def __lt__(self, other):
         return (self.cost < other.cost)

    def __eq__(self, other):
         return (self.cost == other.cost)


class LSrouter(Router):
    """Link state routing and forwarding implementation"""

    def __init__(self, addr, heartbeatTime):
        Router.__init__(self, addr, heartbeatTime)  # initialize superclass - don't remove
        self.graph = {} # A dictionary with KEY = router
                        # VALUE = a list of lists of all its neighbor routers/clients and the cost to each neighbor
                        # {router: [[neighbor_router_or_client, cost]]}
        self.graph[self.addr] = []
        """add your own class fields and initialization code here"""
        self.routingTable = {}
        self.seqNum = 0
        self.highestseqNum = defaultdict(int)

        self.seqNum = 0
        self.highestseqNum = defaultdict(int)


    def handlePacket(self, port, packet):
        """process incoming packet"""
        # default implementation sends packet back out the port it arrived
        # you should replace it with your implementation
        if packet.isData():
            
            if packet.dstAddr in self.routingTable:
                sending_port = 0
                for currport, link in self.links.items():
                    if link.get_e2(self.addr) == self.routingTable[packet.dstAddr][0]:
                        sending_port = currport
                        break
                self.send(sending_port, packet)
            pass
        else:
            lsa, seq = loads(packet.content)
            if self.highestseqNum[packet.srcAddr] < seq:
                self.highestseqNum[packet.srcAddr] = seq

                # update x view
                self.graph[packet.srcAddr] = lsa

                # send on every port except arrived port
                for currport in self.links:
                    if currport != port:
                        self.send(currport, packet)

                # calculate shortest path
                new_path = self.dijkstra()
                new_table = {}

                found = set()
                for item in new_path:
                    if item.addr not in found:
                        self.routingTable[item.addr] = (item.next_hop, item.cost)
                        found.add(item.addr)
                    elif self.routingTable[item.addr][1] > item.cost:
                        self.routingTable[item.addr] = (item.next_hop, item.cost)

    def handleNewLink(self, port, endpoint, cost):
        """a new link has been added to switch port and initialized, or an existing
        link cost has been updated. Implement any routing/forwarding action that
        you might want to take under such a scenario"""
        for neighbor in self.graph[self.addr]:
            if neighbor[0] == endpoint:
                self.graph[self.addr].remove(neighbor)

        self.graph[self.addr].append([endpoint,cost])
        
        updated_path = self.dijkstra()
        for item in updated_path:   
            self.routingTable[item.addr] = (item.next_hop, item.cost)

        # dikstra 
        self.handlePeriodicOps()

    def handleRemoveLink(self, port, endpoint):
        """an existing link has been removed from the switch port. Implement any 
        routing/forwarding action that you might want to take under such a scenario"""
        for neighbor in self.graph[self.addr]:
            if neighbor[0] == endpoint:
                self.graph[self.addr].remove(neighbor)

        updated_path = self.dijkstra()
        for item in updated_path:   
            self.routingTable[item.addr] = (item.next_hop, item.cost)

        self.handlePeriodicOps()
        #update the routing table

    def handlePeriodicOps(self):
        """handle periodic operations. This method is called every heartbeatTime.
        You can change the value of heartbeatTime in the json file"""
        self.graph[self.addr] = []

        for port, link in self.links.items():
            self.graph[self.addr].append([link.get_e2(self.addr), link.get_cost()])


        info = (self.graph[self.addr], self.seqNum)
        for port, link in self.links.items():
            packet = Packet(2, self.addr, link.get_e2(self.addr), dumps(info))
            self.send(port, packet)
        self.seqNum += 1


    def dijkstra(self):
        """An implementation of Dijkstra's shortest path algorithm.
        Operates on self.graph datastructure and returns the cost and next hop to
        each destination router in the graph as a List (finishedQ) of type PQEntry"""
        priorityQ = []
        finishedQ = [PQEntry(self.addr, 0, self.addr)]
        for neighbor in self.graph[self.addr]:
            heappush(priorityQ, PQEntry(neighbor[0], neighbor[1], neighbor[0]))

        while len(priorityQ) > 0:
            dst = heappop(priorityQ)
            finishedQ.append(dst)
            if not(dst.addr in self.graph.keys()):
                continue
            for neighbor in self.graph[dst.addr]:
                #neighbor already exists in finnishedQ
                found = False
                for e in finishedQ:
                    if e.addr == neighbor[0]:
                        found = True
                        break
                if found:
                    continue
                newCost = dst.cost + neighbor[1]
                #neighbor already exists in priorityQ
                found = False
                for e in priorityQ:
                    if e.addr == neighbor[0]:
                        found = True
                        if newCost < e.cost:
                            del e
                            heapify(priorityQ)
                            heappush(priorityQ, PQEntry(neighbor[0], newCost, dst.next_hop))
                        break
                if not found:
                    heappush(priorityQ, PQEntry(neighbor[0], newCost, dst.next_hop))

        return finishedQ

