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
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"

//Network Topology
//
////    10.3.1.0   10.2.1.0    10.1.1.0
//// n3 --------n2 --------n0 --------- n1   n4   n5
////     point-------To-------point      |    |    |
////                    		  ====BUS=====
////                                     LAN 10.1.2.0

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SecondScriptExample");

int 
main (int argc, char *argv[])
{
  bool verbose = true;
  uint32_t nCsma = 2;

  CommandLine cmd;
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);

  cmd.Parse (argc,argv);

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

  nCsma = nCsma == 0 ? 1 : nCsma;

  NodeContainer p2pNodes;
  p2pNodes.Create (6);

  NodeContainer n0n1 = NodeContainer(p2pNodes.Get(0), p2pNodes.Get(1));
  NodeContainer n0n2 = NodeContainer(p2pNodes.Get(0), p2pNodes.Get(2));
  NodeContainer n2n3 = NodeContainer(p2pNodes.Get(2), p2pNodes.Get(3));
  
  //CSMA Nodes 
  NodeContainer csmaNodes;
  csmaNodes.Add (p2pNodes.Get (1));
  csmaNodes.Create (nCsma);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("20Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("10ms"));

  NetDeviceContainer deviceline01, deviceline02, deviceline23;
  deviceline01 = pointToPoint.Install (n0n1);
  deviceline02 = pointToPoint.Install (n0n2);
  deviceline23 = pointToPoint.Install (n2n3);  

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (13120)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);
  
  InternetStackHelper stack;
  stack.Install (p2pNodes.Get(0));
  stack.Install (p2pNodes.Get(2));
  stack.Install (p2pNodes.Get(3));
  stack.Install (csmaNodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (deviceline01);

  address.SetBase ("10.2.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces2;
  p2pInterfaces2 = address.Assign (deviceline02);
 
  address.SetBase ("10.3.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces3;
  p2pInterfaces3 = address.Assign (deviceline23);
  
  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);
 
  // 5. Create Applications:
  
  //Connection 1: Client1(node n0) sends a request to Server1(node n3) at 2rd
  // second and closes at 4th second. This server application opens from 1st
  // second to 5th second, using port 53 to listen to all potential requests and echo back. 
  
  //Connection 2: Client2(node n0) sends a request to Server2(node n5) 
  // at 9th second and closes at 11th second. This server application opens 
  // from 8th second to 15th second, using port 80 to listen this request and echo back.
  // (every request, echo and arp message is one packet with size of 2048. 
  // You don’t need to consider the concept of packets here). 

  //Link between 0 and 3
 

  UdpEchoServerHelper echoServer2 (9);

  ApplicationContainer serverApps2 = echoServer2.Install (p2pNodes.Get(3));
  serverApps2.Start (Seconds (1.0));
  serverApps2.Stop (Seconds (5.0));

  UdpEchoClientHelper echoClient2 (p2pInterfaces3.GetAddress (1), 9);
  echoClient2.SetAttribute ("MaxPackets", UintegerValue (50));
  echoClient2.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient2.SetAttribute ("PacketSize", UintegerValue (2048));

  
  ApplicationContainer clientApps2 = echoClient2.Install (p2pNodes.Get(0));
  clientApps2.Start (Seconds (2.0));
  clientApps2.Stop (Seconds (4.0));

  // Starting Server
  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (csmaNodes.Get (nCsma));
  serverApps.Start (Seconds (8.0));
  serverApps.Stop (Seconds (15.0));

  UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress (nCsma), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (3));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (2048));

  ApplicationContainer clientApps = echoClient.Install (p2pNodes.Get (0));
  clientApps.Start (Seconds (9.0));
  clientApps.Stop (Seconds (11.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  pointToPoint.EnablePcapAll ("second");
  csma.EnablePcap ("second", csmaDevices.Get (1), true);
  
  
  
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
