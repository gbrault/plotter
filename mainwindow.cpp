/***************************************************************************
**  This file is part of Serial Port Plotter                              **
**                                                                        **
**                                                                        **
**  Serial Port Plotter is a program for plotting integer data from       **
**  serial port or TCP/IP stream                                          **
**  using Qt and QCustomPlot                                              **
**                                                                        **
**  This program is free software: you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation, either version 3 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  along with this program.  If not, see http://www.gnu.org/licenses/.   **
**                                                                        **
****************************************************************************
**           Author: Borislav                                             **
**           Contact: b.kereziev@gmail.com                                **
**           Creation Date: 29.12.14                                      **
**           Modified by Gilbert Brault                                   **
**           Date: 26/02/2016                                             **
***************************************************************************/

#include "mainwindow.hpp"
#include "ui_mainwindow.h"

/******************************************************************************************************************/
/* Constructor */
/******************************************************************************************************************/
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    connected(false), plotting(false), dataPointNumber(0), numberOfAxes(1), STATE(WAIT_START), NUMBER_OF_POINTS(500)
{
    ui->setupUi(this);
    createUI();                                                                           // Create the UI
    configurePlot();
    serialPort = NULL;                                                                    // Set serial port pointer to NULL initially
    myServer=NULL;                                                                        // Set TCP SERVER pointer to NULL initially
    ui->stopPlotButton->setEnabled(false);                                                // Plot button is disabled initially

    connect(&updateTimer, SIGNAL(timeout()), this, SLOT(replot()));                       // Connect update timer to replot slot
    updateTimer.start(50);
    connected=false;
    /*
    ui->comboAxes->addItem("1");                                                          // Populate axes combo box; 3 axes maximum allowed
    ui->comboAxes->addItem("2");
    ui->comboAxes->addItem("3");
    ui->comboAxes->setEnabled(enable);
    */
    mFirst="";
    mMiddle="";
    mLast="";
    mAfter="";
    lastTime = duration_cast< milliseconds >(
                system_clock::now().time_since_epoch()
            );
    lastdataPointNumber=0;
    targetConsole="";
    myAudioOutput=NULL;
    audioBuffer.clear();
    lock=0;
    simulatePeriod=10;
    m_Source = NULL;
}
/******************************************************************************************************************/


/******************************************************************************************************************/
/* Destructor */
/******************************************************************************************************************/
MainWindow::~MainWindow()
{
    if(serialPort != NULL) delete serialPort;
    if(myServer!=NULL) delete myServer;
    delete ui;
}
/******************************************************************************************************************/

/******************************************************************************************************************/
/**Create the GUI */
/******************************************************************************************************************/
void MainWindow::configurePlot()
{
    ui->plot->setBackground(QBrush(QColor(48,47,47)));                                    // Background for the plot area
    setupPlot();                                                                          // Setup plot area

    ui->plot->setNotAntialiasedElements(QCP::aeAll);                                      // used for higher performance (see QCustomPlot real time example)
    QFont font;
    font.setStyleStrategy(QFont::NoAntialias);
    ui->plot->xAxis->setTickLabelFont(font);
    ui->plot->yAxis->setTickLabelFont(font);
    ui->plot->legend->setFont(font);

    ui->plot->yAxis->setAutoTickStep(true);                                              // User can change tick step with a spin box
    // ui->plot->yAxis->setTickStep(ui->spinYStep->value());                                                    // Set initial tick step
    ui->plot->xAxis->setTickLabelColor(QColor(170,170,170));                              // Tick labels color
    ui->plot->yAxis->setTickLabelColor(QColor(170,170,170));                              // See QCustomPlot examples / styled demo
    ui->plot->xAxis->grid()->setPen(QPen(QColor(170,170,170), 1, Qt::DotLine));
    ui->plot->yAxis->grid()->setPen(QPen(QColor(170,170,170), 1, Qt::DotLine));
    ui->plot->xAxis->grid()->setSubGridPen(QPen(QColor(80,80,80), 1, Qt::DotLine));
    ui->plot->yAxis->grid()->setSubGridPen(QPen(QColor(80,80,80), 1, Qt::DotLine));
    ui->plot->xAxis->grid()->setSubGridVisible(true);
    ui->plot->yAxis->grid()->setSubGridVisible(true);
    ui->plot->xAxis->setBasePen(QPen(QColor(170,170,170)));
    ui->plot->yAxis->setBasePen(QPen(QColor(170,170,170)));
    ui->plot->xAxis->setTickPen(QPen(QColor(170,170,170)));
    ui->plot->yAxis->setTickPen(QPen(QColor(170,170,170)));
    ui->plot->xAxis->setSubTickPen(QPen(QColor(170,170,170)));
    ui->plot->yAxis->setSubTickPen(QPen(QColor(170,170,170)));
    ui->plot->xAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
    ui->plot->yAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
    ui->plot->setInteraction(QCP::iRangeDrag, true);
    ui->plot->setInteraction(QCP::iRangeZoom, true);
    // Slot for printing coordinates
    connect(ui->plot, SIGNAL(mouseRelease(QMouseEvent*)), this, SLOT(onMouseMoveInPlot(QMouseEvent*)));
}

