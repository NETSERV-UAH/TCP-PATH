


#ifndef PATH_HEADER_H
#define PATH_HEADER_H
 
#include "ns3/header.h"
#include <string>
#include "ns3/mac48-address.h"


namespace ns3 {
class PathHeader : public Header
{
	public:
		PathHeader();
  		static TypeId GetTypeId (void);
  		virtual TypeId GetInstanceTypeId (void) const;
  		virtual uint32_t GetSerializedSize (void) const;
  		virtual void Serialize (Buffer::Iterator start) const;
  		virtual uint32_t Deserialize (Buffer::Iterator start);
  		virtual void Print (std::ostream &os) const;
  		
  		void SetSource(Mac48Address src);
  		void SetDestination(Mac48Address dst);
  		void SetOriginalMac(Mac48Address path);
  		Mac48Address GetSource(void);
  		Mac48Address GetDestination(void);
  		Mac48Address GetOriginalMac(void);
  		
  		void SetTcpPorts(uint16_t src, uint16_t dst);
  		uint16_t GetTcpSrc(void);
  		uint16_t GetTcpDst(void);
  		
  		void IncremenJumpCount(void);
  		uint16_t GetJumpCount(void);

	private:
		static const int TCP_PORT_SIZE = 2;    
  		static const int MAC_ADDR_SIZE = 6; 

  //Address
  Mac48Address m_src;
  Mac48Address m_dst;
  Mac48Address m_path;
  uint16_t m_jump_count;
  uint16_t m_tcp_src;
  uint16_t m_tcp_dst;
};
}

#endif