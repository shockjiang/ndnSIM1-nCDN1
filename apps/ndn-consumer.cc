/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 University of California, Los Angeles
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
 * Author: Ilya Moiseenko <iliamo@cs.ucla.edu>
 */

#include "ndn-consumer.h"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"

#include "ns3/ndn-app-face.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-content-object.h"
// #include "ns3/weights-path-stretch-tag.h"

#include <boost/ref.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

#include "ns3/names.h"

namespace ll = boost::lambda;

NS_LOG_COMPONENT_DEFINE ("ndn.Consumer");

namespace ns3 {
namespace ndn {
    
NS_OBJECT_ENSURE_REGISTERED (Consumer);
    
TypeId
Consumer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::Consumer")
    .SetGroupName ("Ndn")
    .SetParent<App> ()
    .AddAttribute ("StartSeq", "Initial sequence number",
                   IntegerValue (0),
                   MakeIntegerAccessor(&Consumer::m_seq),
                   MakeIntegerChecker<int32_t>())

    .AddAttribute ("Prefix","Name of the Interest",
                   StringValue ("/"),
                   MakeNameComponentsAccessor (&Consumer::m_interestName),
                   MakeNameComponentsChecker ())
    .AddAttribute ("LifeTime", "LifeTime for interest packet",
                   StringValue ("2s"),
                   MakeTimeAccessor (&Consumer::m_interestLifeTime),
                   MakeTimeChecker ())

    .AddAttribute ("RetxTimer",
                   "Timeout defining how frequent retransmission timeouts should be checked",
                   StringValue ("50ms"),
                   MakeTimeAccessor (&Consumer::GetRetxTimer, &Consumer::SetRetxTimer),
                   MakeTimeChecker ())

    .AddTraceSource ("PathWeightsTrace", "PathWeightsTrace",
                    MakeTraceSourceAccessor (&Consumer::m_pathWeightsTrace))
    ;

