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
 *
 * Author: Diego Lopez Pajares  <diego.lopezp@uah.es> 
 * Derived from bridge-net-device */


#include "tfe-switch.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/header.h"
#include "ns3/ethernet-header.h"
#include "ns3/arp-header.h"
#include "ns3/tcp-header.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/enum.h"
#include "ns3/random-variable-stream.h"
#include <bitset>

#define DEBUG 0
#define MOBILITY
#define HELLO_PROT 65392
#define PATH_PROT 65455
#define ELEPH_PORT_FIRST 3500
#define ELEPH_PORT_LAST  4500
#define RABBI_PORT_FIRST 2500
#define MOUSE_PORT_FIRST 1500
namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("TFESwitch");
NS_OBJECT_ENSURE_REGISTERED (TFESwitch);
TypeId
TFESwitch::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TFESwitch")
    .SetParent<NetDevice> ()
    .SetGroupName("Bridge")
    .AddConstructor<TFESwitch> ()
    .AddAttribute ("HelloTime",
                   "Time for resend new Hello Packets",
                   TimeValue (Seconds (5)),
                   MakeTimeAccessor (&TFESwitch::m_hello_period),
                   MakeTimeChecker ())
    .AddAttribute ("ArpExpire",
                   "Expiration Time ARP",
                   TimeValue (Seconds (10)),
                   MakeTimeAccessor (&TFESwitch::m_ArpExpire),
                   MakeTimeChecker ())
    .AddAttribute ("ArpBlock",
                   "Block Time ARP",
                   TimeValue (Seconds (3)),
                   MakeTimeAccessor (&TFESwitch::m_ArpBlock),
                   MakeTimeChecker ())
    .AddAttribute ("TcpExpire",
                   "Block Time TCP",
                   TimeValue (Seconds (10)),
                   MakeTimeAccessor (&TFESwitch::m_TcpExpire),
                   MakeTimeChecker ())
    .AddAttribute ("SwitchMode", "Work mode",
                   UintegerValue (TFEPATH),
                   MakeUintegerAccessor (&TFESwitch::m_switch_mode),
                   MakeUintegerChecker<uint16_t> ())
   .AddAttribute ("IsToR",
                   "Mark if the switch is a ToR(not used)",
                   BooleanValue(false),
                   MakeBooleanAccessor (&TFESwitch::m_tor),
                   MakeBooleanChecker())
    .AddAttribute ("Hierarchy",
                   "Mark if the switch hierarcy",
                   EnumValue(0),
                   MakeEnumAccessor (&TFESwitch::m_hierarchy),
                   MakeEnumChecker(Core,"Core",Aggr,"Aggr",Tor,"Tor",Err,"Err"))
  ;
  return tid;
}

TFESwitch::TFESwitch ()
  
{
  NS_LOG_FUNCTION(this);
  m_channel = CreateObject<BridgeChannel> ();
  dst_broadcast= CreateObject<UniformRandomVariable> ();
  Simulator::Schedule(MicroSeconds(1),&TFESwitch::SendPeriodicHello,this);
  Simulator::Schedule(m_TcpExpire,&TFESwitch::CleanTcpTable,this);
  Simulator::Schedule(Seconds(10),&TFESwitch::CleanArpTable,this);

}

TFESwitch::~TFESwitch()
{
  NS_LOG_FUNCTION(this);
}

void 
TFESwitch::AddBridgePort (NetDeviceSwitchTFE  bridgePort)
{
  NS_LOG_FUNCTION(this);
  NS_ASSERT (bridgePort.first != this);
  if (!Mac48Address::IsMatchingType (bridgePort.first->GetAddress ()))
    {
      NS_FATAL_ERROR ("Device does not support eui 48 addresses: cannot be added to bridge.");
    }
  if (!bridgePort.first->SupportsSendFrom ())
    {
      NS_FATAL_ERROR ("Device does not support SendFrom: cannot be added to bridge.");
    }
  if (m_address == Mac48Address ())
    {
      m_address = Mac48Address::ConvertFrom (bridgePort.first->GetAddress ());
    }

  m_node->RegisterProtocolHandler (MakeCallback (&TFESwitch::ReceiveFromDevice, this),
                                   0, bridgePort.first, true);
  m_ports.insert (bridgePort);
  m_ordered_ports.push_back(bridgePort.first);
  m_channel->AddChannel (bridgePort.first->GetChannel ());
}


