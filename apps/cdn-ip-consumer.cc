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

   .AddAttribute ("Producers", "Data Producers",
						  StringValue ("0"),
						  //MakeDoubleAccessor (&CDNIPApp::m_SeqType),
						  //MakeUintegerChecker<uint32_t> ())
						  //MakeTimeAccessor (&Consumer::GetRetxTimer, &Consumer::SetRetxTimer),
						  //MakeNameComponentsAccessor (&CDNIPApp::m_interestName),

						  MakeStringAccessor (&CDNIPConsumer::GetProducers, &CDNIPConsumer::SetProducers),
						  MakeStringChecker ())
//						  MakeObjectVectorAccessor (&CDNIPApp::m_Producers),
//						  MakeObjectVectorChecker ())
						  //MakeNameComponentsAccessor (&CDNIPApp::GetDataSources, &CDNIPApp::SetDataSources),
						  //MakeNameComponentsChecker ())

    ;

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


std::string
CDNIPConsumer::GetProducers() const
{

	stringstream str("");

	for (vector<NameComponents>::size_type ix=0; ix !=m_producers.size(); ++ix){
		NameComponents name = m_producers[ix];
		name.Print(str);
	}
    NS_LOG_LOGIC("Producers are="<<str.str());
	return str.str();
}

void
CDNIPConsumer::AddProducers(NameComponents producer){
	//NameComponents source = NameComponents(prefix);

		vector<NameComponents>::size_type ix = 0;
		for (; ix <m_producers.size(); ++ix) {
			if (m_producers[ix] == producer) {
				break;
			}
		}
		if (ix == m_producers.size()) {
			m_producers.push_back(producer);
			m_interestName = producer;
            NS_LOG_LOGIC("add Data provider "<<producer);
		}

}

void
CDNIPConsumer::SetProducers(std::string prefixes){
	//prefixes.
	std::string s = prefixes;
	char seperator = ';';

	std::string::size_type prev_pos = 0, pos = 0;
	NameComponents name;
	while((pos = s.find(seperator, pos)) != std::string::npos)
	{
		std::string substring( s.substr(prev_pos, pos-prev_pos) );

		 name = NameComponents(substring);

		AddProducers(name);
		prev_pos = ++pos;
	}

	name = NameComponents(s.substr(prev_pos, pos-prev_pos)); // Last word
	AddProducers(name);
}


vector<NameComponents>::size_type
CDNIPConsumer::ChangeProducer(){
	//vector<NameComponents>::size_type ix = 0;
	vector<NameComponents>::size_type cur = 0;
//	if (sent.empty() || received.empty()) {
//		for (vector<NameComponents>::size_type i=0; i<m_producers.size(); i++) {
//			sent.push_back(0);
//			received.push_back(0);
//		}
//	}
	if (m_interestName == m_producers[cur]) {
		NS_LOG_INFO("= Hold Data Producer. datasource.size= "<<m_producers.size());
	}else {
		NS_LOG_INFO("! Change Data Producer to "<<cur);
		m_interestName = m_producers[cur];
	}

	return cur;
}


} /* namespace ndn */
} /* namespace ns3 */
