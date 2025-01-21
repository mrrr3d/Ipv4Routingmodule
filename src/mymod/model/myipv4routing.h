#ifndef MYIPV4ROUTING_H
#define MYIPV4ROUTING_H

#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/names.h"
#include "ns3/node-container.h"
#include "ns3/simulator.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"

#include <cstdint>

namespace ns3
{

class myipv4routing : public Ipv4RoutingProtocol
{
  public:
    static TypeId GetTypeId();
    myipv4routing();
    ~myipv4routing() override;
    Ptr<Ipv4Route> RouteOutput(Ptr<Packet> p,
                               const Ipv4Header& header,
                               Ptr<NetDevice> oif,
                               Socket::SocketErrno& sockerr) override;
    bool RouteInput(Ptr<const Packet> p,
                    const Ipv4Header& ipHeader,
                    Ptr<const NetDevice> idev,
                    const UnicastForwardCallback& ucb,
                    const MulticastForwardCallback& mcb,
                    const LocalDeliverCallback& lcb,
                    const ErrorCallback& ecb) override;
    void NotifyInterfaceUp(uint32_t interface) override;
    void NotifyInterfaceDown(uint32_t interface) override;
    void NotifyAddAddress(uint32_t interface, Ipv4InterfaceAddress address) override;
    void NotifyRemoveAddress(uint32_t interface, Ipv4InterfaceAddress address) override;
    void SetIpv4(Ptr<Ipv4> ipv4) override;
    void PrintRoutingTable(Ptr<OutputStreamWrapper> stream,
                           Time::Unit unit = Time::S) const override;

    Ptr<Ipv4Route> Lookupsw(Ipv4Address dest, Ptr<NetDevice> oif = nullptr);
    void AddNetworkRouteTo(Ipv4Address network,
                           Ipv4Mask networkMask,
                           Ipv4Address nextHop,
                           uint32_t interface,
                           uint32_t metric = 0);

    void AddNetworkRouteTo(Ipv4Address network,
                           Ipv4Mask networkMask,
                           uint32_t interface,
                           uint32_t metric = 0);
    bool LookupRoute(const Ipv4RoutingTableEntry& route, uint32_t metric);

    uint32_t GetNRoutes() const;
    Ipv4RoutingTableEntry* GetRoute(uint32_t i) const;
    uint32_t GetMetric(uint32_t index) const;

  protected:
    void DoDispose() override;

  private:
    typedef std::list<std::pair<Ipv4RoutingTableEntry*, uint32_t>> NetworkRoutes;
    typedef std::list<std::pair<Ipv4RoutingTableEntry*, uint32_t>>::const_iterator NetworkRoutesCI;
    typedef std::list<std::pair<Ipv4RoutingTableEntry*, uint32_t>>::iterator NetworkRoutesI;
    Ptr<Ipv4> m_ipv4;
    NetworkRoutes m_networkroutes;
};
} // namespace ns3

#endif