void
TFESwitch::ReceiveFromDevice (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet, uint16_t protocol,
                                    Address const &src, Address const &dst, PacketType packetType)
{
  NS_LOG_FUNCTION(this);
  Mac48Address src48 = Mac48Address::ConvertFrom (src);
  Mac48Address dst48 = Mac48Address::ConvertFrom (dst);
  PossiblePackets packetiD;
  packetiD=TypeOfPacket(packet, protocol);
  if (!m_promiscRxCallback.IsNull ())
    {
      m_promiscRxCallback (this, packet, protocol, src, dst, packetType);
    }
  switch (packetType)
    {
    case PACKET_HOST:
      if (dst48 == m_address)
        {
          m_rxCallback (this, packet, protocol, src);
        }
      break;
    case PACKET_BROADCAST:
    case PACKET_MULTICAST:
      if(protocol==HELLO_PROT)//Hello packet EtherType
      {
      	m_rxCallback (this, packet, protocol, src);
      	RcvHelloPacket(incomingPort, src48);
      }
      else
      {
      	m_rxCallback (this, packet, protocol, src);
      	switch(m_switch_mode)
      	{
      		case ARPPATH:
      			ModeArppath(incomingPort, packet, protocol, src48, dst48, packetiD);
      			break;
      		case TCPPATH:
				ModeTcppath(incomingPort, packet, protocol, src48, dst48, packetiD);
				break;
			case TFEPATH:
				if( (m_tcp_src>=ELEPH_PORT_FIRST && m_tcp_src<=ELEPH_PORT_LAST) || 
					(m_tcp_dst>=ELEPH_PORT_FIRST && m_tcp_dst<=ELEPH_PORT_LAST))
				{
					ModeTcppath(incomingPort, packet, protocol, src48, dst48, packetiD);
				}
				else
				{
					ModeArppath(incomingPort, packet, protocol, src48, dst48, packetiD);
				}
				break;
			default:
				NS_LOG_ERROR("Fatal Error");
				break;
      	}
      }
      break;

    case PACKET_OTHERHOST:
      if (dst48 == m_address)
        {
          	m_rxCallback (this, packet, protocol, src);
        }
      else
        {
        	m_rxCallback (this, packet, protocol, src);
        	switch(m_switch_mode)
      		{
      			case ARPPATH:
      				ModeArppath(incomingPort, packet, protocol, src48, dst48, packetiD);
      				break;
      			case TCPPATH:
					ModeTcppath(incomingPort, packet, protocol, src48, dst48, packetiD);
					break;
				case TFEPATH:
					if( (m_tcp_src>=ELEPH_PORT_FIRST && m_tcp_src<=ELEPH_PORT_LAST) || 
						(m_tcp_dst>=ELEPH_PORT_FIRST && m_tcp_dst<=ELEPH_PORT_LAST))
					{
						ModeTcppath(incomingPort, packet, protocol, src48, dst48, packetiD);
					}
					else
					{
						ModeArppath(incomingPort, packet, protocol, src48, dst48, packetiD);
					}
					break;
				default:
					NS_LOG_ERROR("Fatal Error, Switch mode not found!");
					break;
      		}
        }
      break;
    }
}


