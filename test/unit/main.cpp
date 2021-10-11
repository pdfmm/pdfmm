/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <iostream>
#include <pdfmm/pdfmm.h>

void show_help()
{
    std::cout << "podofo-test" << std::endl << std::endl;
    std::cout << "Supported commandline switches:" << std::endl;
    std::cout << "\t --help\t So this help message." << std::endl;
    std::cout << "\t --selftest\t Output in compiler compatible format." << std::endl;
    std::cout << "\t --test [name]\t Run only the test case [name]." << std::endl;
    std::cout << std::endl;
}

int main(int argc, char* argv[])
{
    // Get the top level suite from the registry
    CppUnit::Test* suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();

    // Adds the test to the list of test to run
    CppUnit::TextUi::TestRunner runner;
    runner.addTest(suite);

    // check some commandline arguments
    std::string sTestName = "";
    bool bSelfTest = false;
    if (argc > 1)
    {
        for (int i = 1; i < argc; i++)
        {
            std::string argument(argv[i]);

            if (argument == "--help" || argument == "-help")
            {
                show_help();
                return 0;
            }
            else if (argument == "--selftest" || argument == "-selftest")
            {
                bSelfTest = true;
            }
            else if ((argument == "--test" || argument == "-test") && i + 1 < argc)
            {
                if (i == argc - 1)
                {
                    show_help();
                    return 0;
                }

                i++;
                sTestName = argv[i];
            }
        }
    }

    if (bSelfTest)
    {
        // Change the default outputter to a compiler error format outputter
        runner.setOutputter(new CppUnit::CompilerOutputter(&runner.result(),
            std::cerr));
    }
    else
    {
        // Change the default outputter to a xml format outputter
        // The test runner owns the new outputter.
        CppUnit::XmlOutputter* xmlOutputter = new
            CppUnit::XmlOutputter(&runner.result(), std::cerr);
        runner.setOutputter(xmlOutputter);
    }

    // Enable PoDoFo debugging and logging
    PoDoFo::PdfError::EnableLogging(true);
    PoDoFo::PdfError::EnableDebug(true);

    // Run the tests.
    bool wasSucessful = runner.run(sTestName);

    // Return error code 1 if the one of test failed.
    return wasSucessful ? 0 : 1;
}