/******************************************************************************************************************/
/**Create the GUI */
/******************************************************************************************************************/
void MainWindow::createUI()
{
    if(QSerialPortInfo::availablePorts().size() == 0) {                                   // Check if there are any ports at all; if not, disable controls and return
        enableControls(false);
        ui->connectButton->setEnabled(false);
        writeStatus("No ports detected.",last);
        ui->saveJPGButton->setEnabled(false);
        return;
    }

    for(QSerialPortInfo port : QSerialPortInfo::availablePorts()) {                       // List all available serial ports and populate ports combo box
        ui->comboPort->addItem(port.portName());
    }

    ui->comboBaud->addItem("1200");                                                       // Populate baud rate combo box
    ui->comboBaud->addItem("2400");
    ui->comboBaud->addItem("4800");
    ui->comboBaud->addItem("9600");
    ui->comboBaud->addItem("19200");
    ui->comboBaud->addItem("38400");
    ui->comboBaud->addItem("57600");
    ui->comboBaud->addItem("115200");
    ui->comboBaud->setCurrentIndex(7);                                                    // Select 9600 bits by default

    ui->comboData->addItem("8 bits");                                                     // Populate data bits combo box
    ui->comboData->addItem("7 bits");

    ui->comboParity->addItem("none");                                                     // Populate parity combo box
    ui->comboParity->addItem("odd");
    ui->comboParity->addItem("even");

    ui->comboStop->addItem("1 bit");                                                      // Populate stop bits combo box
    ui->comboStop->addItem("2 bits");

}
/******************************************************************************************************************/


/******************************************************************************************************************/
/* Enable/disable controls */
/******************************************************************************************************************/
void MainWindow::enableControls(bool enable)
{
    ui->comboBaud->setEnabled(enable);                                                    // Disable controls in the GUI
    ui->comboData->setEnabled(enable);
    ui->comboParity->setEnabled(enable);
    ui->comboPort->setEnabled(enable);
    ui->comboStop->setEnabled(enable);
}
/******************************************************************************************************************/


/******************************************************************************************************************/
/* Setup the plot area */
/******************************************************************************************************************/
void MainWindow::setupPlot()
{
    ui->plot->clearItems();                                                              // Remove everything from the plot

    numberOfAxes = ui->comboAxes->currentText().toInt();                                 // Get number of axes from the user combo
    ui->plot->yAxis->setRange(ui->spinAxesMin->value(), ui->spinAxesMax->value());       // Set lower and upper plot range
    min = ui->spinAxesMin->value();
    max = ui->spinAxesMax->value();
    ui->plot->yAxis->setTickStep(ui->spinYStep->value());                                // Set tick step according to user spin box
    ui->plot->xAxis->setRange(0, NUMBER_OF_POINTS);                                      // Set x axis range for specified number of points

    if(numberOfAxes == 1) {                                                               // If 1 axis selected
        ui->plot->addGraph();                                                             // add Graph 0
        ui->plot->graph(0)->setPen(QPen(Qt::red));
    } else if(numberOfAxes == 2) {                                                        // If 2 axes selected
        ui->plot->addGraph();                                                             // add Graph 0
        ui->plot->graph(0)->setPen(QPen(Qt::red));
        ui->plot->addGraph();                                                             // add Graph 1
        ui->plot->graph(1)->setPen(QPen(Qt::yellow));
    } else if(numberOfAxes == 3) {                                                        // If 3 axis selected
        ui->plot->addGraph();                                                             // add Graph 0
        ui->plot->graph(0)->setPen(QPen(Qt::red));
        ui->plot->addGraph();                                                             // add Graph 1
        ui->plot->graph(1)->setPen(QPen(Qt::yellow));
        ui->plot->addGraph();                                                             // add Graph 2
        ui->plot->graph(2)->setPen(QPen(Qt::green));
    }
}
/******************************************************************************************************************/


