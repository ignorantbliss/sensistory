#pragma once

/*#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class IngestClient : public boost::enable_shared_from_this<IngestClient>
{
public:
	typedef boost::shared_ptr<IngestClient> pointer;

	static pointer Create(boost::asio::io_context& io_context, class IngestService *s);
	
	IngestClient(boost::asio::io_service& io_service);
	~IngestClient();
	tcp::socket& GetSocket();

private:	
	tcp::socket m_socket;	
	class IngestService* Srv;
};

class IngestService
{
public:	
	IngestService();
	~IngestService();	

	bool Start();
	bool Stop();		
	
private:	

	void AcceptNext();

	boost::thread *thd;
	short PortNo;
	boost::asio::io_service service;
	boost::asio::io_context context;
	tcp::acceptor* acceptor;	
	void handle_accept(IngestClient::pointer con, const boost::system::error_code& error);
};*/