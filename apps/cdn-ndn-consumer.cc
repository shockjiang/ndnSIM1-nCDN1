/*
 * cdn-ndn-consumer.cc
 *
 *  Created on: Jun 27, 2013
 *      Author: shock
 */

#include "cdn-ndn-consumer.h"
#include "ns3/ndn-app-face.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-content-object.h"

#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"
#include "ndn-consumer-zipf-mandelbrot.h"
#include <sstream>
#include <math.h>

NS_LOG_COMPONENT_DEFINE ("ndn.CDNConsumer");

using namespace std;

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (CDNConsumer);



TypeId
CDNConsumer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::CDNConsumer")
    .SetGroupName ("Ndn")
    .SetParent<ConsumerZipfMandelbrot> ()
    .AddConstructor<CDNConsumer> ()
    .AddAttribute ("ReTxMax","Max trial time of requesting",
    				IntegerValue (0),
    				MakeIntegerAccessor (&CDNConsumer::m_reTxMax),
    				MakeIntegerChecker<int32_t> ())

    .AddTraceSource ("TimeoutRequest", "Timeout request during PIT checking",
                    MakeTraceSourceAccessor (&CDNConsumer::m_timeoutRequest))

	.AddTraceSource ("NewRequest", "New Request, no retransmission or on transmission",
					MakeTraceSourceAccessor (&CDNConsumer::m_newRequest))
    ;

  return tid;
}

CDNConsumer::CDNConsumer() {
	// TODO Auto-generated constructor stub

}

CDNConsumer::~CDNConsumer() {
	// TODO Auto-generated destructor stub
}


void
CDNConsumer::OnNack (const Ptr<const InterestHeader> &interest, Ptr<Packet> packet){

	//uint32_t seq = boost::lexical_cast<uint32_t> (interest->GetName ().GetComponents ().back ());
	Consumer::OnNack(interest, packet);
}

Time
CDNConsumer::GetRetxTimer () const
{
  return m_retxTimer;
}

void
CDNConsumer::SetRetxTimer (Time retxTimer)
{
  m_retxTimer = retxTimer;
  if (m_retxEvent.IsRunning ())
    {
      // m_retxEvent.Cancel (); // cancel any scheduled cleanup events
      Simulator::Remove (m_retxEvent); // slower, but better for memory
    }

  // schedule even with new timeout
  m_retxEvent = Simulator::Schedule (m_retxTimer,
                                     &CDNConsumer::CheckRetxTimeout, this);
}

void
CDNConsumer::CheckRetxTimeout ()
{
  Time now = Simulator::Now ();

  Time rto = m_rtt->RetransmitTimeout ();
  // NS_LOG_DEBUG ("Current RTO: " << rto.ToDouble (Time::S) << "s");

  while (!m_seqTimeouts.empty ())
    {
      SeqTimeoutsContainer::index<i_timestamp>::type::iterator entry =
        m_seqTimeouts.get<i_timestamp> ().begin ();
      if (entry->time + rto <= now) // timeout expired?
        {
          uint32_t seqNo = entry->seq;
          m_seqTimeouts.get<i_timestamp> ().erase (entry);
          CDNConsumer::OnTimeout (seqNo);
        }
      else
        break; // nothing else to do. All later packets need not be retransmitted
    }

  m_retxEvent = Simulator::Schedule (m_retxTimer,
                                     &CDNConsumer::CheckRetxTimeout, this);
}

void
CDNConsumer::OnTimeout (uint32_t sequenceNumber) {
	m_timeoutRequest(this, sequenceNumber);
	NS_LOG_FUNCTION(sequenceNumber);
	Consumer::OnTimeout(sequenceNumber);
}


void
CDNConsumer::OnContentObject (const Ptr<const ContentObjectHeader> &contentObject,
                   Ptr<Packet> payload) {
//	NameComponents name = contentObject->GetName();
//	Name::const_iterator iter = name.begin();
//	Name::const_iterator next = iter;
//	next ++;
//	NameComponents source;
//	while (next != name.end()) {
//		source.Add(*iter);
//
//		iter ++;
//		next ++;
//	}
//	NS_LOG_LOGIC("ContentName="<<contentObject->GetName()<<". source="<<source);
	Consumer::OnContentObject (contentObject, payload);
}