/******************************************************************************************************************/
/* Open the inside serial port; connect its signals */
/******************************************************************************************************************/
void MainWindow::openPort(QSerialPortInfo portInfo, int baudRate, QSerialPort::DataBits dataBits, QSerialPort::Parity parity, QSerialPort::StopBits stopBits)
{

    serialPort = new QSerialPort(portInfo, 0);                                            // Create a new serial port

    connect(this, SIGNAL(portOpenOK()), this, SLOT(portOpenedSuccess()));                 // Connect port signals to GUI slots
    connect(this, SIGNAL(portOpenFail()), this, SLOT(portOpenedFail()));
    connect(this, SIGNAL(portClosed()), this, SLOT(onPortClosed()));
    connect(this, SIGNAL(newData(QStringList)), this, SLOT(onNewDataArrived(QStringList)));
    connect(serialPort, SIGNAL(readyRead()), this, SLOT(readData()));

    if(serialPort->open(QIODevice::ReadWrite) ) {
        serialPort->setBaudRate(baudRate);
        serialPort->setParity(parity);
        serialPort->setDataBits(dataBits);
        serialPort->setStopBits(stopBits);
        emit portOpenOK();
    } else {
        emit portOpenedFail();
        qDebug() << serialPort->errorString();
    }
}

/******************************************************************************************************************/


/******************************************************************************************************************/
/* Port Combo Box index changed slot; displays info for selected port when combo box is changed */
/******************************************************************************************************************/
void MainWindow::on_comboPort_currentIndexChanged(const QString &arg1)
{
    QSerialPortInfo selectedPort(arg1);                                                   // Dislplay info for selected port    
    writeStatus(selectedPort.description(),last);
}
/******************************************************************************************************************/


/******************************************************************************************************************/
/* Connect Button clicked slot; handles connect and disconnect */
/******************************************************************************************************************/
void MainWindow::on_connectButton_clicked()
{
    if(!ui->checkBox->isChecked()){
        if(connected) {                                                                       // If application is connected, disconnect
            serialPort->close();                                                              // Close serial port
            emit portClosed();                                                                // Notify application
            delete serialPort;                                                                // Delete the pointer
            serialPort = NULL;                                                                // Assign NULL to dangling pointer
            ui->connectButton->setText("Connect");                                            // Change Connect button text, to indicate disconnected
            writeStatus("Disconnected!",last);                                               // Show message in status bar
            connected = false;                                                                // Set connected status flag to false
            plotting = false;                                                                 // Not plotting anymore
            receivedData.clear();                                                             // Clear received string
            ui->stopPlotButton->setEnabled(false);                                            // Take care of controls
            ui->saveJPGButton->setEnabled(false);
            enableControls(true);
        } else {                                                                              // If application is not connected, connect
            // Get parameters from controls first
            QSerialPortInfo portInfo(ui->comboPort->currentText());                           // Temporary object, needed to create QSerialPort
            int baudRate = ui->comboBaud->currentText().toInt();                              // Get baud rate from combo box
            int dataBitsIndex = ui->comboData->currentIndex();                                // Get index of data bits combo box
            int parityIndex = ui->comboParity->currentIndex();                                // Get index of parity combo box
            int stopBitsIndex = ui->comboStop->currentIndex();                                // Get index of stop bits combo box
            QSerialPort::DataBits dataBits;
            QSerialPort::Parity parity;
            QSerialPort::StopBits stopBits;

            if(dataBitsIndex == 0) {                                                          // Set data bits according to the selected index
                dataBits = QSerialPort::Data8;
            } else {
                dataBits = QSerialPort::Data7;
            }

            if(parityIndex == 0) {                                                            // Set parity according to the selected index
                parity = QSerialPort::NoParity;
            } else if(parityIndex == 1) {
                parity = QSerialPort::OddParity;
            } else {
                parity = QSerialPort::EvenParity;
            }

            if(stopBitsIndex == 0) {                                                          // Set stop bits according to the selected index
                stopBits = QSerialPort::OneStop;
            } else {
                stopBits = QSerialPort::TwoStop;
            }

            serialPort = new QSerialPort(portInfo, 0);                                        // Use local instance of QSerialPort; does not crash
            openPort(portInfo, baudRate, dataBits, parity, stopBits);                         // Open serial port and connect its signals
        }
    }
}
/******************************************************************************************************************/


