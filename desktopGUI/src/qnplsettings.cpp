#include "qnplsettings.h"

#include <QApplication>
#include <QDir>
#include <util.h>

QnplSettings::QnplSettings()
    : QSettings(QSettings::IniFormat, QSettings::UserScope,
                "telemidia", "gingagui")
{
    if (value("autoplay").toString() ==  ""){
        setValue("autoplay", true);
    }

    if (value("screensize").toString() == ""){
        setValue("screensize", "640x480");
    }

    if (value("location").toString() == ""){
#ifdef Q_OS_WIN
        setValue("location", QApplication::applicationDirPath()+"/"+"ginga.exe");
#else
        setValue("location", "/usr/bin/ginga");
#endif
    }

    if (value("device_port").toString() == ""){
        setValue("device_port", 22222);
    }

    if (value("passive_running").toString() == ""){
        setValue("passive_running", false);
    }

    if (value("run_as").toString() == "")
    {
      setValue("run_as", "base");
    }

    if (value("enablelog").toString() == "")
    {
      setValue("enablelog", false);
    }

    if (value("lang").toString() == ""){
        setValue("lang", "en");
    }

    //if (value("parameters").toString() == "" || value("version").toInt() < 106)
    //{
        setValue(Util::V_PARAMETERS, Util::defaultParameters());
    //}

#ifdef Q_OS_WIN
    if (value("gingaconfig_file").toString() == "" || value("version").toInt() < 105){
        setValue("gingaconfig_file", QDir::toNativeSeparators(QDir::homePath())+"\\AppData\\Roaming\\telemidia\\ginga\\contexts.ini");
    }
#else
    if (value("gingaconfig_file").toString() == ""){
        setValue("gingaconfig_file", "");
    }
#endif

   setValue("version", 107);
}

QnplSettings::~QnplSettings()
{

}