void
CDNConsumer::SendPacket() {

	  if (!m_active) return;

	  NS_LOG_FUNCTION_NOARGS ();

	  uint32_t seq=std::numeric_limits<uint32_t>::max (); //invalid
	  if (CDNConsumer::m_reTxMax == 0)
	  {
		  seq = ConsumerZipfMandelbrot::GetNextSeq();
	  } else
	  {

		  std::set<uint32_t>::iterator it = m_retxSeqs.begin();
		  while (it != m_retxSeqs.end())
		  {
			  uint32_t tt = *it;
			  if (m_seqRetxCounts[tt] >= m_reTxMax)
			  {
				  it ++;
			  } else
			  {
				  seq = tt;
				  break;
			  }
		  }
		  m_retxSeqs.erase(m_retxSeqs.begin(), it);

		  if (seq == std::numeric_limits<uint32_t>::max ())
		    {
			  seq = ConsumerZipfMandelbrot::GetNextSeq();
			  while (m_seqTimeouts.find(seq) != m_seqTimeouts.end())
			  {
				  seq = ConsumerZipfMandelbrot::GetNextSeq();
				  m_newRequest(this, seq);
			  }
		    }

	  }


	  Ptr<NameComponents> nameWithSequence = Create<NameComponents> (m_interestName);
	  (*nameWithSequence) (seq);
	  //

	  InterestHeader interestHeader;
	  interestHeader.SetNonce (m_rand.GetValue ());
	  interestHeader.SetName  (nameWithSequence);

	  // NS_LOG_INFO ("Requesting Interest: \n" << interestHeader);
	  NS_LOG_INFO ("> Interest for " << seq<<", Total: "<<m_seq<<", face: "<<m_face->GetId());

	  Ptr<Packet> packet = Create<Packet> ();

	  //NS_LOG_LOGIC ("= Interest for " << seq<<", Total: "<<m_seq<<", face: "<<m_face->GetId());
	  packet->AddHeader (interestHeader);


	  //NS_LOG_LOGIC ("Interest packet size: " << packet->GetSize ());

	  NS_LOG_LOGIC ("Trying to add " << seq << " with " << Simulator::Now () << ". already " << m_seqTimeouts.size () << " items");

	  m_seqTimeouts.insert (SeqTimeout (seq, Simulator::Now ()));
	  m_seqFullDelay.insert (SeqTimeout (seq, Simulator::Now ()));

	  m_seqLastDelay.erase (seq);
	  m_seqLastDelay.insert (SeqTimeout (seq, Simulator::Now ()));

	  m_seqRetxCounts[seq] ++;

	  m_transmittedInterests (&interestHeader, this, m_face);

	  m_rtt->SentSeq (SequenceNumber32 (seq), 1);

	  FwHopCountTag hopCountTag;
	  packet->AddPacketTag (hopCountTag);

	  m_protocolHandler (packet);

	  m_seq ++;

	  CDNConsumer::ScheduleNextPacket ();
}


void
CDNConsumer::ScheduleNextPacket ()
{
  // double mean = 8.0 * m_payloadSize / m_desiredRate.GetBitRate ();
  // std::cout << "next: " << Simulator::Now().ToDouble(Time::S) + mean << "s\n";

  if (m_firstTime)
    {
      m_sendEvent = Simulator::Schedule (Seconds (0.0),
                                         &CDNConsumer::SendPacket, this);
      m_firstTime = false;
    }
  else if (!m_sendEvent.IsRunning ())
    m_sendEvent = Simulator::Schedule (
                                       (m_random == 0) ?
                                         Seconds(1.0 / m_frequency)
                                       :
                                         Seconds(m_random->GetValue ()),
                                       &CDNConsumer::SendPacket, this);

}



} /* namespace ndn */
} /* namespace ns3 */
