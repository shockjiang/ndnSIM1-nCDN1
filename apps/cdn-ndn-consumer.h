/*
 * cdn-ndn-consumer.h
 *
 *  Created on: Jun 27, 2013
 *      Author: shock
 */

#ifndef CDN_NDN_CONSUMER_H_
#define CDN_NDN_CONSUMER_H_

#include "ndn-consumer-zipf-mandelbrot.h"
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

#include "ndn-consumer-zipf-mandelbrot.h"

#include <vector>
using std::vector;

namespace ns3 {
namespace ndn {

class CDNConsumer: public ns3::ndn::ConsumerZipfMandelbrot {
public:
public:
  static TypeId GetTypeId ();

  /**
   * \brief Default constructor
   * Sets up randomized Number Generator (RNG)
   * Note: m_seq of its parent class ConsumerCbr here is used to record the interest number
   */
  CDNConsumer ();
  virtual ~CDNConsumer ();


  virtual void SendPacket();

  virtual void
  OnNack (const Ptr<const InterestHeader> &interest, Ptr<Packet> packet);


  /**
   * @brief Timeout event
   * @param sequenceNumber time outed sequence number
   */
  virtual void
  OnTimeout (uint32_t sequenceNumber);



  virtual void
  OnContentObject (const Ptr<const ContentObjectHeader> &contentObject,
                   Ptr<Packet> payload);

  //void   addDataSources(std::string prefix);

  virtual void
  ScheduleNextPacket ();

//  void
//  AddProducers(NameComponents source);
//
//  void
//  SetProducers(std::string prefixes);
//
//
//  vector<NameComponents>::size_type
//  ChangeProducer();
//
//  std::string
//  GetProducers() const;

//  vector<NameComponents>     m_producers;
//  NameComponents m_currentProducer;
//  uint32_t m_sentN;

  TracedCallback<Ptr<App> /*app */, uint32_t /* seq */> m_timeoutRequest;
};

} /* namespace ndn */
} /* namespace ns3 */
#endif /* CDN_NDN_CONSUMER_H_ */
