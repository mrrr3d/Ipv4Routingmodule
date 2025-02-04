#include "myipv4routing.h"

#include "ns3/output-stream-wrapper.h"

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("myipv4routing");

NS_OBJECT_ENSURE_REGISTERED(myipv4routing);

TypeId
myipv4routing::GetTypeId()
{
    static TypeId tid = TypeId("ns3::myipv4routing")
                            .SetParent<Ipv4RoutingProtocol>()
                            .SetGroupName("mymod")
                            .AddConstructor<myipv4routing>();
    return tid;
}

myipv4routing::myipv4routing()
    : m_ipv4(nullptr)
{
    NS_LOG_FUNCTION(this);
}

myipv4routing::~myipv4routing()
{
}

Ptr<Ipv4Route>
myipv4routing::RouteOutput(Ptr<Packet> p,
                           const Ipv4Header& header,
                           Ptr<NetDevice> oif,
                           Socket::SocketErrno& sockerr)
{
    return nullptr;
}

bool
myipv4routing::RouteInput(Ptr<const Packet> p,
                          const Ipv4Header& ipHeader,
                          Ptr<const NetDevice> idev,
                          const UnicastForwardCallback& ucb,
                          const MulticastForwardCallback& mcb,
                          const LocalDeliverCallback& lcb,
                          const ErrorCallback& ecb)
{
    NS_LOG_FUNCTION(this << p << ipHeader << ipHeader.GetSource() << ipHeader.GetDestination()
                         << idev << &ucb << &mcb << &lcb << &ecb);
    uint32_t iif = m_ipv4->GetInterfaceForDevice(idev);

    if (ipHeader.GetDestination().IsMulticast())
    {
        return false;
    }

    if (m_ipv4->IsDestinationAddress(ipHeader.GetDestination(), iif))
    {
        if (!lcb.IsNull())
        {
            lcb(p, ipHeader, iif);
            return true;
        }
        else
        {
            return false;
        }
    }

    if (!m_ipv4->IsForwarding(iif))
    {
        ecb(p, ipHeader, Socket::ERROR_NOROUTETOHOST);
        return true;
    }

    auto protocol = ipHeader.GetProtocol();
    auto packet = p->Copy();
    // uint16_t dstport;
    // uint16_t srcport;
    TcpHeader tcpheader;
    UdpHeader udpheader;

    if (protocol == 6)
    {
        packet->PeekHeader(tcpheader);
        // if (tcpheader.GetFlags() & TcpHeader::SYN)
        // {
        //     return false;
        // }
        // dstport = tcpheader.GetDestinationPort();
        // srcport = tcpheader.GetSourcePort();
    }
    else if (protocol == 17)
    {
        packet->PeekHeader(udpheader);
        // dstport = udpheader.GetDestinationPort();
        // srcport = udpheader.GetSourcePort();
    }
    else
    {
        return false;
    }

    Ptr<Ipv4Route> rtentry = Lookupsw(ipHeader.GetDestination());

    if (!rtentry)
    {
        return false;
    }

    ucb(rtentry, packet, ipHeader);
    return true;
}

Ptr<Ipv4Route>
myipv4routing::Lookupsw(Ipv4Address dest, Ptr<NetDevice> oif)
{
    Ptr<Ipv4Route> rtentry = nullptr;
    uint16_t longest_mask = 0;
    uint32_t shortest_metric = 0xffffffff;
    if (dest.IsLocalMulticast())
    {
        rtentry = Create<Ipv4Route>();
        rtentry->SetDestination(dest);
        rtentry->SetGateway(Ipv4Address::GetZero());
        rtentry->SetOutputDevice(oif);
        rtentry->SetSource(m_ipv4->GetAddress(m_ipv4->GetInterfaceForDevice(oif), 0).GetLocal());
        return rtentry;
    }

    for (NetworkRoutesI i = m_networkroutes.begin(); i != m_networkroutes.end(); ++i)
    {
        Ipv4RoutingTableEntry* entry_ptr = i->first;
        uint32_t metric = i->second;
        Ipv4Mask mask = entry_ptr->GetDestNetworkMask();
        uint16_t masklen = mask.GetPrefixLength();
        Ipv4Address entry = entry_ptr->GetDestNetwork();

        if (mask.IsMatch(dest, entry))
        {
            if (oif && oif != m_ipv4->GetNetDevice(entry_ptr->GetInterface()))
            {
                continue;
            }

            if (masklen < longest_mask)
            {
                continue;
            }

            if (masklen > longest_mask)
            {
                shortest_metric = 0xffffffff;
            }
            longest_mask = masklen;
            if (metric > shortest_metric)
            {
                continue;
            }
            shortest_metric = metric;

            rtentry = Create<Ipv4Route>();
            rtentry->SetDestination(entry_ptr->GetDest());
            rtentry->SetGateway(entry_ptr->GetGateway());
            rtentry->SetOutputDevice(m_ipv4->GetNetDevice(entry_ptr->GetInterface()));
            rtentry->SetSource(
                m_ipv4->SourceAddressSelection(entry_ptr->GetInterface(), entry_ptr->GetDest()));
            if (masklen == 32)
            {
                break;
            }
        }
    }

    return rtentry;
}

