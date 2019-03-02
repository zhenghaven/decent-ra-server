#include <string>
#include <memory>
#include <iostream>

#include <tclap/CmdLine.h>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/filesystem/path.hpp>
#include <sgx_quote.h>

#include <DecentApi/CommonApp/Common.h>

#include <DecentApi/CommonApp/Net/SmartMessages.h>
#include <DecentApi/CommonApp/Net/TCPConnection.h>
#include <DecentApi/CommonApp/Net/TCPServer.h>
#include <DecentApi/CommonApp/Net/LocalConnection.h>
#include <DecentApi/CommonApp/Net/LocalServer.h>
#include <DecentApi/CommonApp/Net/SmartServer.h>

#include <DecentApi/CommonApp/SGX/EnclaveUtil.h>
#include <DecentApi/CommonApp/SGX/IasConnector.h>

#include <DecentApi/CommonApp/Tools/DiskFile.h>
#include <DecentApi/CommonApp/Tools/ConfigManager.h>

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/Ra/WhiteList/HardCoded.h>

#include <DecentApi/DecentServerApp/DecentServer.h>

using namespace Decent;
using namespace Decent::Tools;

static const sgx_spid_t gsk_sgxSPID = { {
		0xDD,
		0x16,
		0x40,
		0xFE,
		0x0D,
		0x28,
		0xC9,
		0xA8,
		0xB3,
		0x05,
		0xAF,
		0x4D,
		0x4E,
		0x76,
		0x58,
		0xBE,
	} };

static bool GetConfigurationJsonString(const std::string & filePath, std::string & outJsonStr)
{
	try
	{
		DiskFile file(filePath, FileBase::Mode::Read);
		outJsonStr.resize(file.GetFileSize());
		file.ReadBlockExactSize(outJsonStr);
		return true;
	}
	catch (const FileException&)
	{
		return false;
	}
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
	std::cout << "================ Decent Server ================" << std::endl;

	TCLAP::CmdLine cmd("Decent Server", ' ', "ver", true);

	TCLAP::ValueArg<std::string> configPathArg("c", "config", "Path to the configuration file.", false, "Config.json", "String");
	cmd.add(configPathArg);

	cmd.parse(argc, argv);

	std::string configJsonStr;
	if (!GetConfigurationJsonString(configPathArg.getValue(), configJsonStr))
	{
		PRINT_W("Failed to load configuration file.");
		return -1;
	}

	ConfigManager configManager(configJsonStr);

	const ConfigItem& decentServerConfig = configManager.GetItem(Ra::WhiteList::sk_nameDecentServer);

	/*TODO: Add SGX capability test.*/
	/*TODO: Move SPID, certificate path, key path to configuration file.*/
	uint32_t serverIp = boost::asio::ip::address_v4::from_string(decentServerConfig.GetAddr()).to_uint();
	const std::string localServerName = "Local_" + decentServerConfig.GetAddr() + "_" + std::to_string(decentServerConfig.GetPort());

	std::shared_ptr<Ias::Connector> iasConnector = std::make_shared<Ias::Connector>();
	Net::SmartServer smartServer;

	std::shared_ptr<RaSgx::DecentServer> enclave;
	try 
	{
		enclave = std::make_shared<RaSgx::DecentServer>(
			gsk_sgxSPID, iasConnector, ENCLAVE_FILENAME, KnownFolderType::LocalAppDataEnclave, TOKEN_FILENAME);
	}
	catch (const std::exception& e)
	{
		PRINT_W("Failed to start enclave program! Error Msg:\n%s", e.what());
		return -1;
	}
	

	std::unique_ptr<Net::Server> tcpServer;
	std::unique_ptr<Net::Server> localServer;
	try
	{
		tcpServer = std::make_unique<Net::TCPServer>(serverIp, decentServerConfig.GetPort());
	}
	catch (const std::exception& e)
	{
		PRINT_W("Failed to start TCP server! Error Message:\n%s", e.what());
	}

	try
	{
		localServer = std::make_unique<Net::LocalServer>(localServerName);
	}
	catch (const std::exception& e)
	{
		PRINT_W("Failed to start local server! Error Message:\n%s", e.what());
	}

	if (!tcpServer && !localServer)
	{
		PRINT_W("Failed to start all servers. Program will be terminated!");
		return -1;
	}

	smartServer.AddServer(tcpServer, enclave);
	smartServer.AddServer(localServer, enclave);
	smartServer.RunUtilUserTerminate();

	PRINT_I("Exit ...\n");
	return 0;
}
