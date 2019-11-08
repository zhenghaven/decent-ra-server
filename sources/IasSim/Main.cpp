#include <string>
#include <memory>
#include <iostream>

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/Net/ConnectionPoolBase.h>

#include <DecentApi/CommonApp/Net/TCPConnection.h>
#include <DecentApi/CommonApp/Net/TCPServer.h>
#include <DecentApi/CommonApp/Net/SmartServer.h>
#include <DecentApi/CommonApp/Threading/MainThreadAsynWorker.h>

#include "IasSimApp.h"

#define DECENT_IAS_SIM_VERSION_MAIN 0
#define DECENT_IAS_SIM_VERSION_SUB  5

#define QUOTE(str) #str
#define EXPAND_AND_QUOTE(str) QUOTE(str)
#define DECENT_IAS_SIM_VERSION_STR EXPAND_AND_QUOTE(DECENT_IAS_SIM_VERSION_MAIN) "." EXPAND_AND_QUOTE(DECENT_IAS_SIM_VERSION_SUB)
#define DECENT_IAS_SIM_VERSION_STR_W_PREFIX "Ver" DECENT_IAS_SIM_VERSION_STR

using namespace Decent;
using namespace Decent::IasSim;
using namespace Decent::Net;
using namespace Decent::Tools;
using namespace Decent::Threading;

static std::shared_ptr<ConnectionPoolBase> GetIasSimConnectionPool()
{
	static std::shared_ptr<ConnectionPoolBase> tcpConnectionPool = std::make_shared<ConnectionPoolBase>(1000);
	return tcpConnectionPool;
}

/**
 * \brief	Main entry-point for this application
 *
 * \param	argc	The number of command-line arguments provided.
 * \param	argv	An array of command-line argument strings.
 *
 * \return	Exit-code for the process - 0 for success, else an error code.
 */
int main(int argc, char ** argv)
{
	//------- Construct main thread worker at very first:
	std::shared_ptr<MainThreadAsynWorker> mainThreadWorker = std::make_shared<MainThreadAsynWorker>();

	std::cout << "================ IAS Simulator " DECENT_IAS_SIM_VERSION_STR_W_PREFIX " ================" << std::endl;

	//------- Setup Smart Server:
	Net::SmartServer smartServer(mainThreadWorker);

	//------- Setup TCP server:
	std::unique_ptr<Net::Server> tcpServer;
	try
	{
		tcpServer = std::make_unique<Net::TCPServer>("127.0.0.1", 57720);
	}
	catch (const std::exception& e)
	{
		PRINT_W("Failed to start TCP server. Error Message: %s", e.what());
	}

	if (!tcpServer)
	{
		PRINT_W("Failed to start all servers. Program will be terminated!");
		return -1;
	}

	//------- Setup simulator app:
	std::shared_ptr<IasSimApp> iasSimApp;
	try
	{
		iasSimApp = std::make_shared<IasSimApp>();
	}
	catch (const std::exception& e)
	{
		PRINT_W("Failed to start enclave program. Error Msg: %s", e.what());
		return -1;
	}

	//------- Add servers to smart server.
	smartServer.AddServer(tcpServer, iasSimApp, GetIasSimConnectionPool(), 50, 1000);

	//------- keep running until an interrupt signal (Ctrl + C) is received.
	mainThreadWorker->UpdateUntilInterrupt();

	//------- Exit...
	smartServer.Terminate();

	PRINT_I("Exit ...\n");
	return 0;
}