/******************************************************************************************************************/
/* Slot for port opened successfully */
/******************************************************************************************************************/
void MainWindow::portOpenedSuccess()
{
    //qDebug() << "Port opened signal received!";
    ui->connectButton->setText("Disconnect");                                             // Change buttons
    writeStatus("Connected!",last);
    enableControls(false);                                                                // Disable controls if port is open
    ui->stopPlotButton->setText("Stop Plot");                                             // Enable button for stopping plot
    ui->stopPlotButton->setEnabled(true);
    ui->saveJPGButton->setEnabled(true);                                                  // Enable button for saving plot
    setupPlot();                                                                          // Create the QCustomPlot area
    updateTimer.start(50);                                                                // Slot is refreshed 20 times per second
    connected = true;                                                                     // Set flags
    plotting = true;
}
/******************************************************************************************************************/


/******************************************************************************************************************/
/* Slot for fail to open the port */
/******************************************************************************************************************/
void MainWindow::portOpenedFail()
{
    //qDebug() << "Port cannot be open signal received!";
    writeStatus("Cannot open port!",last);
}
/******************************************************************************************************************/


/******************************************************************************************************************/
/* Slot for closing the port */
/******************************************************************************************************************/
void MainWindow::onPortClosed()
{
    //qDebug() << "Port closed signal received!";
    updateTimer.stop();
    connected = false;
    disconnect(serialPort, SIGNAL(readyRead()), this, SLOT(readData()));
    disconnect(this, SIGNAL(portOpenOK()), this, SLOT(portOpenedSuccess()));             // Disconnect port signals to GUI slots
    disconnect(this, SIGNAL(portOpenFail()), this, SLOT(portOpenedFail()));
    disconnect(this, SIGNAL(portClosed()), this, SLOT(onPortClosed()));
    disconnect(this, SIGNAL(newData(QStringList)), this, SLOT(onNewDataArrived(QStringList)));
}


/******************************************************************************************************************/
/* Replot */
/******************************************************************************************************************/
void MainWindow::replot()
{
    if(dataPointNumber!=lastdataPointNumber){
        milliseconds now = duration_cast< milliseconds >(
                    system_clock::now().time_since_epoch()
                    );
        milliseconds duration = now-lastTime;
        double speed = (((dataPointNumber-lastdataPointNumber) * 1.0)/(duration.count() * 1.0))*1000;
        lastdataPointNumber = dataPointNumber;
        lastTime=now;
        char buf[20];
        sprintf(buf,"%5.3f",speed);
        QString message(buf);
        writeStatus(message,middle);
    }
    if(connected) {
        ui->plot->xAxis->setRange(dataPointNumber - NUMBER_OF_POINTS, dataPointNumber);
        ui->plot->replot();
    }
}
/******************************************************************************************************************/


/******************************************************************************************************************/
/* Stop Plot Button */
/******************************************************************************************************************/
void MainWindow::on_stopPlotButton_clicked()
{
    if(plotting) {                                                                        // Stop plotting
        updateTimer.stop();                                                               // Stop updating plot timer
        plotting = false;
        ui->stopPlotButton->setText("Start Plot");
    } else {                                                                              // Start plotting
        updateTimer.start();                                                              // Start updating plot timer
        plotting = true;
        ui->stopPlotButton->setText("Stop Plot");
    }
}
/******************************************************************************************************************/