void TFESwitch::SendPeriodicHello(void)
{
	NS_LOG_FUNCTION(this);	
	std::vector<NeighbourTable>::iterator table_iter=m_neighbour.begin();
	for (table_iter; table_iter!=m_neighbour.end();)
	{
		if(table_iter->period<Simulator::Now())
		{
			m_neighbour.erase(table_iter);
			table_iter=m_neighbour.begin();
		}
		else
		{
			table_iter++;
		}
	}	
	NetDeviceContainerSwitchTFE::iterator ports_iter;
	for (ports_iter=m_ports.begin(); ports_iter!=m_ports.end(); ports_iter++)
	{
		Ptr<Packet> packet = Create<Packet> ();
		EthernetHeader eth_head;
		eth_head.SetDestination(Mac48Address::GetBroadcast());
		eth_head.SetSource(Mac48Address::ConvertFrom(ports_iter->first->GetAddress()));
		packet->AddHeader(eth_head);		
		ports_iter->first->SendFrom(packet->Copy (),Mac48Address::ConvertFrom(ports_iter->first->GetAddress()),Mac48Address::GetBroadcast(), HELLO_PROT);//Protocol 0xFF70
	}
	Simulator::Schedule(m_hello_period,&TFESwitch::SendPeriodicHello,this);
}


void TFESwitch::RcvHelloPacket(Ptr<NetDevice> port_in, Mac48Address src)
{	
	std::vector<NeighbourTable>::iterator table_iter=m_neighbour.begin();
	NeighbourTable newEntry;
	bool search=false;
	
	if(m_neighbour.size()!=0)
	{
		for(table_iter; table_iter!=m_neighbour.end(); table_iter++)
		{
			if(table_iter->device==port_in)
			{
				table_iter->period=Simulator::Now()+m_hello_period;
				search=true;
				break;
			}
		}
		if(!search)
		{
			newEntry.device=port_in;
			newEntry.direction=src;
			newEntry.period=Simulator::Now()+m_hello_period;
			m_neighbour.push_back(newEntry);
		}
	}
	else
	{
		newEntry.device=port_in;
		newEntry.direction=src;
		newEntry.period=Simulator::Now()+m_hello_period;
		m_neighbour.push_back(newEntry);
	}
	
}

bool TFESwitch::SearchEntryHelloTable(Ptr<NetDevice> port)
{
	NS_LOG_FUNCTION(this);
	std::vector<NeighbourTable>::iterator table_iter=m_neighbour.begin();
	for(table_iter; table_iter!=m_neighbour.end(); table_iter++)
	{
		if(table_iter->device==port)
		{
			return true;
		}
	}
	return false;	
}

