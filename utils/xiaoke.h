/*
 * xiaoke.h
 *
 *  Created on: Apr 7, 2013
 *      Author: shock
 */

#ifndef XIAOKE_H_
#define XIAOKE_H_
#include "ns3/simple-ref-count.h"
#include "ns3/attribute.h"
#include "ns3/attribute-helper.h"

#include <string>
#include <algorithm>
#include <list>
#include "ns3/object.h"
#include "ns3/buffer.h"
#include "ns3/ndn-name.h"
#include "ns3/ndn-interest.h"

#include <boost/ref.hpp>

namespace ns3 {
namespace ndn {

class xiaoke {
public:
	xiaoke();
	virtual ~xiaoke();

	static Name
	Translate2Set(Name name);
};

} /* namespace ndn */
} /* namespace ns3 */
#endif /* XIAOKE_H_ */
