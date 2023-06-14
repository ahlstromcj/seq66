:: @echo off
:: **************************************************************************
:: Seq66 Windows Win Deployment Batch File
:: --------------------------------------------------------------------------
::
:: \file        windeploybruteforce.bat
:: \library     Seq66 for Windows
:: \author      Chris Ahlstrom
:: \date        2023-06-06
:: \update      2023-06-12
:: \license     $XPC_SUITE_GPL_LICENSE$
::
::      This script sets up and creates a release build of Seq66 for
:: ....
:: The only parameter for this batch file is the source location of the Qt
:: files. The base location is something like:
::
:: C:\Qt\Qt5.12.9\5.12.9\mingw73_64\ or 
:: C:\Qt\Qt5.12.9\5.12.9\mingw73_32\
::
:: Qt complicates it by getting a few files from
::
:: C:\Qt\Qt5.12.9\Tools\mingw730_64\ or
:: C:\Qt\Qt5.12.9\Tools\mingw730_32\
::
:: For "simplicity", we define both the Qt version and the bitness path in
:: addition to the release destination.
::
:: %1 = 5.12.9
:: %2 = mingw73_32
:: %2 = 32 or 64
:: %3 = Seq66qt\release

:: mkdir %3\bin

mkdir %3\iconengines
mkdir %3\imageformats
mkdir %3\styles
mkdir %3\translations

copy C:\Qt\Qt%1\%1\mingw73_%2\bin\Qt5Core.dll %3
copy C:\Qt\Qt%1\%1\mingw73_%2\bin\Qt5Gui.dll %3
copy C:\Qt\Qt%1\%1\mingw73_%2\bin\Qt5Svg.dll %3
copy C:\Qt\Qt%1\%1\mingw73_%2\bin\Qt5Widgets.dll %3
copy C:\Qt\Qt%1\%1\mingw73_%2\bin\libGLESV2.dll %3
copy C:\Qt\Qt%1\%1\mingw73_%2\bin\libEGL.dll %3
copy C:\Qt\Qt%1\%1\mingw73_%2\bin\D3Dcompiler_47.dll %3
copy C:\Qt\Qt%1\%1\mingw73_%2\bin\opengl32sw.dll %3
copy C:\Qt\Qt%1\Tools\mingw730_%2\bin\libgcc_s_seh-1.dll %3
copy C:\Qt\Qt%1\Tools\mingw730_%2\bin\libgcc_s_dw2-1.dll %3
copy C:\Qt\Qt%1\Tools\mingw730_%2\bin\libstdc++-6.dll %3
copy C:\Qt\Qt%1\Tools\mingw730_%2\bin\libwinpthread-1.dll %3
copy C:\Qt\Qt%1\%1\mingw73_%2\plugins\iconengines\qsvgicon.dll %3\iconengines
copy C:\Qt\Qt%1\%1\mingw73_%2\plugins\imageformats\qgif.dll %3\imageformats
copy C:\Qt\Qt%1\%1\mingw73_%2\plugins\imageformats\qicns.dll %3\imageformats
copy C:\Qt\Qt%1\%1\mingw73_%2\plugins\imageformats\qico.dll %3\imageformats
copy C:\Qt\Qt%1\%1\mingw73_%2\plugins\imageformats\qjpeg.dll %3\imageformats
copy C:\Qt\Qt%1\%1\mingw73_%2\plugins\imageformats\qsvg.dll %3\imageformats
copy C:\Qt\Qt%1\%1\mingw73_%2\plugins\imageformats\qtga.dll %3\imageformats
copy C:\Qt\Qt%1\%1\mingw73_%2\plugins\imageformats\qtiff.dll %3\imageformats
copy C:\Qt\Qt%1\%1\mingw73_%2\plugins\imageformats\qwbmp.dll %3\imageformats
copy C:\Qt\Qt%1\%1\mingw73_%2\plugins\imageformats\qwebp.dll %3\imageformats
copy C:\Qt\Qt%1\%1\mingw73_%2\plugins\platforms\qwindows.dll %3\imageformats
copy C:\Qt\Qt%1\%1\mingw73_%2\plugins\styles\qwindowsvistastyle.dll %3\styles
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qt_ar.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qtbase_ar.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qt_bg.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qtbase_bg.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qt_ca.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qtbase_ca.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qt_cs.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qtbase_cs.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qt_da.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qtbase_da.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qt_de.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qtbase_de.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qt_en.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qtbase_en.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qt_es.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qtbase_es.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qt_fi.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qtbase_fi.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qt_fr.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qtbase_fr.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qt_gd.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qtbase_gd.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qt_he.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qtbase_he.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qt_hu.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qtbase_hu.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qt_it.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qtbase_it.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qt_ja.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qtbase_ja.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qt_ko.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qtbase_ko.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qt_lv.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qtbase_lv.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qt_pl.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qtbase_pl.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qt_ru.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qtbase_ru.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qt_sk.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qtbase_sk.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qt_uk.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qtbase_uk.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qt_zh_TW.qm %3\translations
copy C:\Qt\Qt%1\%1\mingw73_%2\translations\qtbase_zh_TW.qm %3\translations

:: vim: ts=4 sw=4 ft=dosbatch fileformat=dos