PossiblePackets TFESwitch::TypeOfPacket(Ptr<const Packet> packet, uint16_t protocol)
{
	NS_LOG_FUNCTION(this);
  	Ptr<Packet> q = packet->Copy();
  	PacketMetadata::ItemIterator metadataIterator = q->BeginItem();
  	PacketMetadata::Item item;
  	PossiblePackets packet_type;
    uint8_t flag=0;

	std::ostringstream impresion;
	q->Print(impresion);
  	while (metadataIterator.HasNext())
  	{
    	item = metadataIterator.Next();
    	if (item.type==PacketMetadata::Item::HEADER)
    	{	
    		if (item.tid.GetName()=="PathHeader")
      		{
      			Callback<ObjectBase *> constructor = item.tid.GetConstructor ();
        		ObjectBase *instance = constructor ();
        		PathHeader *chunk = dynamic_cast<PathHeader *> (instance);
        		chunk->Deserialize (item.current);
					m_tcp_src=chunk->GetTcpSrc();
					m_tcp_dst=chunk->GetTcpDst();
					delete chunk;
          			packet_type=PATH_REQ;
          			break;			
      				return packet_type;
      		}
      		else if (item.tid.GetName()=="ns3::EthernetHeader")
      		{

      			Callback<ObjectBase *> constructor = item.tid.GetConstructor ();
        		ObjectBase *instance = constructor ();
        		EthernetHeader *chunk = dynamic_cast<EthernetHeader *> (instance);
        		chunk->Deserialize (item.current);
        		if (chunk->GetDestination().IsBroadcast())
        		{
          			delete chunk;
          			packet_type=ETH_BROAD;
        		}
        		else
        		{
        			delete chunk;
          			packet_type=OTHERS;
        		}
      		}
      		else if (item.tid.GetName()=="ns3::ArpHeader")
      		{
        		Callback<ObjectBase *> constructor = item.tid.GetConstructor ();
        		ObjectBase *instance = constructor ();
        		ArpHeader *chunk = dynamic_cast<ArpHeader *> (instance);
        		chunk->Deserialize (item.current);
        		if (chunk->IsRequest())
        		{
          			delete chunk;
          			packet_type=ARP_REQ;
        		}
        		else
        		{
          			delete chunk;
          			packet_type=ARP_REP;
        		}
      		}
      		else if (item.tid.GetName()=="ns3::TcpHeader")
      		{
        		Callback<ObjectBase *> constructor = item.tid.GetConstructor ();
        		ObjectBase *instance = constructor ();
        		TcpHeader *chunk = dynamic_cast<TcpHeader *> (instance);
        		chunk->Deserialize (item.current);       		
        		flag=chunk->GetFlags();

				switch (flag)
				{
					case 2: //SYN
						packet_type=TCP_SYN;
						break;
					case 16:
						//ACK
						packet_type=TCP_ACK;
						break;
					case 17: //FIN+ACK
						packet_type=TCP_FIN_ACK;
						break;
					case 18: //SYN+ACK
						packet_type=TCP_SYN_ACK;
						break;
					default:
						packet_type=OTHERS;
						break;
				}
				TFESwitch::m_tcp_src=chunk->GetSourcePort();
				TFESwitch::m_tcp_dst=chunk->GetDestinationPort();
          		delete chunk;
      		}
      		else if (item.tid.GetName()=="ns3::Ipv4Header")
      		{
      			uint8_t *buffer = new uint8_t[q->GetSize ()];
				q->CopyData(buffer, q->GetSize ());
      			m_tcp_src=buffer[20]<<8|buffer[21];
      			m_tcp_dst=buffer[22]<<8|buffer[23]; 
      			flag=buffer[33];
      			switch (flag)
				{
					case 2:
						packet_type=TCP_SYN;
						break;
					case 16:
						packet_type=TCP_ACK;
						break;
					case 17:
					case 25:
						packet_type=TCP_FIN_ACK;
						break;
					case 18:
						packet_type=TCP_SYN_ACK;
						break;
					default:
						packet_type=OTHERS;
						break;
				}
				delete buffer;	
      		}
      		else
      		{
        		packet_type=OTHERS;
      		}
    	}
  	}
  	return packet_type;	
}

void
TFESwitch::ModeArppath (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet,
                                 uint16_t protocol, Mac48Address src, Mac48Address dst, PossiblePackets packetiD)
{
	NS_LOG_FUNCTION(this);
	Ptr<NetDevice> outputPort=NULL;
	Ptr<NetDevice> inEntry=NULL;
	if(!dst.IsBroadcast())
	{
		outputPort=GetPortArpTable(dst);

        if(packetiD==ARP_REP)
		{
			LearnUpdateArpTable(src,incomingPort,packetiD);
			LearnUpdateArpTable(dst,outputPort,ARP_REQ);
		}
		LearnUpdateArpTable(dst,outputPort,packetiD);
		outputPort->SendFrom (packet->Copy (), src, dst, protocol);
	}
	else
	{
		inEntry=GetPortArpTable(src);	
		LearnUpdateArpTable(src,incomingPort,packetiD);
		ResendBroadcast(incomingPort,packet->Copy (), src,dst, protocol);
	}	
}

