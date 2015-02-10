/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"

// Default Network Topology
//
//   Wifi 10.1.3.0
//                 AP
//  *    *    *    *
//  |    |    |    |    10.1.1.0
// n5   n6   n7   n0 -------------- n1   n2   n3   n4
//                   point-to-point  |    |    |    |
//                                   ================
//                                     LAN 10.1.2.0

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ThirdScriptExample");

void
CourseChange (std::string context, Ptr<const MobilityModel> model)
{
    Vector position = model->GetPosition ();
    NS_LOG_UNCOND (context <<
                   " x = " << position.x << ", y = " << position.y);
}

int 
main (int argc, char *argv[])
{
  // Enable Logging
  bool verbose = true;
  // nCsma represents the number of extra nodes sharing the LAN besides n1 here.
  // Same as nWifi
  uint32_t nCsma = 2;
  uint32_t nWifi = 3;

  // Adding Command line arguments here.
  // Use $ ./waf --run "scratch/mysecond --PrintHelp" to see help.
  // For example: $ ./waf --run "scratch/mysecond --nCsma=100 --nWifi=10"
  CommandLine cmd;
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);

  cmd.Parse (argc,argv);

  if (nWifi > 18)
    {
      std::cout << "Number of wifi nodes " << nWifi << 
                   " specified exceeds the mobility bounding box" << std::endl;
      exit (1);
    }

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

  //Notice that n1 at p2pNodes is added as the 0th node at csmaNodes set
  NodeContainer csmaNodes;
  csmaNodes.Create (3);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);
  
  //wifiStaNodes represent wifi Station
  //In IEEE 802.11 (Wi-Fi) terminology, a station (STA) is a device that has the capability to use the 802.11 protocol. For example, a station may be a laptop, a desktop PC, PDA, access point or Wi-Fi phone.
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (2);
  // use the “leftmost” node of the point-to-point link as the node for the wireless access point
  NodeContainer wifiApNode = csmaNodes.Get (0);

  // PHY Layer configuration
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  // we create a channel object and associate it to our PHY layer object manager to make sure that all the PHY layer objects created by the YansWifiPhyHelper share the same underlying channel, that is, they share the same wireless medium and can communication and interfere
  phy.SetChannel (channel.Create ());

  // MAC Layer configuration
  WifiHelper wifi = WifiHelper::Default ();
  // rate control algorithm
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  // we choose non-Qos MAC. Qos means Quality of Service
  NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();

  // service set identifier (SSID)
  Ssid ssid = Ssid ("ns-3-ssid");
    
  // the MAC instance next created will be a non-QoS non-AP station (STA) in an infrastructure BSS (i.e., a BSS with an AP
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  // Here phy and mac are set separately, especially they are separated from the Application
  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  // Change the Type of mac then install phy and mac to wifiApNode
  // If you use two different MAC protocol here, change the following mac to coressponding name.
  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  
  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);
  
  // Mobility Model
  // We want the STA nodes to be mobile, wandering around inside a bounding box, and we want to make the AP node stationary.
  MobilityHelper mobility;

  // reference: explore the Doxygen for class ns3::GridPositionAllocator
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  //RandomWalk2dMobilityModel: the nodes move in a random direction at a random speed around inside a bounding box
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility.Install (wifiStaNodes);
  
  //Another mobility model used for Access Point: fixed position
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);
 

  // 2nd Wifi Station
  //wifiStaNodes represent wifi Station
  NodeContainer wifiStaNodes2;
  wifiStaNodes2.Create (2);
  // use the “leftmost” node of the point-to-point link as the node for the wireless access point
  NodeContainer wifiApNode2 = csmaNodes.Get (1);

  // PHY Layer configuration
  YansWifiChannelHelper channel2 = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy2 = YansWifiPhyHelper::Default ();
  // we create a channel object and associate it to our PHY layer object manager to make sure that all the PHY layer objects created by the YansWifiPhyHelper share the same underlying channel, that is, they share the same wireless medium and can communication and interfere
  phy2.SetChannel (channel2.Create ());

  // MAC Layer configuration
  WifiHelper wifi2 = WifiHelper::Default ();
  // rate control algorithm
  wifi2.SetRemoteStationManager ("ns3::AarfWifiManager");

  // we choose non-Qos MAC. Qos means Quality of Service
  NqosWifiMacHelper mac2 = NqosWifiMacHelper::Default ();

  // service set identifier (SSID)
  Ssid ssid2 = Ssid ("ns-3-ssid-2");
    
  // the MAC instance next created will be a non-QoS non-AP station (STA) in an infrastructure BSS (i.e., a BSS with an AP
  mac2.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid2),
               "ActiveProbing", BooleanValue (false));

  // Here phy and mac are set separately, especially they are separated from the Application
  NetDeviceContainer staDevices2;
  staDevices2 = wifi2.Install (phy2, mac2, wifiStaNodes2);

  // Change the Type of mac then install phy and mac to wifiApNode
  // If you use two different MAC protocol here, change the following mac to coressponding name.
  mac2.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid2));

  
  NetDeviceContainer apDevices2;
  apDevices2 = wifi2.Install (phy2, mac2, wifiApNode2);
  
  // Mobility Model
  // We want the STA nodes to be mobile, wandering around inside a bounding box, and we want to make the AP node stationary.
  MobilityHelper mobility2;

  // reference: explore the Doxygen for class ns3::GridPositionAllocator
  mobility2.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  //RandomWalk2dMobilityModel: the nodes move in a random direction at a random speed around inside a bounding box
  mobility2.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility2.Install (wifiStaNodes2);

  //Another mobility model used for Access Point: fixed position
  mobility2.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility2.Install (wifiApNode2);
  
  // 3rd Wifi Station
  //wifiStaNodes represent wifi Station
  NodeContainer wifiStaNodes3;
  wifiStaNodes3.Create (2);
  // use the “leftmost” node of the point-to-point link as the node for the wireless access point
  NodeContainer wifiApNode3 = csmaNodes.Get (2);

  // PHY Layer configuration
  YansWifiChannelHelper channel3 = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy3 = YansWifiPhyHelper::Default ();
  // we create a channel object and associate it to our PHY layer object manager to make sure that all the PHY layer objects created by the YansWifiPhyHelper share the same underlying channel, that is, they share the same wireless medium and can communication and interfere
  phy3.SetChannel (channel3.Create ());

  // MAC Layer configuration
  WifiHelper wifi3 = WifiHelper::Default ();
  // rate control algorithm
  wifi3.SetRemoteStationManager ("ns3::AarfWifiManager");

  // we choose non-Qos MAC. Qos means Quality of Service
  NqosWifiMacHelper mac3 = NqosWifiMacHelper::Default ();

  // service set identifier (SSID)
  Ssid ssid3 = Ssid ("ns-3-ssid-3");
    
  // the MAC instance next created will be a non-QoS non-AP station (STA) in an infrastructure BSS (i.e., a BSS with an AP
  mac3.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid3),
               "ActiveProbing", BooleanValue (false));

  // Here phy and mac are set separately, especially they are separated from the Application
  NetDeviceContainer staDevices3;
  staDevices3 = wifi3.Install (phy3, mac3, wifiStaNodes3);

  // Change the Type of mac then install phy and mac to wifiApNode
  // If you use two different MAC protocol here, change the following mac to coressponding name.
  mac3.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid3));

  
  NetDeviceContainer apDevices3;
  apDevices3 = wifi3.Install (phy3, mac3, wifiApNode3);
  
  // Mobility Model
  // We want the STA nodes to be mobile, wandering around inside a bounding box, and we want to make the AP node stationary.
  MobilityHelper mobility3;

  // reference: explore the Doxygen for class ns3::GridPositionAllocator
  mobility3.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  //RandomWalk2dMobilityModel: the nodes move in a random direction at a random speed around inside a bounding box
  mobility3.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility3.Install (wifiStaNodes3);

  //Another mobility model used for Access Point: fixed position
  mobility3.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility3.Install (wifiApNode3);
  


  // Notice that there is no return. When we have A.Install (B), we will change object B with methods in A.
  // stack.Install is used to glu previous protocol layers together at the node.
  InternetStackHelper stack;
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);
  //stack.Install (csmaNodes.Get(1));
  //stack.Install (csmaNodes.Get(2));
  stack.Install (wifiApNode2);
  stack.Install (wifiStaNodes2);
  stack.Install (wifiApNode3);
  stack.Install (wifiStaNodes3);


  Ipv4AddressHelper address, address2;
  // BUS ADDRESSES
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  // Line for A
  address.SetBase ("10.1.2.0", "255.255.255.0");
  address.Assign (staDevices);
  address.Assign (apDevices);
  
  // Line for B 
  address.SetBase ("10.1.3.0", "255.255.255.0");
  address.Assign (staDevices2);
  address.Assign (apDevices2);
  
  // Line for C
  address2.SetBase ("10.1.4.0", "255.255.255.0");
  address2.Assign (staDevices3);
  address2.Assign (apDevices3);

  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (wifiApNode.Get (0));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress(0), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (5));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = 
    echoClient.Install (wifiStaNodes2.Get (0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  // Set the global routing because the packets have to be routed to the dentination.
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // wireless access point to generate beacons. It will generate beacons forever
  Simulator::Stop (Seconds (10.0));

  phy.EnablePcap ("third", apDevices.Get (0));
  phy2.EnablePcap ("third2", apDevices2.Get (0));
  
  csma.EnablePcap ("third", csmaDevices.Get (0), true);

  //std::ostringstream oss;
  //oss <<
  //"/NodeList/" << wifiStaNodes.Get (0)->GetId () <<
  //"/$ns3::MobilityModel/CourseChange";
    
 // Config::Connect (oss.str (), MakeCallback (&CourseChange));
  
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
