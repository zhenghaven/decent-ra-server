#include <string>
#include <memory>
#include <iostream>

#include <tclap/CmdLine.h>
#include <boost/filesystem/path.hpp>

#include <DecentApi/CommonApp/Common.h>

#include <DecentApi/CommonApp/Net/TCPConnection.h>
#include <DecentApi/CommonApp/Net/TCPServer.h>
#include <DecentApi/CommonApp/Net/LocalConnection.h>
#include <DecentApi/CommonApp/Net/LocalServer.h>
#include <DecentApi/CommonApp/Net/SmartServer.h>

#include <DecentApi/CommonApp/SGX/EnclaveUtil.h>
#include <DecentApi/CommonApp/SGX/IasConnector.h>

#include <DecentApi/CommonApp/Tools/DiskFile.h>
#include <DecentApi/CommonApp/Tools/FileSystemUtil.h>

#include <DecentApi/CommonApp/Threading/MainThreadAsynWorker.h>

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/Ra/WhiteList/WhiteList.h>

#include <DecentApi/DecentServerApp/SGX/ServerConfigManager.h>
#include <DecentApi/DecentServerApp/DecentServer.h>

using namespace Decent;
using namespace Decent::Tools;
using namespace Decent::Threading;

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
	MainThreadAsynWorker mainThreadWorker;

	std::cout << "================ Decent Server ================" << std::endl;

	//------- Setup Smart Server:
	Net::SmartServer smartServer(mainThreadWorker);

	/*TODO: Add SGX capability test.*/

	TCLAP::CmdLine cmd("Decent Server", ' ', "ver", true);

	TCLAP::ValueArg<std::string> configPathArg("c", "config", "Path to the configuration file.", false, "Config.json", "String");
	cmd.add(configPathArg);

	cmd.parse(argc, argv);

	//------- Read configuration file:
	std::unique_ptr<Sgx::ServerConfigManager> configMgr;
	try
	{
		std::string configJsonStr;
		DiskFile file(configPathArg.getValue(), FileBase::Mode::Read, true);
		configJsonStr.resize(file.GetFileSize());
		file.ReadBlockExactSize(configJsonStr);

		configMgr = std::make_unique<Sgx::ServerConfigManager>(configJsonStr);
	}
	catch (const std::exception& e)
	{
		PRINT_W("Failed to load configuration file. Error Msg: %s", e.what());
		return -1;
	}

	//------- Read Decent Server Configuration:
	uint16_t serverPort = 0;
	uint32_t serverIp = 0;
	std::string localServerName;
	try
	{
		const ConfigItem& decentServerConfig = configMgr->GetItem(Ra::WhiteList::sk_nameDecentServer);

		serverIp = Net::TCPConnection::GetIpAddressFromStr(decentServerConfig.GetAddr());
		serverPort = decentServerConfig.GetPort();
		localServerName = "Local_" + decentServerConfig.GetAddr() + "_" + std::to_string(decentServerConfig.GetPort());
	}
	catch (const std::exception& e)
	{
		PRINT_W("Failed to read Decent Server Configuration. Error Msg: %s", e.what());
		return -1;
	}

	//------- Setup TCP server:
	std::unique_ptr<Net::Server> tcpServer;
	try
	{
		tcpServer = std::make_unique<Net::TCPServer>(serverIp, serverPort);
	}
	catch (const std::exception& e)
	{
		PRINT_W("Failed to start TCP server. Error Message: %s", e.what());
	}

	//------- Setup Local server:
	std::unique_ptr<Net::Server> localServer;
	try
	{
		localServer = std::make_unique<Net::LocalServer>(localServerName);
	}
	catch (const std::exception& e)
	{
		PRINT_W("Failed to start local server. Error Message: %s", e.what());
	}

	if (!tcpServer && !localServer)
	{
		PRINT_W("Failed to start all servers. Program will be terminated!");
		return -1;
	}

	//------- Setup IAS connector:
	std::shared_ptr<Ias::Connector> iasConnector;
	try 
	{
		iasConnector = std::make_shared<Ias::Connector>(configMgr->GetServiceProviderCertPath(),
			configMgr->GetServiceProviderPrvKeyPath());
	}
	catch (const std::exception& e)
	{
		PRINT_W("Failed to open Service Provider certificate or key file. Error Msg: %s", e.what());
		return -1;
	}
	
	//------- Setup enclave:
	std::shared_ptr<RaSgx::DecentServer> enclave;
	try 
	{
		boost::filesystem::path tokenPath = GetKnownFolderPath(KnownFolderType::LocalAppDataEnclave).append(TOKEN_FILENAME);
		enclave = std::make_shared<RaSgx::DecentServer>(
			configMgr->GetSpid(), iasConnector, ENCLAVE_FILENAME, tokenPath);
	}
	catch (const std::exception& e)
	{
		PRINT_W("Failed to start enclave program. Error Msg: %s", e.what());
		return -1;
	}

	//------- Add servers to smart server.
	smartServer.AddServer(tcpServer, enclave, nullptr, 1);
	smartServer.AddServer(localServer, enclave, nullptr, 1);

	//------- keep running until an interrupt signal (Ctrl + C) is received.
	mainThreadWorker.UpdateUntilInterrupt();

	//------- Exit...
	enclave.reset();
	smartServer.Terminate();

	PRINT_I("Exit ...\n");
	return 0;
}