void
TFESwitch::ModeTcppath (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet,
                                 uint16_t protocol, Mac48Address src, Mac48Address dst,PossiblePackets packetiD)
{
	NS_LOG_FUNCTION(this);				
	Ptr<NetDevice> outputPort=NULL;
	Ptr<NetDevice> arpEntry=NULL;
	Ptr<NetDevice> tcpEntry=NULL;
	Ptr<Packet> newPacket;
	Mac48Address path_dst;
    int i=0;
	switch (packetiD)
	{
		case TCP_SYN:
			outputPort=GetPortArpTable(dst);			
			if(outputPort!=NULL && !SearchEntryHelloTable(outputPort))
			{
				LearnUpdateTcpTable(src,dst,incomingPort,packetiD);
				outputPort->SendFrom(packet->Copy (), src,dst, protocol);
			}
			else
			{	
				newPacket=CreatePathPacket(packet,src,dst);
				LearnUpdateTcpTable(src,dst,incomingPort,packetiD);
				ResendBroadcast(incomingPort,newPacket->Copy (), src,Mac48Address::GetBroadcast(),PATH_PROT);
			}
			#ifdef MOBILITY
			 UpdateNeighbourArpTable(incomingPort);
			#endif
			break;
		case TCP_SYN_ACK:
			LearnUpdateTcpTable(src,dst,incomingPort,packetiD);
			LearnUpdateArpTable(dst,incomingPort,packetiD);
			outputPort=GetPortTcpTable(src,dst,packetiD);			
			#ifdef MOBILITY
			 	UpdateNeighbourArpTable(incomingPort);
			#endif
			outputPort->SendFrom (packet->Copy (), src,dst, protocol);
			break;
		case TCP_FIN_ACK:
			outputPort=GetPortTcpTable(src,dst,packetiD);
			#ifdef MOBILITY
				UpdateNeighbourArpTable(incomingPort);
			#endif
			outputPort->SendFrom (packet->Copy (), src, dst, protocol);
			LearnUpdateTcpTable(src,dst,incomingPort,packetiD);
			break;
		case PATH_REQ:
			path_dst=GetDstPathPaquet(packet);
			tcpEntry=GetPortTcpTable(src,path_dst,packetiD);
			if(tcpEntry==NULL || tcpEntry==incomingPort)
			{
				LearnUpdateTcpTable(src,path_dst,incomingPort,packetiD);
				arpEntry=GetPortArpTable(path_dst);
				if(arpEntry==NULL || SearchEntryHelloTable(arpEntry))
				{
					ResendBroadcast(incomingPort,IncrementCount(packet->Copy())->Copy(), src, Mac48Address::GetBroadcast(), PATH_PROT);
				}
				else
				{
					newPacket=DeletePathPacket(packet);
					arpEntry->SendFrom (newPacket->Copy (),src,path_dst, 2048);
				}
				
			}
			break;
		case ARP_REQ:
			ModeArppath (incomingPort,packet,protocol,src,dst,packetiD);
			break;
		case ARP_REP:
			ModeArppath (incomingPort,packet,protocol,src,dst,packetiD);
			break;
		default:
			outputPort=GetPortTcpTable(src,dst,packetiD);
			#ifdef MOBILITY
			 	UpdateNeighbourArpTable(incomingPort);
			#endif
			outputPort->SendFrom (packet->Copy (), src, dst, protocol);
			LearnUpdateTcpTable(src,dst,incomingPort,packetiD);
			break;
	}
	m_tcp_src=0;
  	m_tcp_dst=0;	
}

void TFESwitch::LearnUpdateArpTable(Mac48Address source, Ptr<NetDevice> port, PossiblePackets iD)
{
	NS_LOG_FUNCTION(this);
	bool find=false;
	std::vector<ArpTable>::iterator arp_table_iter=m_arp_table.begin();
	ArpTable newEntry;
	for(arp_table_iter; arp_table_iter!=m_arp_table.end(); arp_table_iter++)
	{
		if(arp_table_iter->MacAddr==source)
		{
			if(iD==ARP_REQ || ETH_BROAD)
				arp_table_iter->valid_period=Simulator::Now()+m_ArpBlock;
			else
				arp_table_iter->valid_period=Simulator::Now()+m_ArpExpire;
			find=true;
			break;
		}
	}
	if(!find)
	{
		newEntry.MacAddr=source;
		newEntry.output_device=port;
		newEntry.neighbour=!SearchEntryHelloTable(port);
		if(iD==ARP_REQ || ETH_BROAD)
			newEntry.valid_period=Simulator::Now()+m_ArpBlock;
		else
			newEntry.valid_period=Simulator::Now()+m_ArpExpire;
		m_arp_table.push_back(newEntry);
	}  
}

