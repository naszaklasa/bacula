/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2000-2007 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of John Walker.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/
 
/*
 *   Version $Id: jobplot.cpp 5296 2007-08-07 03:00:36Z bartleyd2 $
 *
 *  JobPlots Class
 *
 *   Dirk Bartley, March 2007
 *
 */ 

#include <QtGui>
#include "bat.h"
#include "jobgraphs/jobplot.h"


JobPlotPass::JobPlotPass()
{
   use = false;
}

JobPlotPass& JobPlotPass::operator=(const JobPlotPass &cp)
{
   use = cp.use;
   recordLimitCheck = cp.recordLimitCheck;
   daysLimitCheck = cp.daysLimitCheck;
   recordLimitSpin = cp.recordLimitSpin;
   daysLimitSpin = cp.daysLimitSpin;
   jobCombo = cp.jobCombo;
   clientCombo = cp.clientCombo;
   volumeCombo = cp.volumeCombo;
   fileSetCombo = cp.fileSetCombo;
   purgedCombo = cp.purgedCombo;
   levelCombo = cp.levelCombo;
   statusCombo = cp.statusCombo;
   return *this;
}

/*
 * Constructor for the controls class which inherits QScrollArea and a ui header
 */
JobPlotControls::JobPlotControls()
{
   setupUi(this);
}

/*
 * Constructor, this class does not inherit anything but pages.
 */
JobPlot::JobPlot(QTreeWidgetItem *parentTreeWidgetItem, JobPlotPass &passVals)
{
   setupUserInterface();
   m_name = "JobPlot";
   pgInitialize(parentTreeWidgetItem);
   readSplitterSettings();
   QTreeWidgetItem* thisitem = mainWin->getFromHash(this);
   thisitem->setIcon(0,QIcon(QString::fromUtf8(":images/graph1.png")));
   m_drawn = false;

   /* this invokes the pass values = operator function */
   m_pass = passVals;
   m_closeable = true;
   dockPage();
   /* If the values of the controls are predetermined (from joblist), then set
    * this class as current window at the front of the stack */
   if (m_pass.use)
      setCurrent();
   m_jobPlot->replot();
}

/*
 * Kill, crush Destroy
 */
JobPlot::~JobPlot()
{
   if (m_drawn)
      writeSettings();
   m_pjd.clear();
}

/*
 * This is called when the page selector has this page selected
 */
void JobPlot::currentStackItem()
{
   if (!m_drawn) {
      setupControls();
      reGraph();
      m_drawn=true;
   }
}

/*
 * Slot for the refrehs push button, also called from constructor.
 */
void JobPlot::reGraph()
{
   /* clear m_pjd */
   m_pjd.clear();
   runQuery();
   m_jobPlot->clear();
   addCurve();
   m_jobPlot->replot();
}

/*
 * Setup the control widgets for the graph, this are the objects from JobPlotControls
 */
