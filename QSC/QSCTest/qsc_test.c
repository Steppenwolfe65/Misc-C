/* The GPL version 3 License (GPLv3)
*
* Copyright (c) 2020 Digital Freedom Defence Inc. Inc.
* This file is part of the QSC Cryptographic library
*
* This program is free software : you can redistribute it and / or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
* Written by John G. Underhill
* Contact: develop@vtdev.com */

#include "../QSC/common.h"
#include "../QSC/cpuid.h"
#include "../QSC/csx.h"
#include "../QSC/rcs.h"
#include "../QSC/secrand.h"
#include "../QSC/selftest.h"
#include "benchmark.h"
#include "chacha_test.h"
#include "common.h"
#include "csx_test.h"
#include "dilithium_test.h"
#include "ecdh_test.h"
#include "ecdsa_test.h"
#include "kyber_test.h"
#include "mceliece_test.h"
#include "poly1305_test.h"
#include "rcs_test.h"
#include "secrand_test.h"
#include "sha2_test.h"
#include "sha3_test.h"
#include "sphincsplus_test.h"
#include "testutils.h"

void print_title()
{
	qsctest_print_line("***************************************************");
	qsctest_print_line("* QSC: Quantum Secure Cryptographic library in C  *");
	qsctest_print_line("*                                                 *");
	qsctest_print_line("* Release:   v1.0.0.5a (A5)                       *");
	qsctest_print_line("* License:   GPLv3                                *");
	qsctest_print_line("* Date:      December 07, 2020                    *");
	qsctest_print_line("* Contact:   develop@vtdev.com                    *");
	qsctest_print_line("***************************************************");
	qsctest_print_line("");
}