/******************************************************************************************************************/
/* Slot for new data from serial port . Data is comming in QStringList and needs to be parsed */
/******************************************************************************************************************/
void MainWindow::onNewDataArrived(QStringList newData)
{
    if(plotting) { // to-do: multiple plot...
        int value0 = newData[0].toInt() * ui->horizontalSliderAMPL->value();
        if (min>value0){
            min=value0;
            if(ui->checkBoxAutoScale->isChecked()){
                ui->plot->yAxis->setRangeLower(min);
            }
        }
        if (max<value0){
            max = value0;
            if(ui->checkBoxAutoScale->isChecked()){
                ui->plot->yAxis->setRangeUpper(max);
            }
        }
        int dataListSize = newData.size();                                                    // Get size of received list
        dataPointNumber++;  // Increment data number

        if(numberOfAxes == 1 && dataListSize > 0) {                                           // Add data to graphs according to number of axes
            ui->plot->graph(0)->addData(dataPointNumber, value0);                 // Add data to Graph 0
            ui->plot->graph(0)->removeDataBefore(dataPointNumber - NUMBER_OF_POINTS);           // Remove data from graph 0
        } else if(numberOfAxes == 2) {
            ui->plot->graph(0)->addData(dataPointNumber, value0);
            ui->plot->graph(0)->removeDataBefore(dataPointNumber - NUMBER_OF_POINTS);
            if(dataListSize > 1){
                ui->plot->graph(1)->addData(dataPointNumber, newData[1].toInt());
                ui->plot->graph(1)->removeDataBefore(dataPointNumber - NUMBER_OF_POINTS);
            }
        } else if(numberOfAxes == 3) {
            ui->plot->graph(0)->addData(dataPointNumber,value0);
            ui->plot->graph(0)->removeDataBefore(dataPointNumber - NUMBER_OF_POINTS);
            if(dataListSize > 1) {
                ui->plot->graph(1)->addData(dataPointNumber, newData[1].toInt());
                ui->plot->graph(1)->removeDataBefore(dataPointNumber - NUMBER_OF_POINTS);
            }
            if(dataListSize > 2) {
                ui->plot->graph(2)->addData(dataPointNumber, newData[2].toInt());
                ui->plot->graph(2)->removeDataBefore(dataPointNumber - NUMBER_OF_POINTS);
            }
        } else return;
    }
}
/******************************************************************************************************************/


/******************************************************************************************************************/
/* Slot for spin box for plot minimum value on y axis */
/******************************************************************************************************************/
void MainWindow::on_spinAxesMin_valueChanged(int arg1)
{
    ui->plot->yAxis->setRangeLower(arg1);
    ui->plot->replot();
}
/******************************************************************************************************************/


/******************************************************************************************************************/
/* Slot for spin box for plot maximum value on y axis */
/******************************************************************************************************************/
void MainWindow::on_spinAxesMax_valueChanged(int arg1)
{
    ui->plot->yAxis->setRangeUpper(arg1);
    ui->plot->replot();
}
/******************************************************************************************************************/


/******************************************************************************************************************/
/* Read data for inside serial port */
/******************************************************************************************************************/
void MainWindow::readData()
{
    if(serialPort->bytesAvailable()) {                                                    // If any bytes are available
        QByteArray Data = serialPort->readAll();                                          // Read all data in QByteArray
        if(Data.count()!=0)
            readDataTcp(Data);
    }
}
/******************************************************************************************************************/
/* Read data for TCP port */
/******************************************************************************************************************/
void  MainWindow::readDataTcp(QByteArray Data){
    int i;
    QByteArray data;

    for(i=0;i<Data.count();i++){
        data.clear();
        data.append(Data.at(i));
        processData(data);
    }
}

/******************************************************************************************************************/
/* Process Read data */
/******************************************************************************************************************/
void MainWindow::processData(QByteArray data)
{
    char temp;
    if(!data.isEmpty()) {                                                             // If the byte array is not empty
        temp = (uint8_t)(data.at(0));                                                     // Get a '\0'-terminated char* to the data                                        // Iterate over the char*
        switch(STATE) {                                                           // Switch the current state of the message
        case WAIT_START:                                                          // If waiting for start [$], examine each char
            if(temp == START_MSG) {                                            // If the char is $, change STATE to IN_MESSAGE
                STATE = IN_MESSAGE;
                receivedData.clear();                                             // Clear temporary QString that holds the message
                if(targetConsole.count()!=0)
                    writeStatus(targetConsole,after);
                targetConsole.clear();
                break;                                                            // Break out of the switch
            } else {
                targetConsole.append(temp);
            }
            break;
        case IN_MESSAGE:                                                          // If state is IN_MESSAGE
            if(temp == END_MSG) {                                              // If char examined is ;, switch state to END_MSG
                STATE = WAIT_START;
                QStringList incomingData = receivedData.split(' ');               // Split string received from port and put it into list
                emit newData(incomingData);                                       // Emit signal for data received with the list
                break;
            }
            else if(((temp>='0') && (temp<='9')) || temp==' ' ) {                      // If examined char is a digit, and not '$' or ';', append it to temporary string
                receivedData.append(temp);
            }
            else if( temp=='-' || temp=='+' ) {                      // If examined char is a digit, and not '$' or ';', append it to temporary string
                receivedData.append(temp);
            }
            break;
        default: break;
        }
    }

}

