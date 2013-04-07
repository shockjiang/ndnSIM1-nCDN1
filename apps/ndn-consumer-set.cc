/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 Tsinghua University, P.R.China
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
 * Author: Xiaoke Jiang <shock.jiang@gmail.com>
 */

#include "ndn-consumer-zipf-mandelbrot.h"

#include "ns3/ndn-app-face.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-content-object.h"

#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"

#include <math.h>


NS_LOG_COMPONENT_DEFINE ("ndn.ConsumerSet");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (ConsumerSet);

TypeId
ConsumerSet::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::ConsumerSet")
    .SetGroupName ("Ndn")
    .SetParent<ConsumerCbr> ()
    .AddConstructor<ConsumerSet> ()
   ;

  return tid;
}


ConsumerSet::ConsumerSet()
  : m_SeqRng (0.0, 1.0)
{
  // SetNumberOfContents is called by NS-3 object system during the initialization
}

ConsumerSet::~ConsumerSet()
{
}

void
ConsumerSet::SendPacket() {
	  if (!m_active) return;

	  NS_LOG_FUNCTION_NOARGS ();

	  uint32_t seq=std::numeric_limits<uint32_t>::max (); //invalid
	  bool isRetx = false;
	  while (m_retxSeqs.size ())
	    {
	      isRetx = true;
		  seq = *m_retxSeqs.begin ();
	      m_retxSeqs.erase (m_retxSeqs.begin ());

	      break;
	    }

	  if (seq == std::numeric_limits<uint32_t>::max ())
	    {
	      if (m_seqMax != std::numeric_limits<uint32_t>::max ())
	        {
	          if (m_seq >= m_seqMax)
	            {
	              return; // we are totally done
	            }
	        }

	      seq = m_seq;
	      m_seq += Interest::SET_MOD;
	    }

	  //

	  // NS_LOG_INFO ("Requesting Interest: \n" << interestHeader);

		  Ptr<Name> nameWithSequence = Create<Name> (m_interestName);
		  (*nameWithSequence) (seq);
		  if (!isRetx) {
			  Name name = xiaoke::Translate2Set(*nameWithSequence);
			  nameWithSequence = Create<Name>(name);
		  }

		  Interest interestHeader;
		  interestHeader.SetNonce               (m_rand.GetValue ());
		  interestHeader.SetName                (nameWithSequence);
		  interestHeader.SetInterestLifetime    (m_interestLifeTime);

		  NS_LOG_INFO ("> Interest for " << seq<<" isSet:"<<isRetx);


	  Ptr<Packet> packet = Create<Packet> ();
	  packet->AddHeader (interestHeader);
	  NS_LOG_DEBUG ("Interest packet size: " << packet->GetSize ());

	  NS_LOG_DEBUG ("Trying to add " << seq << " with " << Simulator::Now () << ". already " << m_seqTimeouts.size () << " items");

	  m_seqTimeouts.insert (SeqTimeout (seq, Simulator::Now ()));
	  m_seqFullDelay.insert (SeqTimeout (seq, Simulator::Now ()));

	  m_seqLastDelay.erase (seq);
	  m_seqLastDelay.insert (SeqTimeout (seq, Simulator::Now ()));

	  m_seqRetxCounts[seq] ++;
	  NS_LOG_FUNCTION("Send Interest "<<seq<<" finished3");
	  m_transmittedInterests (&interestHeader, this, m_face);

	  m_rtt->SentSeq (SequenceNumber32 (seq), 1);
	  NS_LOG_FUNCTION("Send Interest "<<seq<<" finished2");
	  FwHopCountTag hopCountTag;
	  packet->AddPacketTag (hopCountTag);
	  NS_LOG_INFO("Send Interest1 "<<seq<<" finished");
	  m_protocolHandler (packet);
	  NS_LOG_INFO("Send Interest0 "<<seq<<" finished");
	  ConsumerSet::ScheduleNextPacket ();
	  NS_LOG_INFO("Send Interest "<<seq<<" finished");
}


uint32_t
ConsumerSet::GetNextSeq()
{
  uint32_t content_index = 1; //[1, m_N]
  return content_index;
}

void
ConsumerSet::ScheduleNextPacket() {

  if (m_firstTime)
    {
      m_sendEvent = Simulator::Schedule (Seconds (0.0),
                                         &ConsumerSet::SendPacket, this);
      m_firstTime = false;
    }
  else if (!m_sendEvent.IsRunning ())
    m_sendEvent = Simulator::Schedule (
                                       (m_random == 0) ?
                                       Seconds(1.0 / m_frequency)
                                       :
                                       Seconds(m_random->GetValue ()),
                                       &ConsumerSet::SendPacket, this);
}

} /* namespace ndn */
} /* namespace ns3 */
