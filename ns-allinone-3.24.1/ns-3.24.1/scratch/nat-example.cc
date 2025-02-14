/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 Sindhuja Venkatesh
 *
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
 *
 * Authors: Sindhuja Venkatesh <intutivestriker88@gmail.com>
 */
#include <iostream>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-module.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("NetfilterExample");

int
main (int argc, char *argv[])
{
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  //uint16_t port = 9;

// Desired topology:
//       n0    n1   n2   n3
//       |     |    |    |
//       ================= <----> n4 <-----> n5


  NS_LOG_INFO ("Create nodes.");
  NodeContainer nodes1;
  nodes1.Create (5);

  NodeContainer nodes2;
  nodes2.Create (5);

  NodeContainer third;
  third.Add ( nodes1.Get (4) );
  third.Add ( nodes2.Get (4) );
  third.Create (2);

  NS_LOG_INFO ("Build Topology");
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices1 = csma.Install (nodes1);
  NetDeviceContainer devices2 = csma.Install (nodes2);
  NetDeviceContainer devices3 = pointToPoint.Install (third);


  InternetStackHelper internet;
  internet.Install (nodes1);
  internet.Install (nodes2.Get(0));
  internet.Install (nodes2.Get(1));
  internet.Install (nodes2.Get(2));
  internet.Install (nodes2.Get(3));
  internet.Install (third.Get (1));


//        private address    NAT      public address
// n0 <--------------------> n1 <-----------------------> n2
// 192.168.1.1   192.168.1.2    203.82.48.1  203.82.48.2
//



// Desired topology:
//       n0    n1   n2   n3
//192.168.1.1 1.2   1.3  1.4
//       |     |    |    |
//       ================= <-------------> n4 <---------------> n5
//                             192.168.1.5  ^  203.82.48.1   203.82.48.2
//                                          . 
//       n0     n1   n2   n4                .
//192.168.2.1  2.2  2.3  2.4                . 
//       |      |    |    |                 .
//       ================== <---------------'
//
//

  Ipv4AddressHelper address1;
  address1.SetBase ("192.168.1.0", "255.255.255.0");

  Ipv4AddressHelper address2;
  address2.SetBase ("154.168.1.0", "255.255.255.0");

  Ipv4AddressHelper address3;
  address3.SetBase ("203.82.48.0", "255.255.255.0");

  Ipv4InterfaceContainer firstInterfaces = address1.Assign (devices1);
  Ipv4InterfaceContainer secondInterfaces = address2.Assign (devices2);
  Ipv4InterfaceContainer thirdInterfaces = address3.Assign (devices3);

  Ipv4NatHelper natHelper;
  // The zeroth element of the second node container is the NAT node
  Ptr<Ipv4Nat> nat = natHelper.Install (third.Get (0));
  // Configure which of its Ipv4Interfaces are inside and outside interfaces
  // The zeroth Ipv4Interface is reserved for the loopback interface
  // Hence, the interface facing n0 is numbered "1" and the interface
  // facing n2 is numbered "2" (since it was assigned in the second step above)
  nat->SetInside (1);
  nat->SetInside (2);
  nat->SetOutside (3);


  //Adding the address to be translated to and port pools.
  nat->AddAddressPool ("192.168.1.15","0.0.0.15","0.0.0.15","255.255.0.0");
  nat->AddPortPool (49153, 49163);

// Dynamic Rules
  Ipv4DynamicNatRule rule1 (Ipv4Address ("192.168.1.0"), Ipv4Mask ("255.255.255.0"));
  nat->AddDynamicRule (rule1);

// Static Rules
  Ipv4StaticNatRule rule2 (Ipv4Address ("192.168.2.1"), 48000, Ipv4Address ("203.82.48.100"), 8081, IPPROTO_UDP);
  nat->AddStaticRule (rule2);

  Ipv4StaticNatRule rule3 (Ipv4Address ("192.168.2.2"), 48000, Ipv4Address ("203.82.48.100"), 8082, IPPROTO_UDP);
  nat->AddStaticRule (rule3);

  Ipv4StaticNatRule rule4 (Ipv4Address ("192.168.2.3"), 48000, Ipv4Address ("203.82.48.100"), 8083, IPPROTO_UDP);
  nat->AddStaticRule (rule4);

  Ipv4StaticNatRule rule5 (Ipv4Address ("192.168.2.4"), 48000, Ipv4Address ("203.82.48.100"), 8084, IPPROTO_UDP);
  nat->AddStaticRule (rule5);



  // Configure applications to generate traffic
  UdpEchoServerHelper echoServer (9);



  // This application corresponds to the first rule
  ApplicationContainer serverApps = echoServer.Install (third.Get (1));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (thirdInterfaces.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (512));


  ApplicationContainer clientApps = echoClient.Install (nodes1.Get (0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  ApplicationContainer clientApp1 = echoClient.Install (nodes1.Get (1));
  clientApp1.Start (Seconds (2.0));
  clientApp1.Stop (Seconds (10.0));

  ApplicationContainer clientApp2 = echoClient.Install (nodes1.Get (2));
  clientApp2.Start (Seconds (2.0));
  clientApp2.Stop (Seconds (10.0));

  ApplicationContainer clientApp3 = echoClient.Install (nodes1.Get (3));
  clientApp3.Start (Seconds (2.0));
  clientApp3.Stop (Seconds (10.0));

  ApplicationContainer clientApp4 = echoClient.Install (nodes1.Get (4));
  clientApp4.Start (Seconds (2.0));
  clientApp4.Stop (Seconds (10.0));



  ApplicationContainer clientApps5 = echoClient.Install (nodes2.Get (0));
  clientApps5.Start (Seconds (2.0));
  clientApps5.Stop (Seconds (10.0));

  ApplicationContainer clientApps6 = echoClient.Install (nodes2.Get (1));
  clientApps6.Start (Seconds (2.0));
  clientApps6.Stop (Seconds (10.0));

  ApplicationContainer clientApps7 = echoClient.Install (nodes2.Get (2));
  clientApps7.Start (Seconds (2.0));
  clientApps7.Stop (Seconds (10.0));

  ApplicationContainer clientApps8 = echoClient.Install (nodes2.Get (4));
  clientApps8.Start (Seconds (2.0));
  clientApps8.Stop (Seconds (10.0));

 
  // Prepare to run the simulation
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  pointToPoint.EnablePcapAll ("ipv4-nat", false);

  Simulator::Run ();
  // Now print them out
  Ptr<OutputStreamWrapper> natStream = Create<OutputStreamWrapper> ("dynamic-nat.rules", std::ios::out);
  nat->PrintTable (natStream);


  Simulator::Destroy ();
  return 0;
}