void TFESwitch::UpdateNeighbourArpTable(Ptr<NetDevice> port)
{
	NS_LOG_FUNCTION(this);
	std::vector<ArpTable>::iterator arp_table_iter=m_arp_table.begin();
	for(arp_table_iter; arp_table_iter!=m_arp_table.end(); arp_table_iter++)
	{
		if(arp_table_iter->output_device==port && arp_table_iter->neighbour)
		{
			arp_table_iter->valid_period=Simulator::Now()+m_ArpExpire;
			return;
		}
	}
	return;
}

Ptr<NetDevice> TFESwitch::GetPortArpTable(Mac48Address addr)
{
	NS_LOG_FUNCTION(this);
	Ptr<NetDevice> out=NULL;	
	std::vector<ArpTable>::iterator arp_table_iter=m_arp_table.begin();

	for(arp_table_iter; arp_table_iter!=m_arp_table.end(); arp_table_iter++)
	{
		#ifndef MOBILITY
			if(arp_table_iter->MacAddr==addr)
			{
				if(arp_table_iter->neighbour || arp_table_iter->valid_period>=Simulator::Now())
					out=arp_table_iter->output_device;
			}
		#else
			if(arp_table_iter->MacAddr==addr && arp_table_iter->valid_period>=Simulator::Now())
			{
				out=arp_table_iter->output_device;
			}
		#endif
	}
	return out;	
}

void TFESwitch::LearnUpdateTcpTable(Mac48Address source, Mac48Address dst, Ptr<NetDevice> port, PossiblePackets iD)
{
	NS_LOG_FUNCTION(this);
	bool find=false;
	std::vector<TcpTable>::iterator tcp_table_iter=m_tcp_table.begin();
	TcpTable newEntry;
	switch(iD)
	{
		case TCP_SYN:
		case PATH_REQ:
			for(tcp_table_iter=m_tcp_table.begin(); tcp_table_iter!=m_tcp_table.end(); tcp_table_iter++)
			{
				if(tcp_table_iter->MacSrc==source && 
				tcp_table_iter->MacDst==dst
				&& tcp_table_iter->src_port==m_tcp_src && 
				tcp_table_iter->dst_port==m_tcp_dst)
				{
					tcp_table_iter->valid_period=Simulator::Now()+m_TcpExpire;
					return;
				}
			}		
			newEntry.MacSrc=source;
			newEntry.MacDst=dst;
			newEntry.src_port=m_tcp_src;
			newEntry.dst_port=m_tcp_dst;
			newEntry.src_device=port;
			newEntry.valid_period=Simulator::Now()+m_TcpExpire;
			m_tcp_table.push_back(newEntry);
			break;
		default:
			for(tcp_table_iter=m_tcp_table.begin(); tcp_table_iter!=m_tcp_table.end(); tcp_table_iter++)
			{
				if(tcp_table_iter->MacSrc==source && 
				tcp_table_iter->MacDst==dst
				&& tcp_table_iter->src_port==m_tcp_src && 
				tcp_table_iter->dst_port==m_tcp_dst)
				{
					tcp_table_iter->valid_period=Simulator::Now()+m_TcpExpire;
				}
				if(tcp_table_iter->MacSrc==dst && 
				tcp_table_iter->MacDst==source
				&& tcp_table_iter->src_port==m_tcp_dst && 
				tcp_table_iter->dst_port==m_tcp_src)
				{
					if(iD==TCP_SYN_ACK)
					tcp_table_iter->dst_device=port;
					tcp_table_iter->valid_period=Simulator::Now()+m_TcpExpire;
				}
			}
			break;
	}
}

