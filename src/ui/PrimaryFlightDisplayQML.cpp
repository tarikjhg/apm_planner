/*===================================================================
APM_PLANNER Open Source Ground Control Station

(c) 2014 Bill Bonney <billbonney@communistech.com>

This file is part of the APM_PLANNER project

    APM_PLANNER is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    APM_PLANNER is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with APM_PLANNER. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

#include "QsLog.h"
#include "PrimaryFlightDisplayQML.h"
#include "ui_PrimaryFlightDisplayQML.h"

#include "configuration.h"
#include "MainWindow.h"
#include <QVBoxLayout>
#include <QQmlContext>
#include <QQuickItem>
#include <QQmlEngine>
#define ToRad(x) (x*0.01745329252)      // *pi/180
#define ToDeg(x) (x*57.2957795131)      // *180/pi

PrimaryFlightDisplayQML::PrimaryFlightDisplayQML(QWidget *parent) :
    QWidget(parent),
//    ui(new Ui::PrimaryFlightDisplayQML),
    m_declarativeView(NULL)
{
//    ui->setupUi(this);
    QUrl url = QUrl::fromLocalFile(QGC::shareDirectory() + "/qml/PrimaryFlightDisplayQML.qml");
    QLOG_DEBUG() << url;
    if (!QFile::exists(QGC::shareDirectory() + "/qml/PrimaryFlightDisplayQML.qml"))
    {
        QMessageBox::information(0,"Error", "" + QGC::shareDirectory() + "/qml/PrimaryFlightDisplayQML.qml" + " not found. Please reinstall the application and try again");
        exit(-1);
    }
    m_declarativeView = new QQuickView();
    m_declarativeView->engine()->addImportPath("qml/"); //For local or win32 builds
    m_declarativeView->engine()->addImportPath(QGC::shareDirectory() +"/qml"); //For installed linux builds
    m_declarativeView->setSource(url);


    QLOG_DEBUG() << "QML Status:" << m_declarativeView->status();
    m_declarativeView->setResizeMode(QQuickView::SizeRootObjectToView);
    QVBoxLayout* layout = new QVBoxLayout();
    QWidget *viewcontainer = QWidget::createWindowContainer(m_declarativeView);
    layout->addWidget(viewcontainer);
    setLayout(layout);
    show();

    // Connect with UAS
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this,
            SLOT(setActiveUAS(UASInterface*)), Qt::UniqueConnection);
    setActiveUAS(UASManager::instance()->getActiveUAS());

}

PrimaryFlightDisplayQML::~PrimaryFlightDisplayQML()
{
//    delete ui;
}

void PrimaryFlightDisplayQML::setActiveUAS(UASInterface *uas)
{
    if (m_uasInterface) {
        disconnect(uas,SIGNAL(textMessageReceived(int,int,int,QString)),
                this,SLOT(uasTextMessage(int,int,int,QString)));
    }
    m_uasInterface = uas;

    if (m_uasInterface) {
        connect(uas,SIGNAL(textMessageReceived(int,int,int,QString)),
                this,SLOT(uasTextMessage(int,int,int,QString)));
        VehicleOverview *obj = LinkManager::instance()->getUasObject(uas->getUASID())->getVehicleOverview();
        RelPositionOverview *rel = LinkManager::instance()->getUasObject(uas->getUASID())->getRelPositionOverview();
        m_declarativeView->rootContext()->setContextProperty("vehicleoverview",obj);
        m_declarativeView->rootContext()->setContextProperty("relpositionoverview",rel);
        m_declarativeView->rootContext()->setContextProperty("abspositionoverview",LinkManager::instance()->getUasObject(uas->getUASID())->getAbsPositionOverview());
        QMetaObject::invokeMethod(m_declarativeView->rootObject(),"activeUasSet");
    }
}

void PrimaryFlightDisplayQML::uasTextMessage(int uasid, int componentid, int severity, QString text)
{
    Q_UNUSED(uasid);
    Q_UNUSED(componentid);
    if (text.contains("PreArm") || severity == 3)
    {
        QObject *root = m_declarativeView->rootObject();
        root->setProperty("statusMessage", text);
        root->setProperty("showStatusMessage", true);
    }
}