  return tid;
}
    
Consumer::Consumer ()
  : m_rand (0, std::numeric_limits<uint32_t>::max ())
  , m_seq (0)
  , m_seqMax (0) // don't request anything
{
  NS_LOG_FUNCTION_NOARGS ();

  m_rtt = CreateObject<RttMeanDeviation> ();
}

void
Consumer::SetRetxTimer (Time retxTimer)
{
  m_retxTimer = retxTimer;
  if (m_retxEvent.IsRunning ())
    {
      // m_retxEvent.Cancel (); // cancel any scheduled cleanup events
      Simulator::Remove (m_retxEvent); // slower, but better for memory
    }

  // schedule even with new timeout
  m_retxEvent = Simulator::Schedule (m_retxTimer,
                                     &Consumer::CheckRetxTimeout, this); 
}

Time
Consumer::GetRetxTimer () const
{
  return m_retxTimer;
}

void
Consumer::CheckRetxTimeout ()
{
  Time now = Simulator::Now ();

  Time rto = m_rtt->RetransmitTimeout ();

  while (!m_seqTimeouts.empty ())
    {
      SeqTimeoutsContainer::index<i_timestamp>::type::iterator entry =
        m_seqTimeouts.get<i_timestamp> ().begin ();
      if (entry->time + rto <= now) // timeout expired?
        {
          uint32_t seqNo = entry->seq;
          m_seqTimeouts.get<i_timestamp> ().erase (entry);
          OnTimeout (seqNo);
        }
      else
        break; // nothing else to do. All later packets need not be retransmitted
    }

  m_retxEvent = Simulator::Schedule (m_retxTimer,
                                     &Consumer::CheckRetxTimeout, this); 
}

// Application Methods
void 
Consumer::StartApplication () // Called at time specified by Start
{
  NS_LOG_FUNCTION_NOARGS ();

  // do base stuff
  App::StartApplication ();

  ScheduleNextPacket ();
}
    
void 
Consumer::StopApplication () // Called at time specified by Stop
{
  NS_LOG_FUNCTION_NOARGS ();

  // cancel periodic packet generation
  Simulator::Cancel (m_sendEvent);

  // cleanup base stuff
  App::StopApplication ();
}
    
void
Consumer::SendPacket ()
{
  if (!m_active) return;

  NS_LOG_FUNCTION_NOARGS ();

  uint32_t seq=std::numeric_limits<uint32_t>::max (); //invalid

  // std::cout << Simulator::Now ().ToDouble (Time::S) << "s max -> " << m_seqMax << "\n";
  
  while (m_retxSeqs.size ())
    {
      seq = *m_retxSeqs.begin ();
      m_retxSeqs.erase (m_retxSeqs.begin ());

      // NS_ASSERT (m_seqLifetimes.find (seq) != m_seqLifetimes.end ());
      // if (m_seqLifetimes.find (seq)->time <= Simulator::Now ())
      //   {
          
      //     NS_LOG_DEBUG ("Expire " << seq);
      //     m_seqLifetimes.erase (seq); // lifetime expired. Trying to find another unexpired sequence number
      //     continue;
      //   }
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
      
      seq = m_seq++;
    }
  
  // std::cout << Simulator::Now ().ToDouble (Time::S) << "s -> " << seq << "\n";
  
  //
  Ptr<NameComponents> nameWithSequence = Create<NameComponents> (m_interestName);
  (*nameWithSequence) (seq);
  //

  InterestHeader interestHeader;
  interestHeader.SetNonce               (m_rand.GetValue ());
  interestHeader.SetName                (nameWithSequence);
        
  // NS_LOG_INFO ("Requesting Interest: \n" << interestHeader);
  NS_LOG_INFO (">Interest for " << interestHeader.GetName()<<", node: "<<GetNode()->GetId());

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (interestHeader);
  //NS_LOG_DEBUG ("Interest packet size: " << packet->GetSize ());

  //NS_LOG_DEBUG ("Trying to add " << seq << " with " << Simulator::Now () << ". already " << m_seqTimeouts.size () << " items");
  
  m_seqTimeouts.insert (SeqTimeout (seq, Simulator::Now ()));
  m_seqLifetimes.insert (SeqTimeout (seq, Simulator::Now () + m_interestLifeTime)); // only one insert will work. if entry exists, nothing will happen... nothing should happen
  m_transmittedInterests (&interestHeader, this, m_face);

  m_rtt->SentSeq (SequenceNumber32 (seq), 1);

  m_protocolHandler (packet);

  ScheduleNextPacket ();
}

///////////////////////////////////////////////////
//          Process incoming packets             //
///////////////////////////////////////////////////


void
Consumer::OnContentObject (const Ptr<const ContentObjectHeader> &contentObject,
                               Ptr<Packet> payload)
{
  if (!m_active) return;

  App::OnContentObject (contentObject, payload); // tracing inside
  
  NS_LOG_FUNCTION (this << contentObject << payload);

  // NS_LOG_INFO ("Received content object: " << boost::cref(*contentObject));
  
  uint32_t seq = boost::lexical_cast<uint32_t> (contentObject->GetName ().GetComponents ().back ());
  NS_LOG_INFO ("=DATA for "<< Consumer::m_interestName<<"/" << seq<<" arrived. node: "<<GetNode()->GetId());

  // SeqTimeoutsContainer::iterator entry = m_seqTimeouts.find (seq);

  //  NS_ASSERT_MSG (entry != m_seqTimeouts.end (),
  //             "Comment out this assert, if it causes problems");

  // if (entry != m_seqTimeouts.end ())
  //   m_seqTimeouts.erase (entry);

  m_seqLifetimes.erase (seq);
  m_seqTimeouts.erase (seq);
  m_retxSeqs.erase (seq);

  m_rtt->AckSeq (SequenceNumber32 (seq));

  // Ptr<const WeightsPathStretchTag> tag = payload->RemovePacketTag<WeightsPathStretchTag> ();
  // if (tag != 0)
  //   {
  //     // Notify trace about path weights vector (e.g., for path-stretch calculation)
  //     m_pathWeightsTrace (GetNode (), tag->GetSourceNode (), seq, tag->GetTotalWeight ());
  //     // if (Names::FindName (GetNode ()) == "36")// || Names::FindName (GetNode ()) == "40"|| Names::FindName (GetNode ()) == "5")
  //     //   std::cout << Simulator::Now () << "\t" << boost::cref(*tag) << " = " << tag->GetTotalWeight () << "\n";
  //   }
}

void
Consumer::OnNack (const Ptr<const InterestHeader> &interest, Ptr<Packet> origPacket)
{
  if (!m_active) return;
  
  App::OnNack (interest, origPacket); // tracing inside
  
  // NS_LOG_DEBUG ("Nack type: " << interest->GetNack ());

  // NS_LOG_FUNCTION (interest->GetName ());

  // NS_LOG_INFO ("Received NACK: " << boost::cref(*interest));
  uint32_t seq = boost::lexical_cast<uint32_t> (interest->GetName ().GetComponents ().back ());
  NS_LOG_INFO ("< NACK for " << seq);
  // std::cout << Simulator::Now ().ToDouble (Time::S) << "s -> " << "NACK for " << seq << "\n"; 

  // put in the queue of interests to be retransmitted
  // NS_LOG_INFO ("Before: " << m_retxSeqs.size ());
  m_retxSeqs.insert (seq);
  // NS_LOG_INFO ("After: " << m_retxSeqs.size ());

  m_rtt->IncreaseMultiplier ();             // Double the next RTO ??
  ScheduleNextPacket ();
}

void
Consumer::OnTimeout (uint32_t sequenceNumber)
{
  NS_LOG_FUNCTION (sequenceNumber);
  // std::cout << Simulator::Now () << ", TO: " << sequenceNumber << ", current RTO: " << m_rtt->RetransmitTimeout ().ToDouble (Time::S) << "s\n";

  m_rtt->IncreaseMultiplier ();             // Double the next RTO
  m_rtt->SentSeq (SequenceNumber32 (sequenceNumber), 1); // make sure to disable RTT calculation for this sample
  m_retxSeqs.insert (sequenceNumber);
  ScheduleNextPacket (); 
}

} // namespace ndn
} // namespace ns3
