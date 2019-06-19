#include <iostream>

#include <sgx_capable.h>

int main(int argc, char** argv)
{
	sgx_status_t retVal;
	sgx_device_status_t sgxDeviceStatus;
	int sgxCapable;

	retVal = sgx_is_capable(&sgxCapable);
	if (retVal != SGX_SUCCESS)
	{
		switch (retVal)
		{
#ifdef SGX_ERROR_EFI_NOT_SUPPORTED
		case SGX_ERROR_EFI_NOT_SUPPORTED:
			std::cout << "Cannot query EFI for Intel(R) SGX support." << endl;
			break;
#endif
		case SGX_ERROR_NO_PRIVILEGE:
			std::cout << "This program requires Administrator privileges." << std::endl;
			break;
		case SGX_ERROR_UNEXPECTED:
		case SGX_ERROR_INVALID_PARAMETER:
			std::cout << "An unexpected error occurred." << std::endl;
		}

		return -1; //Error Exit
	}

	if (sgxCapable == 0)
	{
		std::cout << "This system does not support Intel(R) SGX." << std::endl;

		return -1; //Error Exit
	}

	// Try to enable SGX
	std::cout << "This system does support Intel(R) SGX." << std::endl;
	std::cout << "Checking current status of SGX..." << std::endl;

	retVal = sgx_cap_enable_device(&sgxDeviceStatus);
	if (retVal != SGX_SUCCESS)
	{
		switch (retVal)
		{
		case SGX_ERROR_NO_PRIVILEGE:
			std::cout << "This program requires Administrator privileges.";
			break;
#ifdef SGX_ERROR_EFI_NOT_SUPPORTED
		case SGX_ERROR_EFI_NOT_SUPPORTED:
			std::cout << "This system does not support the EFI interface. Cannot enable SGX via software.";
			*sgx_support |= ST_SGX_BIOS_ENABLE_REQUIRED;
			break;
#endif
#ifdef WIN32
		case SGX_ERROR_HYPERV_ENABLED:
			std::cout << "This version of Windows 10 is incompatible with Hyper-V. Disable Hyper-V and try again.";
			break;
#endif
#ifdef SGX_ERROR_VMM_INCOMPATIBLE
		case SGX_ERROR_VMM_INCOMPATIBLE:
			std::cout << "The virtual machine monitor is incompatible with Intel(R)SGX.";
			break;
#endif
		case SGX_ERROR_INVALID_PARAMETER:
		case SGX_ERROR_UNEXPECTED:
			std::cout << "An unexpected error occurred.";
			break;
		}

		return -1; //Error Exit
	}

	if (sgxDeviceStatus == SGX_ENABLED)
	{
		std::cout << "SGX is already enabled. There is nothing need to be done." << std::endl;
		return 0; //Successful exit.
	}

	// Perform the software opt-in/enable.
	std::cout << "Trying to enable SGX..." << std::endl;

	switch (sgxDeviceStatus)
	{
	case SGX_DISABLED_REBOOT_REQUIRED:
		std::cout << "A reboot is required to enable the SGX." << std::endl;
		break;
	case SGX_DISABLED:
		std::cout << "SGX is not enabled on this platform. More details are unavailable." << std::endl;
		break;
	case SGX_DISABLED_MANUAL_ENABLE:
		std::cout << "SGX is disabled, but can be enabled manually in the BIOS setup." << std::endl;
		break;
	case SGX_DISABLED_LEGACY_OS:
		std::cout << "SGX is disabled and a Software Control Interface is not available to enable it." << std::endl;
		break;
	case SGX_DISABLED_SCI_AVAILABLE:
		std::cout << "SGX is disabled, but a Software Control Interface is available to enable it." << std::endl;
		break;
	case SGX_DISABLED_HYPERV_ENABLED:
		std::cout << "Detected an unsupported version of Windows* 10 with Hyper-V enabled." << std::endl;
		break;
	case SGX_DISABLED_UNSUPPORTED_CPU:
		std::cout << "SGX is not supported by this CPU." << std::endl;
		break;
	default:
		break;
	}

	return -1; //Error Exit
}
