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
  uint32_t nCsma = 3;
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

  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  //Notice that n1 at p2pNodes is added as the 0th node at csmaNodes set
  NodeContainer csmaNodes;
  csmaNodes.Add (p2pNodes.Get (1));
  csmaNodes.Create (nCsma);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  //wifiStaNodes represent wifi Station
  //In IEEE 802.11 (Wi-Fi) terminology, a station (STA) is a device that has the capability to use the 802.11 protocol. For example, a station may be a laptop, a desktop PC, PDA, access point or Wi-Fi phone.
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  // use the “leftmost” node of the point-to-point link as the node for the wireless access point
  NodeContainer wifiApNode = p2pNodes.Get (0);

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

  // Notice that there is no return. When we have A.Install (B), we will change object B with methods in A.
  // stack.Install is used to glu previous protocol layers together at the node.
  InternetStackHelper stack;
  stack.Install (csmaNodes);
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  // No interface here
  address.SetBase ("10.1.3.0", "255.255.255.0");
  address.Assign (staDevices);
  address.Assign (apDevices);

  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (csmaNodes.Get (nCsma));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress (nCsma), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = 
    echoClient.Install (wifiStaNodes.Get (nWifi - 1));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  // Set the global routing because the packets have to be routed to the dentination.
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // wireless access point to generate beacons. It will generate beacons forever
  Simulator::Stop (Seconds (10.0));

  pointToPoint.EnablePcapAll ("third");
  phy.EnablePcap ("third", apDevices.Get (0));
  csma.EnablePcap ("third", csmaDevices.Get (0), true);

  std::ostringstream oss;
  oss <<
  "/NodeList/" << wifiStaNodes.Get (nWifi - 1)->GetId () <<
  "/$ns3::MobilityModel/CourseChange";
    
  Config::Connect (oss.str (), MakeCallback (&CourseChange));
    
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
