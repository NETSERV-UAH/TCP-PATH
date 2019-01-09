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


#include "tfe-switch-helper.h"
#include "ns3/log.h"
#include "ns3/tfe-switch.h"
#include "ns3/node.h"
#include "ns3/names.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TFESwitchHelper");

TFESwitchHelper::TFESwitchHelper ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_deviceFactory.SetTypeId ("ns3::TFESwitch");
}
void 
TFESwitchHelper::SetDeviceAttribute (std::string n1, const AttributeValue &v1)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_deviceFactory.Set (n1, v1);
}
NetDeviceContainer
TFESwitchHelper::Install (Ptr<Node> node, NetDeviceContainer c)
{
	NS_ASSERT_MSG(false,"NOT VALID FUNCTION");
}

NetDeviceContainer
TFESwitchHelper::Install (Ptr<Node> node, NetDeviceContainerSwitchTFE c)
{
  NS_LOG_FUNCTION_NOARGS ();

  NetDeviceContainer devs;
  NetDeviceSwitchTFE extra_container;
  Ptr<TFESwitch> dev = m_deviceFactory.Create<TFESwitch> ();
  devs.Add (dev);
  node->AddDevice (dev);

  for (NetDeviceContainerSwitchTFE::iterator i = c.begin (); i != c.end (); ++i)
    {
      extra_container.first=i->first;
      extra_container.second=i->second;
      dev->AddBridgePort (extra_container);
    }
  return devs;
}


NetDeviceContainer
TFESwitchHelper::Install (std::string nodeName, NetDeviceContainer c)
{
  NS_LOG_FUNCTION_NOARGS ();
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return Install (node, c);
}
}
