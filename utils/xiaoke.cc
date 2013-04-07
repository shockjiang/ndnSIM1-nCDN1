/*
 * xiaoke.cc
 *
 *  Created on: Apr 7, 2013
 *      Author: shock
 */

#include "xiaoke.h"

namespace ns3 {
namespace ndn {

xiaoke::xiaoke() {
	// TODO Auto-generated constructor stub

}

xiaoke::~xiaoke() {
	// TODO Auto-generated destructor stub
}

static Name
Translate2Set(Name name)
{
	//Name prefix = Name(name);
	uint32_t seq = atoi(name.GetLastComponent().c_str());
	std::list<boost::reference_wrapper<const std::string> > comps =  name.GetSubComponents(name.size() - 1);
	Name prefix = Name(comps);
	prefix.Add("_set");
	uint32_t t = seq / Interest::SET_MOD;
	uint32_t min = t * Interest::SET_MOD;
	uint32_t max = (t+1) * Interest::SET_MOD -1;
	std::stringstream str;
	str<<"_min"<<min<<"-_max"<<max;
	prefix .Add(str.str());
}



} /* namespace ndn */
} /* namespace ns3 */
