#include <iomanip>
#include <iostream>
#include "ns3/assert.h"
#include "ns3/path-header.h"
#include "ns3/log.h"
#include "ns3/address-utils.h"

namespace ns3 {
 
NS_LOG_COMPONENT_DEFINE ("PathHeader");
NS_OBJECT_ENSURE_REGISTERED (PathHeader);


PathHeader::PathHeader()
{
	NS_LOG_FUNCTION(this);
	m_dst=Mac48Address::GetBroadcast();
	m_jump_count=0;
}

TypeId PathHeader::GetTypeId (void)
{
  	static TypeId tid = TypeId ("PathHeader")
    .SetParent<Header> ()
    .AddConstructor<PathHeader> ();
  	return tid;
}

TypeId PathHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t PathHeader::GetSerializedSize (void) const
{
  return ((TCP_PORT_SIZE*2)+(MAC_ADDR_SIZE*3)+2);
}

void PathHeader::Serialize (Buffer::Iterator start) const
{
	NS_LOG_FUNCTION (this << &start);
	Buffer::Iterator i = start;

		WriteTo (i, m_dst);
		WriteTo (i, m_src);
		WriteTo (i, m_path);
		i.WriteHtonU16 (m_tcp_src);
		i.WriteHtonU16 (m_tcp_dst);
		i.WriteHtonU16 (m_jump_count);
}

uint32_t PathHeader::Deserialize (Buffer::Iterator start)
{
	NS_LOG_FUNCTION (this << &start);
	Buffer::Iterator i = start;
 
	ReadFrom (i, m_dst);
  	ReadFrom (i, m_src);
  	ReadFrom (i, m_path);
  	m_tcp_src=i.ReadNtohU16 ();
  	m_tcp_dst=i.ReadNtohU16 ();
  	m_jump_count=i.ReadNtohU16 ();
  	  	
  	return GetSerializedSize ();
}

void PathHeader::Print (std::ostream &os) const
{
	NS_LOG_FUNCTION (this);
	
	os << " DST: " <<m_dst<<" SRC: "<< m_src
  		<< " Original DST: " << m_path
		<< " Tcp src: " << m_tcp_src
		<< " Tcp dst: " << m_tcp_dst
		<< " Jump count: "<<m_jump_count;
	
}

void PathHeader::SetSource(Mac48Address src)
{
	NS_LOG_FUNCTION (this);
	m_src=src;
}

void PathHeader::SetDestination(Mac48Address dst)
{
	NS_LOG_FUNCTION (this);
	m_dst=dst;
}

void PathHeader::SetOriginalMac(Mac48Address path)
{
	NS_LOG_FUNCTION (this);
	m_path=path;
}

Mac48Address PathHeader::GetSource(void)
{
	NS_LOG_FUNCTION (this);
	return m_src;
}

Mac48Address PathHeader::GetDestination(void)
{
	NS_LOG_FUNCTION (this);
	return m_dst;
}

Mac48Address PathHeader::GetOriginalMac(void)
{
	NS_LOG_FUNCTION (this);
	return m_path;
}

void PathHeader::SetTcpPorts(uint16_t src, uint16_t dst)
{
	NS_LOG_FUNCTION (this);
	m_tcp_src=src;
	m_tcp_dst=dst;
}

uint16_t PathHeader::GetTcpSrc(void)
{
	NS_LOG_FUNCTION (this);
	return m_tcp_src;
}

uint16_t PathHeader::GetTcpDst(void)
{
	NS_LOG_FUNCTION (this);
	return m_tcp_dst;
}

void PathHeader::IncremenJumpCount(void)
{
	NS_LOG_FUNCTION (this);
	m_jump_count+=1;
}
  		
uint16_t PathHeader::GetJumpCount(void)
{
	NS_LOG_FUNCTION (this);
	return m_jump_count;
}

}