/******************************************************************************************************************/


/******************************************************************************************************************/
/* Number of axes combo; when changed, display axes colors in status bar */
/******************************************************************************************************************/
void MainWindow::on_comboAxes_currentIndexChanged(int index)
{
    if(index == 0) {
        writeStatus("Axis 1: Red",last);
    } else if(index == 1) {
        writeStatus("Axis 1: Red; Axis 2: Yellow",last);
    } else {
        writeStatus("Axis 1: Red; Axis 2: Yellow; Axis 3: Green",last);
    }
}
/******************************************************************************************************************/


/******************************************************************************************************************/
/* Spin box for changing the Y Tick step */
/******************************************************************************************************************/
void MainWindow::on_spinYStep_valueChanged(int arg1)
{
    // ui->plot->yAxis->setTickStep(arg1);
    Q_UNUSED(arg1);
    ui->plot->replot();
}
/******************************************************************************************************************/


/******************************************************************************************************************/
/* Save a JPG image of the plot to current EXE directory */
/******************************************************************************************************************/
void MainWindow::on_saveJPGButton_clicked()
{
    ui->plot->saveJpg(QString::number(dataPointNumber) + ".jpg");
}
/******************************************************************************************************************/


/******************************************************************************************************************/
/* Reset the zoom of the plot to the initial values */
/******************************************************************************************************************/
void MainWindow::on_resetPlotButton_clicked()
{
    disconnect(ui->plot, SIGNAL(mouseRelease(QMouseEvent*)), this, SLOT(onMouseMoveInPlot(QMouseEvent*)));
    configurePlot();
    /*
    ui->plot->yAxis->setRange(0, 4095);
    ui->plot->xAxis->setRange(dataPointNumber - NUMBER_OF_POINTS, dataPointNumber);
    ui->plot->yAxis->setTickStep(ui->spinYStep->value());
    */
    ui->plot->replot();
}
/******************************************************************************************************************/


/******************************************************************************************************************/
/* Prints coordinates of mouse pointer in status bar on mouse release */
/******************************************************************************************************************/
void MainWindow::onMouseMoveInPlot(QMouseEvent *event)
{
    int xx = ui->plot->xAxis->pixelToCoord(event->x());
    int yy = ui->plot->yAxis->pixelToCoord(event->y());
    QString coordinates("X: %1 Y: %2");
    coordinates = coordinates.arg(xx).arg(yy);
    writeStatus(coordinates,first);
}
/******************************************************************************************************************/


/******************************************************************************************************************/
/* Spin box controls how many data points are collected and displayed */
/******************************************************************************************************************/
void MainWindow::on_spinPoints_valueChanged(int arg1)
{
    NUMBER_OF_POINTS = arg1;
    ui->plot->replot();
}

/******************************************************************************************************************/

/******************************************************************************************************************/
/* Emulate data emission */
/******************************************************************************************************************/
void MainWindow::on_checkBox_clicked()
{
    if(connected){
        QMessageBox msgBox;
        msgBox.setText("Please Disconnect first");
        msgBox.exec();
    } else {
        if(!ui->checkBox->isChecked()){
            ui->checkBox->setChecked(false);
        } else {
            ui->checkBox->setChecked(true);
        }
    }
}

