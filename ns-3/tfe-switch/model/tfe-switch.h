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

#ifndef TFE_SWITCH_H
#define TFE_SWITCH_H
#include "ns3/bridge-net-device.h"
#include "ns3/node.h"
#include "ns3/net-device.h"
#include "ns3/mac48-address.h"
#include "ns3/nstime.h"
#include "ns3/net-device.h"
#include "ns3/path-header.h"
#include <time.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include "ns3/output-stream-wrapper.h"
#include "ns3/trace-helper.h"
#include "ns3/random-variable-stream.h"
#include "ns3/double.h"

#define ARPPATH 1
#define TFEPATH 2
#define TCPPATH 3
namespace ns3 {
typedef std::map<Ptr<NetDevice>,Time>  helloTable;
typedef struct neighbour
		{
			Ptr<NetDevice> device;
			Mac48Address direction;
			Time period;
			
		}NeighbourTable;		
typedef struct tcp
		{
			Mac48Address MacSrc;
			Mac48Address MacDst;
			uint16_t src_port;
			uint16_t dst_port;
			Ptr<NetDevice> src_device;
			Ptr<NetDevice> dst_device;
			Time valid_period;
			
		}TcpTable;	
typedef struct arp
		{
			Mac48Address MacAddr;
			Ptr<NetDevice> output_device;
			bool neighbour;
			Time valid_period;
			
		}ArpTable;					
typedef enum packets
		{
			TCP_SYN, TCP_SYN_ACK, TCP_ACK, TCP_FIN_ACK, TCP_OTHER, ARP_REQ, ARP_REP, ETH_BROAD, OTHERS, PATH_REQ
			
		}PossiblePackets;		
enum HierarchySwitch {Core=3, Aggr=2, Tor=1, Err=0};
enum TypeOfConnection {Upper=2, Lower=1};	
typedef std::pair<Ptr<NetDevice>,TypeOfConnection> NetDeviceSwitchTFE;
typedef std::map<Ptr<NetDevice>,TypeOfConnection> NetDeviceContainerSwitchTFE;

class Node;
class TFESwitch : public BridgeNetDevice
{
	public:
	
		static TypeId GetTypeId (void);
  		TFESwitch ();
  		virtual ~TFESwitch ();
  		
  		  void AddBridgePort (NetDeviceSwitchTFE  bridgePort);
  		
  		
  	private:
  		void SendPeriodicHello(void);
  		void RcvHelloPacket(Ptr<NetDevice> port_in, Mac48Address src);
  		bool SearchEntryHelloTable(Ptr<NetDevice> port);
  		virtual void ReceiveFromDevice (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet, uint16_t protocol,
                                    Address const &src, Address const &dst, PacketType packetType);                                   
        void ModeArppath (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet,
                                 uint16_t protocol, Mac48Address src, Mac48Address dst, PossiblePackets packetiD);       
        void ModeTcppath (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet,
                                 uint16_t protocol, Mac48Address src, Mac48Address dst, PossiblePackets packetiD);                                
        void LearnUpdateArpTable(Mac48Address source, Ptr<NetDevice> port, PossiblePackets iD);
        void LearnUpdateTcpTable(Mac48Address source, Mac48Address dst, Ptr<NetDevice> port, PossiblePackets iD);        
        std::pair<bool,TcpTable*> SearchEntryTcpTable(Mac48Address src, Mac48Address dst,uint16_t src_port, uint16_t dst_port);        
        Ptr<NetDevice> GetPortArpTable(Mac48Address addr);
        Ptr<NetDevice> GetPortTcpTable(Mac48Address source, Mac48Address dest, PossiblePackets iD);        
        void CleanTcpTable(void);    
        void CleanArpTable(void);
        void GetTableSize(void);       
        Ptr<Packet> IncrementCount(Ptr<const Packet> packet);
        void GetCount(Ptr<Packet> packet);
        void CreateStoreFiles(void);
    	void UpdateNeighbourArpTable(Ptr<NetDevice> port);
        PossiblePackets TypeOfPacket(Ptr<const Packet> packet, uint16_t protocol);
        Ptr<Packet> CreatePathPacket(Ptr<const Packet> packet, Mac48Address src, Mac48Address dst);
        Ptr<Packet> DeletePathPacket(Ptr<const Packet> packet);
        Mac48Address  GetDstPathPaquet(Ptr<const Packet> packet);        
    	void ResendBroadcast(Ptr<NetDevice> incomingPort, Ptr<Packet> packet, 
    						Mac48Address src, Mac48Address dst, uint16_t protocol);
    	void ResendToAllPorts(Ptr<NetDevice> incomingPort, Ptr<Packet> packet, 
    						Mac48Address src, Mac48Address dst, uint16_t protocol);
    	void ResendToSelectivePorts(Ptr<NetDevice> incomingPort, Ptr<Packet> packet, Mac48Address src, 
    						Mac48Address dst, uint16_t protocol, TypeOfConnection course);
  		Ptr<BridgeChannel> m_channel;
  		Time m_hello_period;
  		uint16_t m_switch_mode;
  		std::vector<NeighbourTable> m_neighbour;
  		std::vector<TcpTable> m_tcp_table;
  		std::vector<ArpTable> m_arp_table;
  		Time m_ArpBlock;
  		Time m_ArpExpire;
  		Time m_TcpBlock;
  		Time m_TcpExpire;
  		uint16_t m_tcp_src;
  		uint16_t m_tcp_dst;
  		Ptr<UniformRandomVariable> dst_broadcast;
  		bool m_tor;
  		AsciiTraceHelper asciiTraceHelper; 
      	Ptr<OutputStreamWrapper> file;
      	Ptr<OutputStreamWrapper> sw_estadistics_file;
      	Ptr<OutputStreamWrapper> sw_tables_file;
      	std::string m_folder;
  		NetDeviceContainerSwitchTFE m_ports;
  		std::vector<Ptr<NetDevice>> m_ordered_ports;
  		HierarchySwitch m_hierarchy;
};
}

#endif /* TFE_SWITCH_H */