void JobPlot::setupControls()
{
   QStringList graphType = QStringList() << /* "Fitted" <<*/ "Sticks" << "Lines" << "Steps" << "None";
   controls->plotTypeCombo->addItems(graphType);
   QStringList symbolType = QStringList() << "Ellipse" << "Rect" << "Diamond" << "Triangle"
            << "DTrianle" << "UTriangle" << "LTriangle" << "RTriangle" << "Cross" << "XCross"
            << "HLine" << "Vline" << "Star1" << "Star2" << "Hexagon" << "None";
   controls->fileSymbolTypeCombo->addItems(symbolType);
   controls->byteSymbolTypeCombo->addItems(symbolType);
   readControlSettings();

   controls->fileCheck->setCheckState(Qt::Checked);
   controls->byteCheck->setCheckState(Qt::Checked);
   connect(controls->plotTypeCombo, SIGNAL(currentIndexChanged(QString)), this, SLOT(setPlotType(QString)));
   connect(controls->fileSymbolTypeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(setFileSymbolType(int)));
   connect(controls->byteSymbolTypeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(setByteSymbolType(int)));
   connect(controls->fileCheck, SIGNAL(stateChanged(int)), this, SLOT(fileCheckChanged(int)));
   connect(controls->byteCheck, SIGNAL(stateChanged(int)), this, SLOT(byteCheckChanged(int)));
   connect(controls->refreshButton, SIGNAL(pressed()), this, SLOT(reGraph()));

   controls->clientComboBox->addItem("Any");
   controls->clientComboBox->addItems(m_console->client_list);

   QStringList volumeList;
   m_console->getVolumeList(volumeList);
   controls->volumeComboBox->addItem("Any");
   controls->volumeComboBox->addItems(volumeList);
   controls->jobComboBox->addItem("Any");
   controls->jobComboBox->addItems(m_console->job_list);
   controls->levelComboBox->addItem("Any");
   controls->levelComboBox->addItems( QStringList() << "F" << "D" << "I");
   controls->purgedComboBox->addItem("Any");
   controls->purgedComboBox->addItems( QStringList() << "0" << "1");
   controls->fileSetComboBox->addItem("Any");
   controls->fileSetComboBox->addItems(m_console->fileset_list);
   QStringList statusLongList;
   m_console->getStatusList(statusLongList);
   controls->statusComboBox->addItem("Any");
   controls->statusComboBox->addItems(statusLongList);

   if (m_pass.use) {
      controls->limitCheckBox->setCheckState(m_pass.recordLimitCheck);
      controls->limitSpinBox->setValue(m_pass.recordLimitSpin);
      controls->daysCheckBox->setCheckState(m_pass.daysLimitCheck);
      controls->daysSpinBox->setValue(m_pass.daysLimitSpin);
      int jobIndex = controls->jobComboBox->findText(m_pass.jobCombo, Qt::MatchExactly); 
      if (jobIndex != -1)
         controls->jobComboBox->setCurrentIndex(jobIndex);
      int clientIndex = controls->clientComboBox->findText(m_pass.clientCombo, Qt::MatchExactly);
      if (clientIndex != -1)
         controls->clientComboBox->setCurrentIndex(clientIndex);
      int volumeIndex = controls->volumeComboBox->findText(m_pass.volumeCombo, Qt::MatchExactly);
      if (volumeIndex != -1)
         controls->volumeComboBox->setCurrentIndex(volumeIndex);
      int filesetIndex = controls->fileSetComboBox->findText(m_pass.fileSetCombo, Qt::MatchExactly);
      if (filesetIndex != -1)
         controls->fileSetComboBox->setCurrentIndex(filesetIndex);
      int purgedIndex = controls->purgedComboBox->findText(m_pass.purgedCombo, Qt::MatchExactly); 
      if (purgedIndex != -1)
         controls->purgedComboBox->setCurrentIndex(purgedIndex);
      int levelIndex = controls->levelComboBox->findText(m_pass.levelCombo, Qt::MatchExactly); 
      if (levelIndex != -1)
         controls->levelComboBox->setCurrentIndex(levelIndex);
      int statusIndex = controls->statusComboBox->findText(m_pass.statusCombo, Qt::MatchExactly); 
      if (statusIndex != -1)
         controls->statusComboBox->setCurrentIndex(statusIndex);
   } else {
      /* Set Defaults for check and spin for limits */
      controls->limitCheckBox->setCheckState(mainWin->m_recordLimitCheck ? Qt::Checked : Qt::Unchecked);
      controls->limitSpinBox->setValue(mainWin->m_recordLimitVal);
      controls->daysCheckBox->setCheckState(mainWin->m_daysLimitCheck ? Qt::Checked : Qt::Unchecked);
      controls->daysSpinBox->setValue(mainWin->m_daysLimitVal);
   } 
}

/*
 * Setup the control widgets for the graph, this are the objects from JobPlotControls
 */
