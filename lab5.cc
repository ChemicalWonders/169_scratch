/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
// Default Network Topology
//
// Wifi 10.1.1.0
// AP
//       * - B
//       |
//      115
//       |
// *-115-*-115-* 
// |     |     |
// D     AP    A
//       |
//     -115
//       * - C
//Consider a Wifi access point (AP) and a base station (STA), which are both static.
//The STA can communicate with the AP only when it is within a certain distance
//from the AP. Beyond that range, the two nodes can't receive each others signals.
// 
// Given below is a code to simulate the said scenario with ns3.
// STA sends a packet to the AP; AP echoes it back to the base station.
// The AP is located at position (x, y) = (0, 0). STA is at (xDistance, 0)
// (all distances in meter). Default value of xDistance is set to 10. [Lines #76, #131]
//  
//  Increase the value of xDistance in the code and find out the maximum distance upto which two way communication is possible. This can be verified from the output of the code, which will show the STA has received reply from the AP (indicated by IP address).
// Node #0 is the AP, #1 is a base station
// #1 sends UDP echo mesg to the AP; AP sends a UDP response back to the node
// Communication is possible only when the station is within a certain distance from the AP

// Mobility model is used for calculating propagation loss and propagation delay.
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("Wifi-2-nodes-fixed");
void
PrintLocations (NodeContainer nodes, std::string header)
{
    std::cout << header << std::endl;
    for(NodeContainer::Iterator iNode = nodes.Begin (); iNode != nodes.End (); ++iNode)
    {
        Ptr<Node> object = *iNode;
        Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
        NS_ASSERT (position != 0);
        Vector pos = position->GetPosition ();
        std::cout << "(" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
    }
    std::cout << std::endl;
}
void
PrintAddresses(Ipv4InterfaceContainer container, std::string header)
{
    std::cout << header << std::endl;
    uint32_t nNodes = container.GetN ();
    for (uint32_t i = 0; i < nNodes; ++i)
    {
        std::cout << container.GetAddress(i, 0) << std::endl;
    }
    std::cout << std::endl;
}

