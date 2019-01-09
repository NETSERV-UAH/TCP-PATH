/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Author: Diego López Pajares <diego.lopezp@uah.es>
 * Derived from bridge-net-device 
 */

#ifndef TFE_SWITCH_HELPER_H
#define TFE_SWITCH_HELPER_H

#include "ns3/net-device-container.h"
#include "ns3/tfe-switch.h"
#include "ns3/object-factory.h"
#include <string>

namespace ns3 {
class Node;
class AttributeValue;
class TFESwitchHelper
{
public:
  TFESwitchHelper ();
  void SetDeviceAttribute (std::string n1, const AttributeValue &v1);
  NetDeviceContainer Install (Ptr<Node> node, NetDeviceContainer c);
  NetDeviceContainer Install (Ptr<Node> node, NetDeviceContainerSwitchTFE c);
  NetDeviceContainer Install (std::string nodeName, NetDeviceContainer c);
private:
  ObjectFactory m_deviceFactory;
};

}
#endif