int32_t main()
{
	qsc_cpu_features features;
	bool valid;
	bool hfeat;

	valid = qsctest_symmetric_selftest_run();

	if (valid == true)
	{
		print_title();

		qsctest_print_line("Passed the internal symmetric primitive self-checks.");

		hfeat = qsc_runtime_features(&features);

		if (hfeat == false)
		{
			qsctest_print_line("The CPU type was not recognized on this system!");
			qsctest_print_line("Some features may be disabled.");
		}

		if (features.has_aesni == true)
		{
			qsctest_print_line("AES-NI is available on this system.");
			qsctest_print_line("The QSC_SYSTEM_AESNI_ENABLED flag has been detected, AES-NI intrinsics are enabled.");
		}
		else
		{
			qsctest_print_line("AES-NI was not detected on this system.");
		}

		if (features.has_avx512 == true)
		{
			qsctest_print_line("AVX-512 intrinsics functions have been detected on this system.");
		}
		else if (features.has_avx2 == true)
		{
			qsctest_print_line("AVX2 intrinsics functions have been detected on this system.");
		}
		else if (features.has_avx == true)
		{
			qsctest_print_line("AVX intrinsics functions have been detected on this system.");
		}
		else
		{
			qsctest_print_line("The AVX intrinsics functions have not been detected or are not enabled.");
			qsctest_print_line("For best performance, enable the maximum available AVX feature set in the project properties (AVX/AVX2/AVX512).");
		}

#if defined(QSC_IS_X86)
		qsctest_print_line("The system is running in X86 mode; for best performance, compile as X64.");
#endif

#if defined(_DEBUG)
		qsctest_print_line("The system is running in Debug mode; for best performance, compile as Release.");
#endif
#if defined(QSC_CSX_AUTHENTICATED)
		qsctest_print_line("The CSX authentication flag was detected.");
		qsctest_print_line("Remove the QSC_CSX_AUTHENTICATED definition from the preprocessor definitions or csx.h to disable CSX authentication.");
#else
		qsctest_print_line("The CSX authentication flag was not detected.");
		qsctest_print_line("Add the QSC_CSX_AUTHENTICATED definition to preprocessor flags to enable the CSX cipher authentication extension.");
#endif

#if defined(QSC_RCS_AUTHENTICATED)
		qsctest_print_line("The RCS authentication flag was detected.");
		qsctest_print_line("Remove the QSC_RCS_AUTHENTICATED definition from the preprocessor definitions or rcs.h to disable RCS authentication.");
#else
		qsctest_print_line("The RCS authentication flag was not detected.");
		qsctest_print_line("Add the QSC_RCS_AUTHENTICATED definition to preprocessor flags to enable the RCS cipher authentication extension.");
#endif

		qsctest_print_line("");
		qsctest_print_line("AVX-512 intrinsics have been fully integrated into this project.");
		qsctest_print_line("On an AVX-512 capable CPU, enable AVX-512 in the project properties for best performance.");
		qsctest_print_line("Enable the maximum available AVX feature set in the project properties (AVX/AVX2/AVX512).");
		qsctest_print_line("");
	}
	else
	{
		qsctest_print_line("Failure! Internal self-checks have errored, aborting tests!");
		valid = false;
	}

	if (valid == true)
	{
		if (qsctest_test_confirm("Press 'Y' then Enter to run Diagnostic Tests, any other key to cancel: ") == true)
		{
			qsctest_print_line("*** Test the ChaCha stream cipher with known answer tests ***");
			qsctest_chacha_run();
			qsctest_print_line("");

			qsctest_print_line("*** Test the CSX-512 stream cipher using stress tests and KAT vectors ***");
			qsctest_csx_run();
			qsctest_print_line("");

			qsctest_print_line("*** Test the RCS stream cipher using stress tests and KAT vectors ***");
			qsctest_rcs_run();
			qsctest_print_line("");

			qsctest_print_line("*** Test the Poly1305 MAC generator with known answer tests ***");
			qsctest_poly1305_run();
			qsctest_print_line("");

			qsctest_print_line("*** Test HKDF, HMAC, and SHA2 implementations using the official KAT vectors ***");
			qsctest_sha2_run();
			qsctest_print_line("");

			qsctest_print_line("*** Test SHAKE, cSHAKE, KMAC, and SHA3 implementations using the official KAT vectors ***");
			qsctest_sha3_run();
			qsctest_print_line("");

			qsctest_print_line("*** Test the Secure Random provider and entropy provider implementations ***");
			qsctest_secrand_run();
			qsctest_print_line("");

			qsctest_print_line("*** Test the ECDH implementation using stress, validity checks, and known answer tests ***");
			qsctest_ecdh_run();
			qsctest_print_line("");

			qsctest_print_line("*** Test the Kyber implementation using stress, validity checks, and known answer tests ***");
			qsctest_kyber_run();
			qsctest_print_line("");

			qsctest_print_line("*** Test the McEliece implementation using stress, validity checks, and known answer tests ***");
			qsctest_mceliece_run();
			qsctest_print_line("");

			qsctest_print_line("*** Test the Dilithium implementation using stress, validity checks, and known answer tests ***");
			qsctest_dilithium_run();
			qsctest_print_line("");

			qsctest_print_line("*** Test the ECDSA implementation using stress, validity checks, and known answer tests ***");
			qsctest_ecdsa_run();
			qsctest_print_line("");

			qsctest_print_line("*** Test the SphincsPlus implementation using stress, validity checks, and known answer tests ***");
			qsctest_sphincsplus_run();
			qsctest_print_safe("");
		}
		else
		{
			qsctest_print_line("");
		}

		if (qsctest_test_confirm("Press 'Y' then Enter to run Symmetric Speed Tests, any other key to cancel: ") == true)
		{
			//qsctest_benchmark_aes_run();
			//qsctest_benchmark_rhx_run();
			qsctest_print_line("Testing symmetric stream ciphers..");
			qsctest_benchmark_chacha_run();
			qsctest_print_line("");
			qsctest_benchmark_csx_run();
			qsctest_print_line("");
			qsctest_benchmark_rcs_run();
			qsctest_print_line("");
			qsctest_print_line("Testing symmetric Keccak primitives..");
			qsctest_benchmark_kpa_run();
			qsctest_print_line("");
			qsctest_benchmark_kmac_run();
			qsctest_print_line("");
			qsctest_benchmark_shake_run();
			qsctest_print_line("");
		}

		qsctest_print_line("Completed! Press any key to close..");
		qsctest_get_wait();
	}
	else
	{
		qsctest_print_line("The test has been cancelled. Press any key to close..");
		qsctest_get_wait();
	}

    return 0;
}
