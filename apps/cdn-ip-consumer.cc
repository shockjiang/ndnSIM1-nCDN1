/*
 * cdn-ip-app.cc
 *
 *  Created on: Mar 11, 2013
 *      Author: shock
 */

#include "cdn-ip-consumer.h"

#include "ns3/ndn-app-face.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-content-object.h"

#include "ns3/names.h"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"
#include "ndn-consumer-zipf-mandelbrot.h"
#include <sstream>
#include <math.h>

NS_LOG_COMPONENT_DEFINE ("ndn.CDNIPConsumer");

using namespace std;

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (CDNIPConsumer);

TypeId
CDNIPConsumer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::CDNIPConsumer")
    .SetGroupName ("Ndn")
    .SetParent<CDNConsumer> ()
    .AddConstructor<CDNIPConsumer> ()

//   .AddAttribute ("Producers", "Data Producers",
//						  StringValue ("0"),
//						  //MakeDoubleAccessor (&CDNIPApp::m_SeqType),
//						  //MakeUintegerChecker<uint32_t> ())
//						  //MakeTimeAccessor (&Consumer::GetRetxTimer, &Consumer::SetRetxTimer),
//						  //MakeNameComponentsAccessor (&CDNIPApp::m_interestName),
//
//						  MakeStringAccessor (&CDNIPConsumer::GetProducers, &CDNIPConsumer::SetProducers),
//						  MakeStringChecker ())
////						  MakeObjectVectorAccessor (&CDNIPApp::m_Producers),
////						  MakeObjectVectorChecker ())
//						  //MakeNameComponentsAccessor (&CDNIPApp::GetDataSources, &CDNIPApp::SetDataSources),
//						  //MakeNameComponentsChecker ())
	.AddAttribute ("EnableMulticast", "Enabling support of Multicast",
				   BooleanValue (false),
				   MakeBooleanAccessor (&CDNIPConsumer::m_enableMulticast),
				   MakeBooleanChecker ())
	.AddTraceSource ("ChangeProducer", "Change producer",
					MakeTraceSourceAccessor (&CDNIPConsumer::m_changeProducer))    ;

  return tid;
}


CDNIPConsumer::CDNIPConsumer()
{
//	vector<NameComponents>::size_type ix = 0;
//	for (; ix !=m_producers.size(); ++ix) {
//		if (m_Producers[ix] == m_interestName) {
//			break;
//		}
//	}
//	if (ix == m_Producers.size()) {
//		m_producers.push_back(m_interestName);
//	}

  // SetNumberOfContents is called by NS-3 object system during the initialization
}



void
CDNIPConsumer::ChangeProducer(){

	  //Ptr<ndn::fib::Entry> cur =
	NS_LOG_FUNCTION("begin to change producer");
	  Ptr<ndn::Fib> fib = this->GetNode() ->GetObject<ndn::Fib> ();
	  if (fib == 0) {
		  NS_LOG_INFO("fib==0");
		  return;
	  }
	  NS_LOG_LOGIC("fib size = "<<fib->GetSize());
	  Ptr<ndn::fib::Entry> tmp = fib->Begin();;

	  if (cur == fib->Begin())
	  {
		  tmp = fib->Next(cur);
	  }
	  if (tmp == fib->End())
		  return;

	  int32_t select_cost = tmp->FindBestCandidate(0).GetRoutingCost();
	  Time select_delay = tmp->FindBestCandidate(0).GetRealDelay();

	  for (Ptr<ndn::fib::Entry> entry = fib->Begin (); entry != fib->End (); entry = fib->Next (entry))
	     {
		  if (entry == cur)
		  {
			  NS_LOG_LOGIC("pass current choice: "<<entry->GetPrefix());
			  continue;
		  }

		  if (entry->FindBestCandidate(0).GetRoutingCost() < select_cost)
		  {
			  tmp = entry;
			  select_cost = entry->FindBestCandidate(0).GetRoutingCost();
			  select_delay = entry->FindBestCandidate(0).GetRealDelay();
			  NS_LOG_LOGIC("try "<<entry->GetPrefix());
		  } else if (entry->FindBestCandidate(0).GetRoutingCost() == select_cost
				  && entry->FindBestCandidate(0).GetRealDelay()<= select_delay)
		  {
			  tmp = entry;
			  NS_LOG_LOGIC("try "<<entry->GetPrefix());
			  select_cost = entry->FindBestCandidate(0).GetRoutingCost();
			  select_delay = entry->FindBestCandidate(0).GetRealDelay();
		  }

//	       cout << *entry <<"; "<<entry->GetPrefix()<< " (this is towards: "<<consumer->GetId()<<" ->";
//	       cout << DynamicCast<const ndn::NetDeviceFace> (entry->FindBestCandidate (0).GetFace ())->GetNetDevice ()->GetChannel ()->GetDevice (1)->GetNode ()->GetId();
//	       //cout << Names::FindName (DynamicCast<const ndn::NetDeviceFace> (entry->FindBestCandidate (0).GetFace ())->GetNetDevice ()->GetChannel ()->GetDevice (1)->GetNode ());
//	       cout<<"; cost="<<entry->FindBestCandidate(0).GetRoutingCost()<< "; delay="<<entry->FindBestCandidate(0).GetRealDelay();
//
//	       cout << ")" << endl;
	     }

	  //cout<<"app="<<this->GetId()<<" change prefix to "<<tmp->GetPrefix()<<" from "<<cur->GetPrefix()<<endl;
	  CDNIPConsumer::m_changeProducer(this, cur, tmp);

	  cur = tmp;

	  Name tn;
	  if (this->m_enableMulticast)
	  {
		  tn = tmp->GetPrefix();
	  } else
	  {
		  string name;
		  name = Names::FindName(this->GetNode());
		  if (name == "")	name = boost::lexical_cast<std::string>(this->GetNode()->GetId());

		  tn = Name(tmp->GetPrefix());
		  tn.Add(name);
	  }

	  NS_LOG_INFO("change interest prefix to "<<tn<< " from " <<this->m_interestName);
	  this->m_interestName = tn;
	  Time now = Simulator::Now ();
}


void
CDNIPConsumer::OnTimeout (uint32_t sequenceNumber) {
	m_timeoutRequest(this, sequenceNumber);
	NS_LOG_FUNCTION(sequenceNumber);
	Time now = Simulator::Now ();

	Time rto = m_rtt->RetransmitTimeout ();

	if (rto + m_lastChangeT <= now )
	{
		NS_LOG_INFO("timeout is not belong to current");
	} else
	{
		CDNIPConsumer::ChangeProducer();
	}
	Consumer::OnTimeout(sequenceNumber);
}


void
CDNIPConsumer::OnNack (const Ptr<const InterestHeader> &interest, Ptr<Packet> packet){

	//uint32_t seq = boost::lexical_cast<uint32_t> (interest->GetName ().GetComponents ().back ());
	Name n = Name(interest->GetName());
	size_t t = 1;
	n = n.cut(t);

	if (n == this->m_interestName)
	{
		CDNIPConsumer::ChangeProducer();
	} else
	{
		NS_LOG_INFO("producer is already changed");
	}
	Consumer::OnNack(interest, packet);
}

void
CDNIPConsumer::ScheduleNextPacket ()
{
  // double mean = 8.0 * m_payloadSize / m_desiredRate.GetBitRate ();
  // std::cout << "next: " << Simulator::Now().ToDouble(Time::S) + mean << "s\n";

  if (m_firstTime)
    {
	 CDNIPConsumer::ChangeProducer();

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