/******************************************************************************************************************/
/* fake data emission */
/******************************************************************************************************************/
void MainWindow::simulatedData()
{
    // Display curve
    QVector<int> display;
    m_Source->readDisplay(&display, simulatePeriod);
    for(int i=0; i< display.count();i++){
        char buf[100];
        sprintf(buf,"%d",display.at(i));
        receivedData.clear();
        receivedData.append(buf);
        QStringList incomingData = receivedData.split(' ');               // Split string received from port and put it into list
        emit newData(incomingData);                                       // Emit signal for data received with the list
    }

    // plays Audio
    if(ui->checkBoxAudioEnable->isChecked()){
        int bytes = myAudioOutput->bytesFree();
        if(bytes!=0){
            int AudioDuration = myAudioOutput->durationForBytes(bytes)/1000;
            QByteArray audio;
            audio.resize(bytes);
            audio.clear();
            m_Source->readAudio(&audio,AudioDuration);
            myAudioOutput->writeData(audio);
        }
    }

    char buf[100];
    sprintf(buf,"d=%d,a=%d",m_Source->displayTime,m_Source->AudioTime);
    writeStatus(buf,after);
}

/******************************************************************************************************************/
/* Connect TCP Server */
/******************************************************************************************************************/
void MainWindow::on_TCP_Connect_clicked()
{
    if(!ui->checkBox->isChecked()){
        if(!connected){
            if(myServer!=NULL) delete myServer;
            myServer = new MyServer(this);
            myServer->port=ui->TCP_port->text();
            myServer->startServer();
            connected=true;
            plotting = true;
            updateTimer.start(50);
            ui->TCP_Connect->setText("DisConnect");                                  // Change Connect button text, to indicate disconnected
            writeStatus("TCP Connected!",last);                                      // Show message in status bar
            connect(this, SIGNAL(newData(QStringList)), this, SLOT(onNewDataArrived(QStringList)));
            ui->stopPlotButton->setEnabled(true);
        } else {
            if(myServer!=NULL){
                myServer->disconnect();
                delete myServer;
                myServer = NULL;
                connected=false;
                plotting = false;
                updateTimer.stop();
                ui->TCP_Connect->setText("Connect");                                            // Change Connect button text, to indicate disconnected
                writeStatus("Disconnected!",last);                                      // Show message in status bar
                disconnect(this, SIGNAL(newData(QStringList)), this, SLOT(onNewDataArrived(QStringList)));
                ui->stopPlotButton->setEnabled(false);
            }
        }
    }
}

/******************************************************************************************************************/
/* test mode connection */
/******************************************************************************************************************/
void MainWindow::on_testButton_clicked()
{
    if(ui->checkBox->isChecked()){
        if(connected){
            simulate.stop();
            connected=false;
            ui->testButton->setText("Connect");                                     // Change Connect button text, to indicate disconnected
            writeStatus("Disconnected!",last);                                      // Show message in status bar
            disconnect(&simulate, SIGNAL(timeout()), this, SLOT(simulatedData()));
            disconnect(this, SIGNAL(newData(QStringList)), this, SLOT(onNewDataArrived(QStringList)));
            plotting = false;
            updateTimer.stop();
            ui->stopPlotButton->setEnabled(false);
        } else {
            // create the Audio Source
            QAudioFormat format;
            audioParamaters p;
            readAudioparameter(&p);
            format.setByteOrder(p.byteOrder);
            format.setChannelCount(p.channelCount);
            format.setSampleRate(p.sampleRate);
            format.setSampleSize(p.sampleSize);
            format.setSampleType(p.sampleType);
            format.setCodec("audio/pcm");
            int Type=0;
            if(ui->radioButtonTESTRadom->isChecked()){
                Type=1;
            }
            max=min=0;
            if (m_Source!=NULL) delete m_Source;
            m_Source = new Source(ui->spinBoxTESTFreq->value(),simulatePeriod,ui->spinBoxTESTAmplitude->value(),format);
            simulate.setInterval(simulatePeriod);
            simulate.setSingleShot(false);
            connect(&simulate, SIGNAL(timeout()), this, SLOT(simulatedData()));
            connect(this, SIGNAL(newData(QStringList)), this, SLOT(onNewDataArrived(QStringList)));
            simulate.start();
            connected=true;
            plotting = true;
            updateTimer.start(50);
            ui->testButton->setText("DisConnect");                               // Change Connect button text, to indicate disconnected
            writeStatus("Connected!",last);                                      // Show message in status bar
            ui->stopPlotButton->setEnabled(true);
        }
    }
}

