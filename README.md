# CAENELS AMC PICO8 EPICS support module

_Author: Hinko Kocevar <hinko.kocevar@esss.se>_
_Updated: March 29 2017_


This modules provides support for CAENELS AMC PICO8 picoamper meter consisting
of DAMC-FMC25 and two FMC-pico-1M4 modules.

Web site: http://www.caenels.com/products/amc-pico-8

Sources:

* <https://bitbucket.org/europeanspallationsource/m-epics-pico8>
* <https://github.com/CAENels/amc-pico8-driver>

Module uses areaDetector 2.5 features such as NDPluginFFT and NDPluginTimeSeries.
It is based on an ADCSimDetector example provided by ADExample.

It also depends on a Linux kernel driver and user space header provided by
CAENELS (from github https://github.com/CAENels/amc-pico8-driver).

An IOC is also provided that utilizes this module along with other areaDetector
plugins that might be useful when working with digitizers (statistics, ROI, ..).

Along with that Pico8 OPI is integrated into basic AD provided OPI. For demo
purposes complete AD OPI are available for quick deployment (with couple of
tweaks and bug fixes).


TODO:

* integrate with ESS EPICS timing receiver module for triggering purposes
* make EEE compatible IOC (once ADCore 2.5 is in EEE)


See also:

* <https://github.com/areaDetector/areaDetector>
* <https://github.com/areaDetector/ADCore>
* <https://github.com/areaDetector/ADExample>
* <http://cars.uchicago.edu/software/epics/ADCSimDetectorDoc.html>
* <http://www.caenels.com/products/amc-pico-8/>
* <http://www.caenels.com/products/fmc-pico-1m4/>
* <https://github.com/CAENels/amc-pico8-driver>