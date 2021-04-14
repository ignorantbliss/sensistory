/*#include "ingestserver.h"

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>

using boost::asio::ip::tcp;
using boost::thread;

IngestService::IngestService()
{
	PortNo = 2223;
	acceptor = 0;
}

IngestService::~IngestService()
{
	if (acceptor != 0) delete acceptor;
	if (thd != 0) delete thd;
}

bool IngestService::Start()
{		
	acceptor = new tcp::acceptor(service, tcp::endpoint(tcp::v4(), PortNo));
	AcceptNext();

	
	thd = new boost::thread(boost::bind(&boost::asio::io_service::run, &service));
	thd->detach();
	//service.run();
	return true;
}

bool IngestService::Stop()
{
	return true;
}

void IngestService::handle_accept(IngestClient::pointer con, const boost::system::error_code& error)
{
	AcceptNext();
}

void IngestService::AcceptNext()
{
	IngestClient::pointer NewClient = IngestClient::Create(service,this);
	acceptor->async_accept(NewClient->GetSocket(), boost::bind(&IngestService::handle_accept, this, NewClient, boost::asio::placeholders::error));
}

tcp::socket& IngestClient::GetSocket()
{
	return m_socket;
}

IngestClient::IngestClient(boost::asio::io_service& io_service) : m_socket(io_service) {}


IngestClient::~IngestClient()
{
}
 
IngestClient::pointer IngestClient::Create(boost::asio::io_context& io_context,IngestService *s)
{
	pointer P = pointer(new IngestClient(io_context));
	P->Srv = s;
	return P;
}*/