Ptr<NetDevice> TFESwitch::GetPortTcpTable(Mac48Address source, Mac48Address dest, PossiblePackets iD)
{
	NS_LOG_FUNCTION(this);
	Ptr<NetDevice> out=NULL;	
	std::vector<TcpTable>::iterator tcp_table_iter=m_tcp_table.begin();
	for(tcp_table_iter; tcp_table_iter!=m_tcp_table.end(); tcp_table_iter++)
	{
				if(tcp_table_iter->MacSrc==dest && 
				tcp_table_iter->MacDst==source && 
				tcp_table_iter->src_port==m_tcp_dst && 
				tcp_table_iter->dst_port==m_tcp_src)
				{
					out=tcp_table_iter->src_device;
					return out;
				}

				if(tcp_table_iter->MacSrc==source && 
				tcp_table_iter->MacDst==dest && 
				tcp_table_iter->src_port==m_tcp_src && 
				tcp_table_iter->dst_port==m_tcp_dst)
				{
					switch(iD)
					{
						case PATH_REQ:
							out=tcp_table_iter->src_device;
							return out;
							break;
						default:
							out=tcp_table_iter->dst_device;
							return out;
							break;
					}
				}
	}
	return out;
}


std::pair<bool,TcpTable*> TFESwitch::SearchEntryTcpTable(Mac48Address src, Mac48Address dst,uint16_t src_port, uint16_t dst_port)
{
	NS_LOG_LOGIC(this);
	std::vector<TcpTable>::iterator tcp_table_iter;
	std::pair<bool,TcpTable*> return_value;
	for(tcp_table_iter=m_tcp_table.begin(); tcp_table_iter!=m_tcp_table.end(); tcp_table_iter++)
	{
		if(tcp_table_iter->MacSrc==src && tcp_table_iter->MacDst==dst &&
		   tcp_table_iter->src_port==src_port && tcp_table_iter->dst_port==dst_port)
		{
			return_value.first=true;
			return_value.second=&(*tcp_table_iter);
			return return_value;
		}
	}
	return_value.first=false;
	return_value.second=NULL;
	return return_value;
}

Ptr<Packet> TFESwitch::CreatePathPacket(Ptr<const Packet> packet, Mac48Address src ,Mac48Address dst)
{
	NS_LOG_FUNCTION(this);
	Ptr<Packet> newPacket;
	newPacket=packet->Copy();
	PathHeader path_head;
	path_head.SetSource(src);
	path_head.SetOriginalMac(dst);
	path_head.SetTcpPorts(m_tcp_src,m_tcp_dst);
	newPacket->AddHeader(path_head);	
	return newPacket;
}

Ptr<Packet> TFESwitch::DeletePathPacket(Ptr<const Packet> packet)
{
	NS_LOG_FUNCTION(this);
	Ptr<Packet> newPacket;
	newPacket=packet->Copy();
	PathHeader path_head;
	newPacket->RemoveHeader(path_head);
	return newPacket;		
}

Mac48Address  TFESwitch::GetDstPathPaquet(Ptr<const Packet> packet)
{
	Ptr<Packet> newPacket;
	newPacket=packet->Copy();
	PathHeader path_head;	
	newPacket->RemoveHeader(path_head);
	return path_head.GetOriginalMac();
}

void TFESwitch::CleanTcpTable(void)
{
	NS_LOG_FUNCTION(this);
	Simulator::Schedule(m_TcpExpire,&TFESwitch::CleanTcpTable,this);
	std::vector<TcpTable>::iterator tcp_table_iter=m_tcp_table.begin();
	for(tcp_table_iter; tcp_table_iter!=m_tcp_table.end();)
	{
		if(tcp_table_iter->valid_period<Simulator::Now())
		{
			m_tcp_table.erase(tcp_table_iter);
			tcp_table_iter=m_tcp_table.begin();
		}
		else
		{
			tcp_table_iter++;
		}
	}	
}

void TFESwitch::CleanArpTable(void)
{
	NS_LOG_FUNCTION(this);
	std::vector<ArpTable>::iterator arp_table_iter=m_arp_table.begin();
	Simulator::Schedule(Seconds(10),&TFESwitch::CleanArpTable,this);
	for(arp_table_iter; arp_table_iter!=m_arp_table.end();)
	{
		if(arp_table_iter->valid_period<Simulator::Now())
		{
			#ifndef MOBILITY
			if(!arp_table_iter->neighbour)
			{
				m_arp_table.erase(arp_table_iter);
				arp_table_iter=m_arp_table.begin();
			}
			else
			{
				arp_table_iter++;
			}
			#else
				m_arp_table.erase(arp_table_iter);
				arp_table_iter=m_arp_table.begin();
			#endif
			
		}
		else
		{
			arp_table_iter++;
		}
	}
}