void JobPlot::runQuery()
{
   /* Set up query */
   QString query("");
   query += "SELECT DISTINCT "
            " Job.Starttime AS JobStart,"
            " Job.Jobfiles AS FileCount,"
            " Job.JobBytes AS Bytes,"
            " Job.JobId AS JobId"
            " FROM Job"
            " LEFT OUTER JOIN Client ON (Client.ClientId=Job.ClientId)"
            " LEFT OUTER JOIN FileSet ON (FileSet.FileSetId=Job.FileSetId)"
            " LEFT OUTER JOIN Status ON (Job.JobStatus=Status.JobStatus)"
            " LEFT OUTER JOIN JobMedia ON (JobMedia.JobId=Job.JobId)"
            " LEFT OUTER JOIN Media ON (JobMedia.MediaId=Media.MediaId)";
   QStringList conditions;
   int jobIndex = controls->jobComboBox->currentIndex();
   if ((jobIndex != -1) && (controls->jobComboBox->itemText(jobIndex) != "Any"))
      conditions.append("Job.Name='" + controls->jobComboBox->itemText(jobIndex) + "'");
   int clientIndex = controls->clientComboBox->currentIndex();
   if ((clientIndex != -1) && (controls->clientComboBox->itemText(clientIndex) != "Any"))
      conditions.append("Client.Name='" + controls->clientComboBox->itemText(clientIndex) + "'");
   int volumeIndex = controls->volumeComboBox->currentIndex();
   if ((volumeIndex != -1) && (controls->volumeComboBox->itemText(volumeIndex) != "Any"))
      conditions.append("Media.VolumeName='" + controls->volumeComboBox->itemText(volumeIndex) + "'");
   int fileSetIndex = controls->fileSetComboBox->currentIndex();
   if ((fileSetIndex != -1) && (controls->fileSetComboBox->itemText(fileSetIndex) != "Any"))
      conditions.append("FileSet.FileSet='" + controls->fileSetComboBox->itemText(fileSetIndex) + "'");
   int purgedIndex = controls->purgedComboBox->currentIndex();
   if ((purgedIndex != -1) && (controls->purgedComboBox->itemText(purgedIndex) != "Any"))
      conditions.append("Job.PurgedFiles='" + controls->purgedComboBox->itemText(purgedIndex) + "'");
   int levelIndex = controls->levelComboBox->currentIndex();
   if ((levelIndex != -1) && (controls->levelComboBox->itemText(levelIndex) != "Any"))
      conditions.append("Job.Level='" + controls->levelComboBox->itemText(levelIndex) + "'");
   int statusIndex = controls->statusComboBox->currentIndex();
   if ((statusIndex != -1) && (controls->statusComboBox->itemText(statusIndex) != "Any"))
      conditions.append("Status.JobStatusLong='" + controls->statusComboBox->itemText(statusIndex) + "'");
   /* If Limit check box For limit by days is checked  */
   if (controls->daysCheckBox->checkState() == Qt::Checked) {
      QDateTime stamp = QDateTime::currentDateTime().addDays(-controls->daysSpinBox->value());
      QString since = stamp.toString(Qt::ISODate);
      conditions.append("Job.Starttime>'" + since + "'");
   }
   bool first = true;
   foreach (QString condition, conditions) {
      if (first) {
         query += " WHERE " + condition;
         first = false;
      } else {
         query += " AND " + condition;
      }
   }
   /* Descending */
   query += " ORDER BY Job.Starttime DESC, Job.JobId DESC";
   /* If Limit check box for limit records returned is checked  */
   if (controls->limitCheckBox->checkState() == Qt::Checked) {
      QString limit;
      limit.setNum(controls->limitSpinBox->value());
      query += " LIMIT " + limit;
   }

   if (mainWin->m_sqlDebug) {
      Pmsg1(000, "Query cmd : %s\n",query.toUtf8().data());
   }
   QString resultline;
   QStringList results;
   if (m_console->sql_cmd(query, results)) {

      QString field;
      QStringList fieldlist;
   
      int row = 0;
      /* Iterate through the record returned from the query */
      foreach (resultline, results) {
         PlotJobData *plotJobData = new PlotJobData();
         fieldlist = resultline.split("\t");
         int column = 0;
         QString statusCode("");
         /* Iterate through fields in the record */
         foreach (field, fieldlist) {
            field = field.trimmed();  /* strip leading & trailing spaces */
            if (column == 0) {
               plotJobData->dt = QDateTime::fromString(field, mainWin->m_dtformat);
            } else if (column == 1) {
               plotJobData->files = field.toDouble();
            } else if (column == 2) {
               plotJobData->bytes = field.toDouble();
            }
            column++;
            m_pjd.prepend(plotJobData);
         }
         row++;
      }
   } 
   if ((controls->volumeComboBox->itemText(volumeIndex) != "Any") && (results.count() == 0)){
      /* for context sensitive searches, let the user know if there were no
       *        * results */
      QMessageBox::warning(this, tr("Bat"),
          tr("The Jobs query returned no results.\n"
         "Press OK to continue?"), QMessageBox::Ok );
   }
}

