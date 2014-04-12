#!/bin/sh

GTESTFILTER='--gtest_filter=libretroshare_services.*'
#GTESTFILTER='--gtest_filter=libretroshare_services.GXS_nxs_basic*'
#GTESTOUTPUT='--gtest_output "xml:test_results.xml"'

echo ./unittests $GTESTOUTPUT $GTESTFILTER
./unittests $GTESTOUTPUT $GTESTFILTER
