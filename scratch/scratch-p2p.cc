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
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

#include <cstdint>

// Default Network Topology
//
//       10.1.1.0
// n0 -------------- n1
//    point-to-point
//

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FirstScriptExample");

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

    Time::SetResolution(Time::NS);
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    NodeContainer nodes;
    nodes.Create(2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // UdpEchoServerHelper echoServer(9);

    // ApplicationContainer serverApps = echoServer.Install(nodes.Get(1));
    // serverApps.Start(Seconds(1.0));
    // serverApps.Stop(Seconds(10.0));

    // UdpEchoClientHelper echoClient(interfaces.GetAddress(1), 9);
    // echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    // echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    // echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    // ApplicationContainer clientApps = echoClient.Install(nodes.Get(0));
    // clientApps.Start(Seconds(2.0));
    // clientApps.Stop(Seconds(10.0));

    uint16_t port = 8889;

    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApp = sinkHelper.Install(nodes.Get(1));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(5.0));

    BulkSendHelper bulkHelper("ns3::TcpSocketFactory",
                              InetSocketAddress(interfaces.GetAddress(1), port));
    bulkHelper.SetAttribute("MaxBytes", UintegerValue(0));
    bulkHelper.SetAttribute("SendSize", UintegerValue(50));
    ApplicationContainer bulkApp = bulkHelper.Install(nodes.Get(0));
    bulkApp.Start(Seconds(1.0));
    bulkApp.Stop(Seconds(2.0));
    
    disprouting(nodes.Get(0));
    disprouting(nodes.Get(1));
    pointToPoint.EnablePcapAll("p2p");

    Simulator::Stop(Seconds(6.0));

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