/*
 * The user interface that used to be in the ui header.  I wanted to have a
 * scroll area which is not in designer.
 */
void JobPlot::setupUserInterface()
{
   QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(5));
   sizePolicy.setHorizontalStretch(0);
   sizePolicy.setVerticalStretch(0);
   sizePolicy.setVerticalStretch(0);
   sizePolicy.setVerticalPolicy(QSizePolicy::Ignored);
   sizePolicy.setHorizontalPolicy(QSizePolicy::Ignored);
   m_gridLayout = new QGridLayout(this);
   m_gridLayout->setSpacing(6);
   m_gridLayout->setMargin(9);
   m_gridLayout->setObjectName(QString::fromUtf8("m_gridLayout"));
   m_splitter = new QSplitter(this);
   m_splitter->setObjectName(QString::fromUtf8("m_splitter"));
   m_splitter->setOrientation(Qt::Horizontal);
   m_jobPlot = new QwtPlot(m_splitter);
   m_jobPlot->setObjectName(QString::fromUtf8("m_jobPlot"));
   m_jobPlot->setSizePolicy(sizePolicy);
   m_jobPlot->setMinimumSize(QSize(0, 0));
   QScrollArea *area = new QScrollArea(m_splitter);
   area->setObjectName(QString::fromUtf8("area"));
   controls = new JobPlotControls();
   area->setWidget(controls);
   
   m_splitter->addWidget(m_jobPlot);
   m_splitter->addWidget(area);

   m_gridLayout->addWidget(m_splitter, 0, 0, 1, 1);
}

/*
 * Add the curves to the plot
 */