/******************************************************************************************************************/
/* Write Status line */
/******************************************************************************************************************/
void MainWindow::writeStatus(QString message, Status type){
    QString sbmessage;
    switch(type){
    case first:
        mFirst = message;
        break;
    case middle:
        mMiddle = message;
        break;
    case last:
        mLast = message;
        break;
    case after:
        mAfter = message;
        break;
    }
    sbmessage=mFirst;
    sbmessage.append("   ");
    sbmessage.append(mMiddle);
    sbmessage.append("   ");
    sbmessage.append(mLast);
    sbmessage.append("   ");
    sbmessage.append(mAfter);
    ui->statusBar->showMessage(sbmessage);
}

/******************************************************************************************************************/
/* Set or Reset Audio taping */
/******************************************************************************************************************/
void MainWindow::on_checkBoxAudioEnable_clicked(bool checked)
{
    if(lock) {
        writeStatus("on-going action-- wait",after);
        return;
    }
    if(checked!=0){
        myAudioOutput = new AudioOutput(this);
        // configure it
        audioParamaters p;
        readAudioparameter(&p);
        myAudioOutput->configure(p.channelCount,p.sampleRate,p.sampleSize,p.byteOrder,p.sampleType,p.buffersize);
        myAudioOutput->createDevice();
    } else{
        if (myAudioOutput!=NULL){
            myAudioOutput->close();
            delete myAudioOutput;
        }
    }
    lock=0;
}

/******************************************************************************************************************/
/* Read Audio parameters */
/******************************************************************************************************************/
void MainWindow::readAudioparameter(audioParamaters *p)
{
    p->channelCount=ui->spinBoxAudioChannels->value();
    p->sampleRate=(ui->comboBoxAudioFreq->currentText().toInt())*1000;
    p->sampleSize=ui->comboBoxAudioSize->currentText().toInt();
    if (ui->comboBoxAudioEndian->currentText() == "little"){
        p->byteOrder = QAudioFormat::Endian::LittleEndian;
    } else if (ui->comboBoxAudioEndian->currentText() == "big"){
        p->byteOrder = QAudioFormat::Endian::BigEndian;
    }
    if (ui->comboBoxAudioType->currentText() == "Unknown"){
        p->sampleType = QAudioFormat::SampleType::Unknown;
    } else if (ui->comboBoxAudioType->currentText() == "SignedInt"){
        p->sampleType = QAudioFormat::SampleType::SignedInt;
    } else if (ui->comboBoxAudioType->currentText() == "UnSignedInt"){
        p->sampleType = QAudioFormat::SampleType::UnSignedInt;
    } else if (ui->comboBoxAudioType->currentText() == "Float"){
        p->sampleType = QAudioFormat::SampleType::Float;
    }
    p->buffersize = ui->spinBoxAudioBuffer->value()*1024;
}



/******************************************************************************************************************/
/* Set signal amplification maximum multiplier */
/******************************************************************************************************************/
void MainWindow::on_spinBoxAMPMult_valueChanged(int arg1)
{
    ui->horizontalSliderAMPL->setMaximum(arg1);
}

/******************************************************************************************************************/
/* Set signal amplification multiplier step */
/******************************************************************************************************************/
void MainWindow::on_spinBoxAMPLStep_valueChanged(int arg1)
{
    ui->horizontalSliderAMPL->setSingleStep(arg1);
}

/******************************************************************************************************************/
/* Set Random test signal */
/******************************************************************************************************************/
void MainWindow::on_radioButtonTESTRadom_clicked(bool checked)
{
   Q_UNUSED(checked);
}

/******************************************************************************************************************/
/* Set Sine test signal */
/******************************************************************************************************************/
void MainWindow::on_radioButtonTESTSine_clicked(bool checked)
{
    Q_UNUSED(checked);
}

/******************************************************************************************************************/
/* Set/Reset test */
/******************************************************************************************************************/
void MainWindow::on_checkBox_clicked(bool checked)
{
    Q_UNUSED(checked);
    if(connected){
        // cannot change state if connected
        QMessageBox msgBox;
        msgBox.setText("Please Disconnect first");
        msgBox.exec();
        ui->checkBox->setChecked(true); // force it back again
    }
}

/******************************************************************************************************************/
/* Adjust simulated period to be a multiple of frequency
/******************************************************************************************************************/
void MainWindow::on_spinBoxTESTFreq_valueChanged(int arg1)
{
    Q_UNUSED(arg1);
}
