/*
 * cdn-ip-app.h
 *
 *  Created on: Mar 11, 2013
 *      Author: shock
 */

#ifndef CDN_IP_APP_H_
#define CDN_IP_APP_H_
#include "ndn-consumer.h"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ndn-consumer-cbr.h"
#include "ns3/random-variable.h"
#include "ns3/ndn-fib.h"
//#include "ns3/ndn-fib-impl.h"
#include "cdn-ndn-consumer.h"
#include <boost/lexical_cast.hpp>


#include <vector>
using std::vector;

//using ndn::fib


namespace ns3 {
namespace ndn {

class CDNIPConsumer: public CDNConsumer
{
public:
  static TypeId GetTypeId ();

  /**
   * \brief Default constructor
   * Sets up randomized Number Generator (RNG)
   * Note: m_seq of its parent class ConsumerCbr here is used to record the interest number
   */
  CDNIPConsumer ();

  void
  ChangeProducer();

  virtual void
  ScheduleNextPacket ();


  virtual void
  OnNack (const Ptr<const InterestHeader> &interest, Ptr<Packet> packet);


  /**
   * @brief Timeout event
   * @param sequenceNumber time outed sequence number
   */
  virtual void
  OnTimeout (uint32_t sequenceNumber);

public:
  bool m_enableMulticast;
 Ptr<ndn::fib::Entry> cur;
 Time m_lastChangeT;
  TracedCallback<Ptr<App> /*app */, Ptr<ndn::fib::Entry> /*entryold*/, Ptr<ndn::fib::Entry> /*entrynew */> m_changeProducer;

};

} /* namespace ndn */
} /* namespace ns3 */
#endif /* CDN_IP_APP_H_ */