void
myipv4routing::NotifyInterfaceUp(uint32_t interface)
{
    for (uint32_t j = 0; j < m_ipv4->GetNAddresses(interface); ++j)
    {
        // Ipv4Address() Ipv4Mask() 会默认返回一个未初始化的值
        if (m_ipv4->GetAddress(interface, j).GetLocal() != Ipv4Address() &&
            m_ipv4->GetAddress(interface, j).GetMask() != Ipv4Mask() &&
            m_ipv4->GetAddress(interface, j).GetMask() != Ipv4Mask::GetOnes())
        {
            AddNetworkRouteTo(m_ipv4->GetAddress(interface, j)
                                  .GetLocal()
                                  .CombineMask(m_ipv4->GetAddress(interface, j).GetMask()),
                              m_ipv4->GetAddress(interface, j).GetMask(),
                              interface);
        }
    }
}

void
myipv4routing::NotifyInterfaceDown(uint32_t interface)
{
    for (auto it = m_networkroutes.begin(); it != m_networkroutes.end();)
    {
        if (it->first->GetInterface() == interface)
        {
            delete it->first;
            it = m_networkroutes.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void
myipv4routing::NotifyAddAddress(uint32_t interface, Ipv4InterfaceAddress address)
{
    if (!m_ipv4->IsUp(interface))
    {
        return;
    }

    auto networkmask = address.GetMask();
    auto networkaddr = address.GetLocal().CombineMask(networkmask);

    if (address.GetLocal() != Ipv4Address() && address.GetMask() != Ipv4Mask())
    {
        AddNetworkRouteTo(networkaddr, networkmask, interface);
    }
}

void
myipv4routing::NotifyRemoveAddress(uint32_t interface, Ipv4InterfaceAddress address)
{
    if (!m_ipv4->IsUp(interface))
    {
        return;
    }

    auto networkmask = address.GetMask();
    auto networkaddr = address.GetLocal().CombineMask(networkmask);

    for (auto it = m_networkroutes.begin(); it != m_networkroutes.end();)
    {
        if (it->first->GetInterface() == interface && it->first->IsNetwork() &&
            it->first->GetDestNetwork() == networkaddr &&
            it->first->GetDestNetworkMask() == networkmask)
        {
            delete it->first;
            it = m_networkroutes.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void
myipv4routing::SetIpv4(Ptr<Ipv4> ipv4)
{
    m_ipv4 = ipv4;
}

#include "ns3/log-macros-enabled.h"

void
myipv4routing::PrintRoutingTable(Ptr<OutputStreamWrapper> stream, Time::Unit unit) const
{
    NS_LOG_FUNCTION(this << stream);
    std::ostream* os = stream->GetStream();
    // Copy the current ostream state
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);

    *os << "Node: " << m_ipv4->GetObject<Node>()->GetId() << ", Time: " << Now().As(unit)
        << ", Local time: " << m_ipv4->GetObject<Node>()->GetLocalTime().As(unit)
        << ", myipv4routing table" << std::endl;

    if (GetNRoutes() > 0)
    {
        *os << "Destination     Gateway         Genmask         Flags Metric Ref    Use Iface"
            << std::endl;
        for (uint32_t j = 0; j < GetNRoutes(); j++)
        {
            std::ostringstream dest;
            std::ostringstream gw;
            std::ostringstream mask;
            std::ostringstream flags;
            Ipv4RoutingTableEntry route = GetRoute(j);
            dest << route.GetDest();
            *os << std::setw(16) << dest.str();
            gw << route.GetGateway();
            *os << std::setw(16) << gw.str();
            mask << route.GetDestNetworkMask();
            *os << std::setw(16) << mask.str();
            flags << "U";
            if (route.IsHost())
            {
                flags << "HS";
            }
            else if (route.IsGateway())
            {
                flags << "GS";
            }
            *os << std::setw(6) << flags.str();
            *os << std::setw(7) << GetMetric(j);
            // Ref ct not implemented
            *os << "-" << "      ";
            // Use not implemented
            *os << "-" << "   ";
            if (!Names::FindName(m_ipv4->GetNetDevice(route.GetInterface())).empty())
            {
                *os << Names::FindName(m_ipv4->GetNetDevice(route.GetInterface()));
            }
            else
            {
                *os << route.GetInterface();
            }
            *os << std::endl;
        }
    }
    *os << std::endl;
    // Restore the previous ostream state
    (*os).copyfmt(oldState);
}

uint32_t
myipv4routing::GetNRoutes() const
{
    return m_networkroutes.size();
}

Ipv4RoutingTableEntry*
myipv4routing::GetRoute(uint32_t i) const
{
    uint32_t cnt = 0;
    for (auto j = m_networkroutes.begin(); j != m_networkroutes.end(); ++j)
    {
        if (cnt == i)
        {
            return j->first;
        }
        ++cnt;
    }

    return nullptr;
}

uint32_t
myipv4routing::GetMetric(uint32_t i) const
{
    uint32_t cnt = 0;
    for (auto j = m_networkroutes.begin(); j != m_networkroutes.end(); ++j)
    {
        if (cnt == i)
        {
            return j->second;
        }
        ++cnt;
    }
    return 0;
}

bool
myipv4routing::LookupRoute(const Ipv4RoutingTableEntry& route, uint32_t metric)
{
    for (auto j = m_networkroutes.begin(); j != m_networkroutes.end(); j++)
    {
        Ipv4RoutingTableEntry* rtentry = j->first;

        if (rtentry->GetDest() == route.GetDest() &&
            rtentry->GetDestNetworkMask() == route.GetDestNetworkMask() &&
            rtentry->GetGateway() == route.GetGateway() &&
            rtentry->GetInterface() == route.GetInterface() && j->second == metric)
        {
            return true;
        }
    }
    return false;
}

void
myipv4routing::AddNetworkRouteTo(Ipv4Address network,
                                 Ipv4Mask networkMask,
                                 Ipv4Address nextHop,
                                 uint32_t interface,
                                 uint32_t metric)
{
    Ipv4RoutingTableEntry route =
        Ipv4RoutingTableEntry::CreateNetworkRouteTo(network, networkMask, nextHop, interface);

    if (!LookupRoute(route, metric))
    {
        auto routeptr = new Ipv4RoutingTableEntry(route);
        m_networkroutes.emplace_back(routeptr, metric);
    }
}

void
myipv4routing::AddNetworkRouteTo(Ipv4Address network,
                                 Ipv4Mask networkMask,
                                 uint32_t interface,
                                 uint32_t metric)
{
    Ipv4RoutingTableEntry route =
        Ipv4RoutingTableEntry::CreateNetworkRouteTo(network, networkMask, interface);

    if (!LookupRoute(route, metric))
    {
        auto routeptr = new Ipv4RoutingTableEntry(route);
        m_networkroutes.emplace_back(routeptr, metric);
    }
}

void
myipv4routing::DoDispose()
{
    for (auto j = m_networkroutes.begin(); j != m_networkroutes.end(); j = m_networkroutes.erase(j))
    {
        delete j->first;
    }
    m_ipv4 = nullptr;
    Ipv4RoutingProtocol::DoDispose();
}
} // namespace ns3