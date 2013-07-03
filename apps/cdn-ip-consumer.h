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

#include "cdn-ndn-consumer.h"

#include <vector>
using std::vector;


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
  AddProducers(NameComponents source);

  void
  SetProducers(std::string prefixes);


  vector<NameComponents>::size_type
  ChangeProducer();

  std::string
  GetProducers() const;


  vector<NameComponents>::size_type pickIndex(NameComponents name);


public:
  vector<NameComponents>     m_producers;        ///< \brief NDN Name of the Interest (use NameComponents)
  vector<uint32_t>     m_costs;        ///< \brief NDN Name of the Interest (use NameComponents)
  //Ptr<NameComponents> m_curProducer; m_interestName from Consumer

};

} /* namespace ndn */
} /* namespace ns3 */
#endif /* CDN_IP_APP_H_ */
