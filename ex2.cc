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
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FirstScriptExample");

int
main (int argc, char *argv[])
{
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  NodeContainer nodes;
  nodes.Create (4);
  NodeContainer n1n2 = NodeContainer(nodes.Get(1), nodes.Get(2));
  NodeContainer n1n3 = NodeContainer(nodes.Get(1), nodes.Get(3));
  NodeContainer n1n0 = NodeContainer(nodes.Get(1), nodes.Get(0));

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("3ms"));
  
  PointToPointHelper pointToPoint2;
  pointToPoint2.SetDeviceAttribute ("DataRate", StringValue("1.5Mbps"));
  pointToPoint2.SetChannelAttribute ("Delay", StringValue ("10ms"));

  NetDeviceContainer deviceline12, deviceline13, deadline;
  deviceline12 = pointToPoint.Install (n1n2);
  deviceline13 = pointToPoint.Install (n1n3);
  deadline = pointToPoint2.Install (n1n0);

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address, address2;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  address2.SetBase ("10.2.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (deviceline12);
  Ipv4InterfaceContainer interface2 = address2.Assign (deviceline13);
  
  //Link between 1 and 2
  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (n1n2);
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (15.0));

  UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (100));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (nodes.Get(2));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (12.0));

  //Link between 1 and 3
  UdpEchoServerHelper echoServer2 (9);

  ApplicationContainer serverApps2 = echoServer2.Install (n1n3);
  serverApps2.Start (Seconds (1.0));
  serverApps2.Stop (Seconds (15.0));

  UdpEchoClientHelper echoClient2 (interface2.GetAddress (1), 9);
  echoClient2.SetAttribute ("MaxPackets", UintegerValue (50));
  echoClient2.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient2.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps2 = echoClient2.Install (nodes.Get(3));
  clientApps2.Start (Seconds (4.0));
  clientApps2.Stop (Seconds (14.0));

 
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
