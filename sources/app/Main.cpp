#include <string>
#include <memory>
#include <iostream>

#include <tclap/CmdLine.h>
#include <boost/asio/ip/address_v4.hpp>
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

#ifndef DEBUG
	TCLAP::ValueArg<uint16_t>  argServerPort("p", "port", "Port number for on-coming local connection.", true, 0, "[0-65535]");
	cmd.add(argServerPort);
#else
	TCLAP::ValueArg<int> testOpt("t", "test-opt", "Test Option Number", false, 0, "A single digit number.");
	cmd.add(testOpt);
#endif

	cmd.parse(argc, argv);

	std::string serverAddr = "127.0.0.1";
	std::string localAddr = "DecentServerLocal";
#ifndef DEBUG
	uint16_t serverPort = argServerPort.getValue();
#else
	uint16_t rootServerPort = 57755U;
	uint16_t serverPort = rootServerPort + testOpt.getValue();
#endif
	/*TODO: Add SGX capability test.*/
	/*TODO: Move SPID, certificate path, key path to configuration file.*/
	uint32_t serverIp = boost::asio::ip::address_v4::from_string(serverAddr).to_uint();

	std::shared_ptr<Ias::Connector> iasConnector = std::make_shared<Ias::Connector>();
	Net::SmartServer smartServer;

	std::shared_ptr<RaSgx::DecentServer> enclave(
		std::make_shared<RaSgx::DecentServer>(
			gsk_sgxSPID, iasConnector, ENCLAVE_FILENAME, KnownFolderType::LocalAppDataEnclave, TOKEN_FILENAME));

	std::unique_ptr<Net::Server> server(std::make_unique<Net::TCPServer>(serverIp, serverPort));
	std::unique_ptr<Net::Server> localServer(std::make_unique<Net::LocalServer>(localAddr + std::to_string(serverPort)));

	smartServer.AddServer(server, enclave);
	smartServer.AddServer(localServer, enclave);
	smartServer.RunUtilUserTerminate();

	printf("Exit ...\n");
	return 0;
}