void JobPlot::addCurve()
{
   m_jobPlot->setTitle("Files and Bytes backed up");
   m_jobPlot->insertLegend(new QwtLegend(), QwtPlot::RightLegend);

   // Set axis titles
   m_jobPlot->enableAxis(QwtPlot::yRight);
   m_jobPlot->setAxisTitle(QwtPlot::yRight, "<-- Bytes Kb");
   m_jobPlot->setAxisTitle(m_jobPlot->xBottom, "date of backup -->");
   m_jobPlot->setAxisTitle(m_jobPlot->yLeft, "Number of Files -->");
   m_jobPlot->setAxisScaleDraw(QwtPlot::xBottom, new DateTimeScaleDraw());

   // Insert new curves
   m_fileCurve = new QwtPlotCurve("Files");
   m_fileCurve->setPen(QPen(Qt::red));
   m_fileCurve->setCurveType(m_fileCurve->Yfx);
   m_fileCurve->setYAxis(QwtPlot::yLeft);

   m_byteCurve = new QwtPlotCurve("Bytes");
   m_byteCurve->setPen(QPen(Qt::blue));
   m_byteCurve->setCurveType(m_byteCurve->Yfx);
   m_byteCurve->setYAxis(QwtPlot::yRight);
   setPlotType(controls->plotTypeCombo->currentText());
   setFileSymbolType(controls->fileSymbolTypeCombo->currentIndex());
   setByteSymbolType(controls->byteSymbolTypeCombo->currentIndex());

   m_fileCurve->attach(m_jobPlot);
   m_byteCurve->attach(m_jobPlot);

   // attach data
   int size = m_pjd.count();
   double tval[size];
   double fval[size];
   double bval[size];
   int j = 0;
   foreach (PlotJobData* plotJobData, m_pjd) {
//      printf("%.0f %.0f %s\n", plotJobData->bytes, plotJobData->files,
//              plotJobData->dt.toString(mainWin->m_dtformat).toUtf8().data());
      fval[j] = plotJobData->files;
      bval[j] = plotJobData->bytes / 1024;
      tval[j] = plotJobData->dt.toTime_t();
//      printf("%i %.0f %.0f %.0f\n", j, tval[j], fval[j], bval[j]);
      j++;
   }
   m_fileCurve->setData(tval,fval,size);
   m_byteCurve->setData(tval,bval,size);

   for (int year=2000; year<2010; year++) {
      for (int month=1; month<=12; month++) {
         QString monthBegin;
         if (month > 9) {
            QTextStream(&monthBegin) << year << "-" << month << "-01 00:00:00";
         } else {
            QTextStream(&monthBegin) << year << "-0" << month << "-01 00:00:00";
         }
         QDateTime mdt = QDateTime::fromString(monthBegin, mainWin->m_dtformat);
         double monbeg = mdt.toTime_t();
   
         //  ...a vertical line at the first of each month
         QwtPlotMarker *mX = new QwtPlotMarker();
         mX->setLabel(mdt.toString("MMM-d"));
         mX->setLabelAlignment(Qt::AlignRight|Qt::AlignTop);
         mX->setLineStyle(QwtPlotMarker::VLine);
         QPen pen(Qt::darkGray);
         pen.setStyle(Qt::DashDotDotLine);
         mX->setLinePen(pen);
         mX->setXValue(monbeg);
         mX->attach(m_jobPlot);
      }
   }
}

/*
 * slot to respond to the plot type combo changing
 */
void JobPlot::setPlotType(QString currentText)
{
   QwtPlotCurve::CurveStyle style = QwtPlotCurve::NoCurve;
   if (currentText == "Fitted") {
      style = QwtPlotCurve::Lines;
      m_fileCurve->setCurveAttribute(QwtPlotCurve::Fitted);
      m_byteCurve->setCurveAttribute(QwtPlotCurve::Fitted);
   } else if (currentText == "Sticks") {
      style = QwtPlotCurve::Sticks;
   } else if (currentText == "Lines") {
      style = QwtPlotCurve::Lines;
      m_fileCurve->setCurveAttribute(QwtPlotCurve::Fitted);
      m_byteCurve->setCurveAttribute(QwtPlotCurve::Fitted);
   } else if (currentText == "Steps") {
      style = QwtPlotCurve::Steps;
   } else if (currentText == "None") {
      style = QwtPlotCurve::NoCurve;
   }
   m_fileCurve->setStyle(style);
   m_byteCurve->setStyle(style);
   m_jobPlot->replot();
}

/*
 * slot to respond to the symbol type combo changing
 */
void JobPlot::setFileSymbolType(int index)
{
   setSymbolType(index, 0);
}