int
main (int argc, char *argv[])
{
    bool verbose = true;
    uint32_t nWifi = 5;
    /** Change this parameter and verify the output */
    double xDistance = 116.0;
    
    CommandLine cmd;
    cmd.AddValue ("xDistance", "Distance between two nodes along x-axis", xDistance);
    
    cmd.Parse (argc,argv);
    if (verbose)
    {
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }
    // 1. Create the nodes and hold them in a container
    NodeContainer wifiStaNodes,
    wifiApNode, ANode, BNode, CNode, DNode;
    
    wifiStaNodes.Create (nWifi);
    wifiApNode = wifiStaNodes.Get (0);
    ANode = wifiStaNodes.Get (1);
    BNode = wifiStaNodes.Get (2);
    CNode = wifiStaNodes.Get (3);
    DNode = wifiStaNodes.Get (4);


    // 2. Create channel for communication
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
    phy.SetChannel (channel.Create ());
    WifiHelper wifi = WifiHelper::Default ();
    wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
    
    NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();
   
     // 3a. Set up MAC for base stations
    Ssid ssid = Ssid ("ns-3-ssid");
    mac.SetType ("ns3::StaWifiMac",
                 "Ssid", SsidValue (ssid),
                 "ActiveProbing", BooleanValue (false));
    NetDeviceContainer ADevice;
    ADevice = wifi.Install (phy, mac, wifiStaNodes.Get (1));
    
    NetDeviceContainer BDevice, CDevice, DDevice;
    BDevice = wifi.Install (phy, mac, wifiStaNodes.Get(2));
    CDevice = wifi.Install (phy, mac, wifiStaNodes.Get(3));
    DDevice = wifi.Install (phy, mac, wifiStaNodes.Get(4));

    // 3b. Set up MAC for AP
    mac.SetType ("ns3::ApWifiMac",
                 "Ssid", SsidValue (ssid),
                 "BeaconGeneration", BooleanValue (true),
                 "BeaconInterval", TimeValue (Seconds (5)));
    NetDeviceContainer apDevice;
    apDevice = wifi.Install (phy, mac, wifiApNode);
    
    
    // 4. Set mobility of the nodes
    MobilityHelper mobility, mobilityA, mobilityB, mobilityC, mobilityD;
    // All space coordinates in meter
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                   "MinX", DoubleValue (0.0),
                                   "MinY", DoubleValue (0.0),
                                   "DeltaX", DoubleValue (0.0),
                                   "DeltaY", DoubleValue (10.0),
                                   "GridWidth", UintegerValue (3),
                                   "LayoutType", StringValue ("RowFirst"));
    
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
   
    mobilityA.SetPositionAllocator ("ns3::GridPositionAllocator",
                                   "MinX", DoubleValue (0.0),
                                   "MinY", DoubleValue (115.0),
                                   "DeltaX", DoubleValue (0.0),
                                   "DeltaY", DoubleValue (0.0),
                                   "GridWidth", UintegerValue (3),
                                   "LayoutType", StringValue ("RowFirst"));
    
    mobilityA.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
   
    mobilityB.SetPositionAllocator ("ns3::GridPositionAllocator",
                                   "MinX", DoubleValue (115.0),
                                   "MinY", DoubleValue (0.0),
                                   "DeltaX", DoubleValue (0.0),
                                   "DeltaY", DoubleValue (10.0),
                                   "GridWidth", UintegerValue (3),
                                   "LayoutType", StringValue ("RowFirst"));
    
    mobilityB.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    
    mobilityC.SetPositionAllocator ("ns3::GridPositionAllocator",
                                   "MinX", DoubleValue (0.0),
                                   "MinY", DoubleValue (-115.0),
                                   "DeltaX", DoubleValue (0.0),
                                   "DeltaY", DoubleValue (10.0),
                                   "GridWidth", UintegerValue (3),
                                   "LayoutType", StringValue ("RowFirst"));
    
    mobilityC.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    
    mobilityD.SetPositionAllocator ("ns3::GridPositionAllocator",
                                   "MinX", DoubleValue (-115.0),
                                   "MinY", DoubleValue (0.0),
                                   "DeltaX", DoubleValue (0.0),
                                   "DeltaY", DoubleValue (10.0),
                                   "GridWidth", UintegerValue (3),
                                   "LayoutType", StringValue ("RowFirst"));
    
    mobilityD.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  

    mobility.Install (wifiApNode);
    mobilityA.Install (ANode);
    mobilityB.Install (BNode);
    mobilityC.Install (CNode);
    mobilityD.Install (DNode);

    // 5.Add Internet layers stack
    InternetStackHelper stack;
    stack.Install (wifiStaNodes);
    

    // 6. Assign IP address to each device
    Ipv4AddressHelper address;
    Ipv4InterfaceContainer wifiApInterface, AInterface, BInterface, CInterface, DInterface;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    wifiApInterface = address.Assign (apDevice);
    AInterface = address.Assign (ADevice); 
    BInterface = address.Assign (BDevice);
    CInterface = address.Assign (CDevice);
    DInterface = address.Assign (DDevice); 

    // 7a. Create and setup applications (traffic sink)
    UdpEchoServerHelper echoServer (9); // Port # 9
    ApplicationContainer serverApps = echoServer.Install (wifiStaNodes.Get(2));
    serverApps.Start (Seconds (1.0));
    serverApps.Stop (Seconds (10.0));
   
    
    UdpEchoServerHelper echoServer2 (19); // Port # 19
    ApplicationContainer serverApps2 = echoServer2.Install (wifiStaNodes.Get(4));
    serverApps2.Start (Seconds (11.0));
    serverApps2.Stop (Seconds (20.0));
    

    // 7b. Create and setup applications (traffic source)
    UdpEchoClientHelper echoClient (BInterface.GetAddress (0), 9);
    echoClient.SetAttribute ("MaxPackets", UintegerValue (2));
    echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
    
    UdpEchoClientHelper echoClient2 (DInterface.GetAddress (0), 19);
    echoClient2.SetAttribute ("MaxPackets", UintegerValue (2));
    echoClient2.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    echoClient2.SetAttribute ("PacketSize", UintegerValue (1024));
    

    ApplicationContainer clientApps = echoClient.Install (wifiStaNodes.Get(1)); 
    clientApps.Start (Seconds (2.0));
    clientApps.Stop (Seconds (6.0));
    
    ApplicationContainer clientApps2 = echoClient2.Install (wifiStaNodes.Get(3)); 
    clientApps2.Start (Seconds (12.0));
    clientApps2.Stop (Seconds (15.0));
    

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    Simulator::Stop (Seconds (20.0));
    
    // 8. Enable tracing (optional)
    //phy.EnablePcapAll ("wifi-2-nodes-fixed", true);
    
    PrintAddresses(wifiApInterface, "IP addresses of base stations");
    PrintAddresses(AInterface, "IP address of A");
    PrintAddresses(BInterface, "IP Address of B");
    PrintAddresses(CInterface, "IP Address of C");
    PrintAddresses(DInterface, "IP Address of D");
    
    PrintLocations(wifiStaNodes, "Location of nodes"); 
    
    Simulator::Run ();
    Simulator::Destroy ();
    
    return 0;
}
