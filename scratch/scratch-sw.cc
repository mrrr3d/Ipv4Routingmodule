/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/myipv4routing.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

#include <cstdint>
#include <iostream>
#include <sstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FirstScriptExample");

void
dispmac(NetDeviceContainer devices)
{
    for (uint32_t i = 0; i < devices.GetN(); ++i)
    {
        auto device = devices.Get(i);
        auto node = device->GetNode();
        auto channel = device->GetChannel();

        std::cout << "Node " << node->GetId()
                  << " has device with MAC address: " << device->GetAddress() << std::endl;

        if (channel)
        {
            std::cout << "  Connected to channel with " << channel->GetNDevices()
                      << " devices:" << std::endl;
            for (uint32_t j = 0; j < channel->GetNDevices(); ++j)
            {
                Ptr<NetDevice> peerDevice = channel->GetDevice(j);
                Ptr<Node> peerNode = peerDevice->GetNode();
                std::cout << "    Node " << peerNode->GetId()
                          << " with MAC address: " << peerDevice->GetAddress() << std::endl;
            }
        }
    }
}

void
disprouting(Ptr<Node> node)
{
    auto nodeL3 = node->GetObject<Ipv4L3Protocol>();
    auto totalrouting = nodeL3->GetRoutingProtocol();
    if (!totalrouting)
    {
        std::cout << "not totalrouting!" << std::endl;
    }
    auto listrouting = DynamicCast<Ipv4ListRouting>(totalrouting);
    if (!listrouting)
    {
        std::cout << "not listrouting!" << std::endl;
        return;
    }
    auto num_proto = listrouting->GetNRoutingProtocols();

    std::cout << "vvvvvvvvvvvv node: " << node->GetId() << std::endl;
    int16_t pri = 0;

    for (uint32_t i = 0; i < num_proto; ++i)
    {
        auto protoi = listrouting->GetRoutingProtocol(i, pri);
        std::cout << "============= i = " << i << ", pri = " << pri << std::endl;
        std::stringstream ss;
        auto rtstream = Create<OutputStreamWrapper>(&ss);
        protoi->PrintRoutingTable(rtstream);
        std::cout << ss.str() << std::endl;
    }
}

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);
    LogComponentEnable("FirstScriptExample", ns3::LOG_LEVEL_ALL);
    // LogComponentEnable("myipv4routing", ns3::LOG_LEVEL_ALL);
    // LogComponentEnable("BulkSendApplication", LOG_LEVEL_ALL);
    // LogComponentEnable("PacketSink", LOG_LEVEL_ALL);

    NS_LOG_FUNCTION("");

    Time::SetResolution(Time::NS);
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    NodeContainer hosts;
    hosts.Create(4);

    NodeContainer sw;
    sw.Create(1);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer H0;
    NetDeviceContainer H1;
    NetDeviceContainer H2;
    NetDeviceContainer H3;
    H0 = pointToPoint.Install(sw.Get(0), hosts.Get(0));
    H1 = pointToPoint.Install(sw.Get(0), hosts.Get(1));
    H2 = pointToPoint.Install(sw.Get(0), hosts.Get(2));
    H3 = pointToPoint.Install(sw.Get(0), hosts.Get(3));

    InternetStackHelper stack;
    stack.Install(hosts);
    stack.Install(sw);

    Ipv4InterfaceContainer H0Iface;
    Ipv4InterfaceContainer H1Iface;
    Ipv4InterfaceContainer H2Iface;
    Ipv4InterfaceContainer H3Iface;

    Ipv4AddressHelper address;
    address.SetBase("10.1.0.0", "255.255.255.0");
    H0Iface = address.Assign(H0);
    address.SetBase("10.1.1.0", "255.255.255.0");
    H1Iface = address.Assign(H1);
    address.SetBase("10.1.2.0", "255.255.255.0");
    H2Iface = address.Assign(H2);
    address.SetBase("10.1.3.0", "255.255.255.0");
    H3Iface = address.Assign(H3);

    auto myrouting = Create<myipv4routing>();

    myrouting->AddNetworkRouteTo("10.1.0.0", "255.255.255.0", 1);
    myrouting->AddNetworkRouteTo("10.1.1.0", "255.255.255.0", 2);
    myrouting->AddNetworkRouteTo("10.1.2.0", "255.255.255.0", 3);
    myrouting->AddNetworkRouteTo("10.1.3.0", "255.255.255.0", 4);

    auto psw = sw.Get(0);
    auto pswipv4 = psw->GetObject<Ipv4L3Protocol>();
    auto totalrouting = pswipv4->GetRoutingProtocol();
    auto listrouting = DynamicCast<Ipv4ListRouting>(totalrouting);
    listrouting->AddRoutingProtocol(myrouting, 10);

    // 为4个host添加默认路由
    for (uint32_t i = 0; i < hosts.GetN(); ++i)
    {
        auto node = hosts.Get(i);
        auto tot = node->GetObject<Ipv4L3Protocol>()->GetRoutingProtocol();
        if (!tot) {
            std::cout << "tot is NULL" << std::endl;
            continue;
        }
        // 这里需要先转换成listrouting，再通过index找出 staticrouting，不能直接找staticrouting
        auto listrouting = DynamicCast<Ipv4ListRouting>(tot);
        if (!listrouting) {
            std::cout << "staticrouting is NULL" << std::endl;
            continue;
        }
        int16_t pri;
        auto staticrouting = DynamicCast<Ipv4StaticRouting>(listrouting->GetRoutingProtocol(0, pri));
        staticrouting->AddNetworkRouteTo(Ipv4Address::GetAny(), Ipv4Mask::GetZero(), 1);
    }

    uint16_t port = 8889;

    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApp = sinkHelper.Install(hosts.Get(1));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(5.0));

    BulkSendHelper bulkHelper("ns3::TcpSocketFactory",
                              InetSocketAddress(H1Iface.GetAddress(1), port));
    bulkHelper.SetAttribute("MaxBytes", UintegerValue(0));
    bulkHelper.SetAttribute("SendSize", UintegerValue(50));
    ApplicationContainer bulkApp = bulkHelper.Install(hosts.Get(0));
    bulkApp.Start(Seconds(1.0));
    bulkApp.Stop(Seconds(2.0));

    disprouting(sw.Get(0));
    disprouting(hosts.Get(0));
    disprouting(hosts.Get(1));
    disprouting(hosts.Get(2));
    disprouting(hosts.Get(3));

    pointToPoint.EnablePcapAll("sw");

    std::cout << "test cout !" << std::endl;
    for (uint32_t i = 0; i < hosts.GetN(); ++i)
    {
        Ptr<Node> node = hosts.Get(i);
        Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
        if (ipv4)
        {
            for (uint32_t j = 0; j < ipv4->GetNInterfaces(); ++j)
            { // 从 1 开始，跳过 loopback 接口
                std::cout << "Node " << node->GetId() << ", Interface " << j
                          << " has IP address: " << ipv4->GetAddress(j, 0).GetLocal() <<
                          std::endl;
            }
        }
    }

    Ptr<Node> node = sw.Get(0);
    auto ipv4 = node->GetObject<Ipv4>();
    if (ipv4)
    {
        for (uint32_t j = 0; j < ipv4->GetNInterfaces(); ++j)
        { // 从 1 开始，跳过 loopback 接口
            std::cout << "Switch " << node->GetId() << ", Interface " << j
                      << " has IP address: " << ipv4->GetAddress(j, 0).GetLocal() << std::endl;
        }
    }

    dispmac(H0);
    dispmac(H1);
    dispmac(H2);
    dispmac(H3);

    Simulator::Stop(Seconds(6.0));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