void TFESwitch::ResendBroadcast(Ptr<NetDevice> incomingPort, Ptr<Packet> packet, 
    						Mac48Address src, Mac48Address dst, uint16_t protocol)
{
	NS_LOG_FUNCTION(this);
	NetDeviceContainerSwitchTFE::iterator port_iter=m_ports.find(incomingPort);
	switch (m_hierarchy)
	{
		case Core:
			ResendToAllPorts(incomingPort,packet,src,dst,protocol);
			break;
		case Aggr:
			if(port_iter->second==Upper)
			{
				ResendToSelectivePorts(incomingPort,packet,src,dst,protocol,Lower);
			}	
			else
			{							
				ResendToAllPorts(incomingPort,packet,src,dst,protocol);
			}
			break;
		case Tor:
			if(port_iter->second==Lower)
			{					
				ResendToAllPorts(incomingPort,packet,src,dst,protocol);
			}
			else
			{					
				ResendToSelectivePorts(incomingPort,packet,src,dst,protocol,Lower);
			}
			break;
		default:
			NS_ASSERT_MSG(false,"Level: "<<m_hierarchy);
	}
}
	
void TFESwitch::ResendToSelectivePorts(Ptr<NetDevice> incomingPort, Ptr<Packet> packet, 
    						Mac48Address src, Mac48Address dst, uint16_t protocol, TypeOfConnection course)	
{	
	NS_LOG_FUNCTION(this);
	std::vector<bool> choosed_dst;
	int index=0;
	int i=0;
	NetDeviceContainerSwitchTFE::iterator port_iter=m_ports.begin();
	choosed_dst.assign(m_ports.size(),false);
	dst_broadcast->SetAttribute("Min",DoubleValue(0));
	dst_broadcast->SetAttribute("Max",DoubleValue(m_ports.size()));
	for(i=0; i<(m_ports.size()-1); i++)
	{
		do
		{
			index=(int)dst_broadcast->GetValue();
			port_iter=m_ports.find(m_ordered_ports[index]);
			if(port_iter->second!=course)
				choosed_dst[index]=true;
			if(std::count (choosed_dst.begin(), choosed_dst.end(),true)==choosed_dst.size())
				return;
		}
		while(choosed_dst[index]==true || incomingPort==port_iter->first);
		choosed_dst[index]=true;
		port_iter->first->SendFrom(packet->Copy(),src,dst,protocol);
	}
}
	
void TFESwitch::ResendToAllPorts(Ptr<NetDevice> incomingPort, Ptr<Packet> packet, 
    						Mac48Address src, Mac48Address dst, uint16_t protocol)	
{	
	NS_LOG_FUNCTION(this);
	std::vector<bool> choosed_dst;
	int index=0;
	int i=0;
	NetDeviceContainerSwitchTFE::iterator port_iter=m_ports.begin();
	choosed_dst.assign(m_ports.size(),false);
	dst_broadcast->SetAttribute("Min",DoubleValue(0));
	dst_broadcast->SetAttribute("Max",DoubleValue(m_ports.size()));
	for(i=0; i<(m_ports.size()-1); i++)
	{
		do
		{
			index=(int)dst_broadcast->GetValue();
			port_iter=m_ports.find(m_ordered_ports[index]);
		}
		while(choosed_dst[index]==true || incomingPort==port_iter->first);
		choosed_dst[index]=true;
		port_iter->first->SendFrom(packet->Copy(),src,dst,protocol);
	}
}

Ptr<Packet> TFESwitch::IncrementCount(Ptr<const Packet> packet)
{
	NS_LOG_FUNCTION(this);
	Ptr<Packet> packetnew=packet->Copy();
	PathHeader path_head;	
	packetnew->RemoveHeader(path_head);
	path_head.IncremenJumpCount();
	packetnew->AddHeader(path_head);	
	return packetnew;	
}

}