void JobPlot::setByteSymbolType(int index)
{
   setSymbolType(index, 1);
}
void JobPlot::setSymbolType(int index, int type)
{
   QwtSymbol sym;
   sym.setPen(QColor(Qt::black));
   sym.setSize(7);
   if (index == 0) {
      sym.setStyle(QwtSymbol::Ellipse);
   } else if (index == 1) {
      sym.setStyle(QwtSymbol::Rect);
   } else if (index == 2) {
      sym.setStyle(QwtSymbol::Diamond);
   } else if (index == 3) {
      sym.setStyle(QwtSymbol::Triangle);
   } else if (index == 4) {
      sym.setStyle(QwtSymbol::DTriangle);
   } else if (index == 5) {
      sym.setStyle(QwtSymbol::UTriangle);
   } else if (index == 6) {
      sym.setStyle(QwtSymbol::LTriangle);
   } else if (index == 7) {
      sym.setStyle(QwtSymbol::RTriangle);
   } else if (index == 8) {
      sym.setStyle(QwtSymbol::Cross);
   } else if (index == 9) {
      sym.setStyle(QwtSymbol::XCross);
   } else if (index == 10) {
      sym.setStyle(QwtSymbol::HLine);
   } else if (index == 11) {
      sym.setStyle(QwtSymbol::VLine);
   } else if (index == 12) {
      sym.setStyle(QwtSymbol::Star1);
   } else if (index == 13) {
      sym.setStyle(QwtSymbol::Star2);
   } else if (index == 14) {
      sym.setStyle(QwtSymbol::Hexagon);
   }
   if (type == 0) {
      sym.setBrush(QColor(Qt::yellow));
      m_fileCurve->setSymbol(sym);
   }
   if (type == 1) {
      sym.setBrush(QColor(Qt::blue));
      m_byteCurve->setSymbol(sym);
   }
   m_jobPlot->replot();
}

/*
 * slot to respond to the file check box changing state
 */
void JobPlot::fileCheckChanged(int newstate)
{
   if (newstate == Qt::Unchecked) {
      m_fileCurve->detach();
      m_jobPlot->enableAxis(QwtPlot::yLeft, false);
   } else {
      m_fileCurve->attach(m_jobPlot);
      m_jobPlot->enableAxis(QwtPlot::yLeft);
   }
   m_jobPlot->replot();
}

/*
 * slot to respond to the byte check box changing state
 */
void JobPlot::byteCheckChanged(int newstate)
{
   if (newstate == Qt::Unchecked) {
      m_byteCurve->detach();
      m_jobPlot->enableAxis(QwtPlot::yRight, false);
   } else {
      m_byteCurve->attach(m_jobPlot);
      m_jobPlot->enableAxis(QwtPlot::yRight);
   }
   m_jobPlot->replot();
}

/*
 * Save user settings associated with this page
 */
void JobPlot::writeSettings()
{
   QSettings settings(m_console->m_dir->name(), "bat");
   settings.beginGroup("JobPlot");
   settings.setValue("m_splitterSizes", m_splitter->saveState());
   settings.setValue("fileSymbolTypeCombo", controls->fileSymbolTypeCombo->currentText());
   settings.setValue("byteSymbolTypeCombo", controls->byteSymbolTypeCombo->currentText());
   settings.setValue("plotTypeCombo", controls->plotTypeCombo->currentText());
   settings.endGroup();
}

/* 
 * Read settings values for Controls
 */
void JobPlot::readControlSettings()
{
   QSettings settings(m_console->m_dir->name(), "bat");
   settings.beginGroup("JobPlot");
   int fileSymbolTypeIndex = controls->fileSymbolTypeCombo->findText(settings.value("fileSymbolTypeCombo").toString(), Qt::MatchExactly);
   if (fileSymbolTypeIndex == -1) fileSymbolTypeIndex = 2;
      controls->fileSymbolTypeCombo->setCurrentIndex(fileSymbolTypeIndex);
   int byteSymbolTypeIndex = controls->byteSymbolTypeCombo->findText(settings.value("byteSymbolTypeCombo").toString(), Qt::MatchExactly);
   if (byteSymbolTypeIndex == -1) byteSymbolTypeIndex = 3;
      controls->byteSymbolTypeCombo->setCurrentIndex(byteSymbolTypeIndex);
   int plotTypeIndex = controls->plotTypeCombo->findText(settings.value("plotTypeCombo").toString(), Qt::MatchExactly);
   if (plotTypeIndex == -1) plotTypeIndex = 2;
      controls->plotTypeCombo->setCurrentIndex(plotTypeIndex);
   settings.endGroup();
}

/*
 * Read and restore user settings associated with this page
 */
void JobPlot::readSplitterSettings()
{
   QSettings settings(m_console->m_dir->name(), "bat");
   settings.beginGroup("JobPlot");
   m_splitter->restoreState(settings.value("m_splitterSizes").toByteArray());
   settings.endGroup();
}
