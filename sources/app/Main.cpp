#include <cstdio>
#include <cstring>

#include <string>
#include <memory>
#include <iostream>

#include <tclap/CmdLine.h>
#include <boost/asio/ip/address_v4.hpp>
#include <json/json.h>
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

#include <DecentApi/DecentServerApp/DecentServer.h>

using namespace Decent;
using namespace Decent::Tools;

static sgx_spid_t g_sgxSPID = { {
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
	TCLAP::CmdLine cmd("Decent Remote Attestation", ' ', "ver", true);

#ifndef DEBUG
	TCLAP::ValueArg<uint16_t>  argServerPort("p", "port", "Port number for on-coming local connection.", true, 0, "[0-65535]");
	cmd.add(argServerPort);
#else
	TCLAP::ValueArg<int> testOpt("t", "test-opt", "Test Option Number", false, 0, "A single digit number.");
	cmd.add(testOpt);
#endif

	cmd.parse(argc, argv);

#ifndef DEBUG
	std::string serverAddr = "127.0.0.1";
	uint16_t serverPort = argServerPort.getValue();
	std::string localAddr = "DecentServerLocal";
#else
	uint16_t rootServerPort = 57755U;

	std::string serverAddr = "127.0.0.1";
	uint16_t serverPort = rootServerPort + testOpt.getValue();
	std::string localAddr = "DecentServerLocal";
#endif

	uint32_t serverIp = boost::asio::ip::address_v4::from_string(serverAddr).to_uint();


	sgx_device_status_t deviceStatusRes;
	sgx_status_t deviceStatusResErr = Sgx::GetDeviceStatus(deviceStatusRes);
	//ASSERT(deviceStatusResErr == SGX_SUCCESS, "%s\n", Sgx::GetErrorMessage(deviceStatusResErr).c_str());

	std::shared_ptr<Ias::Connector> iasConnector = std::make_shared<Ias::Connector>();
	Net::SmartServer smartServer;

	std::cout << "================ Decent Server ================" << std::endl;

	std::shared_ptr<RaSgx::DecentServer> enclave(
		std::make_shared<RaSgx::DecentServer>(
			g_sgxSPID, iasConnector, ENCLAVE_FILENAME, KnownFolderType::LocalAppDataEnclave, TOKEN_FILENAME));

	//if (!isRootServer)
	//{
	//	std::unique_ptr<Connection> connection = std::make_unique<TCPConnection>(rootServerIp, rootServerPort);
	//	DecentRASession::SendHandshakeMessage(*connection, *enclave);
	//	Json::Value jsonRoot;
	//	connection->ReceivePack(jsonRoot);
	//	enclave->ProcessSmartMessage(Messages::ParseCat(jsonRoot), jsonRoot, *connection);
	//}

	std::unique_ptr<Net::Server> server(std::make_unique<Net::TCPServer>(serverIp, serverPort));
	std::unique_ptr<Net::Server> localServer(std::make_unique<Net::LocalServer>(localAddr + std::to_string(serverPort)));

	smartServer.AddServer(server, enclave);
	smartServer.AddServer(localServer, enclave);
	smartServer.RunUtilUserTerminate();

	printf("Exit ...\n");
	return 0;
}
