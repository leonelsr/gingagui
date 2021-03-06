#include "mainwindow.h"

#include <QDebug>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QMovie>
#include <QSettings>
#include <QEventLoop>

#include "pagexmlparser.h"
#include "useraccountpage.h"
#include "userpreferences.h"
#include "ippage.h"

MainWindow::MainWindow(QString mainPage, QWidget *parent) :
  QMainWindow(parent)
{
  QSettings settigs (QSettings::IniFormat, QSettings::UserScope,
                     TELEMIDIA_ID, APP_NAME, this);
  _execPath = settigs.value("location").toString();

#if __linux__
  if (_execPath == "")
      _execPath = "ginga";
#endif

  _usbPage = 0;
  _ipPage = 0;

  _stackedLayout = new QStackedLayout;
  QWidget *mainWidget = new QWidget;
  mainWidget->setLayout(_stackedLayout);
  setCentralWidget(mainWidget);
  setWindowTitle("Ginga GUI");

  setStyleSheet("QMainWindow { border-image: url(:backgrounds/bg_gui); }");
  showFullScreen();

  _gingaProxy = GingaProxy::getInstance(_execPath, this);
  _gingaPage = new GingaPage;

  _stackedLayout->addWidget(_gingaPage);

  connect (_gingaProxy, SIGNAL(gingaStarted()),
           this, SLOT(showGingaView()));
  connect (_gingaProxy, SIGNAL(gingaFinished(int,QProcess::ExitStatus)),
           this, SLOT(hideGingaView()));
  connect (this, SIGNAL(keyPressed(QString)),
           _gingaProxy, SLOT(sendCommand(QString)));

  _mainPage = parsePage(mainPage);
  focusNextChild();

  _loadingPage = new QWidget (this);
  QMovie *movie = new QMovie(":/backgrounds/loading");
  QLabel *loadingLabel = new QLabel(this);
  loadingLabel->setMovie(movie);
  movie->start();

  QHBoxLayout *loadingLayout = new QHBoxLayout();
  loadingLayout->addWidget(loadingLabel);
  loadingLayout->setAlignment(loadingLabel,
                              Qt::AlignHCenter | Qt::AlignVCenter);

  _loadingPage->setLayout(loadingLayout);

  _stackedLayout->addWidget(_loadingPage);

  _usbPathWatcher = new QFileSystemWatcher(this);


  QDir tmp_dir (USB_XML_PARENT_DIR);
  if (!tmp_dir.exists())
      tmp_dir.mkpath(USB_XML_PARENT_DIR);

  _usbPathWatcher->addPath(USB_XML_PARENT_DIR);

#if __linux__
  QStringList args;
  args << "/mnt" << USB_XML_FILE;
  QProcess::execute("gingagui-minipc-walk", args);
#endif

  connect (_usbPathWatcher, SIGNAL (directoryChanged(QString)),
           this, SLOT(analyzeDir(QString)));

  _stackedLayout->setCurrentWidget(_mainPage);
}


Page* MainWindow::parsePage(QString pagePath)
{
  PageXmlParser *pageParser = new PageXmlParser(pagePath);

  if (!pageParser->hasError()){
    Page *currentPage = new Page (0, _gingaPage, _execPath,
                                  pageParser->title(),
                                  pageParser->description(),
                                  pageParser->language(),
                                  pageParser->items());

    connect (currentPage, SIGNAL(menuItemSelected(MenuItem*)),
             this, SLOT(changePage(MenuItem*)));
    connect (currentPage, SIGNAL(configurePlay()),
             this, SLOT(showGingaView()));

    _pages.insert(pagePath, currentPage);

    _stackedLayout->addWidget(currentPage);
    return currentPage;
  }

  return 0;
}

void MainWindow::changePage(MenuItem *item)
{
  if (item){
    QString path = item->link();

    Page *requestedPage = _pages.value(path);
    if (requestedPage && requestedPage != _ipPage){
      _stackedLayout->setCurrentWidget(requestedPage);
      requestedPage->updateValues();
    }
    else{
      Page *newPage = 0;

      PageXmlParser *pageParser = new PageXmlParser(path);
      if (!pageParser->hasError()){
        newPage = new Page ((Page *)_stackedLayout->currentWidget(),
                            _gingaPage, _execPath, pageParser->title(),
                            pageParser->description(),
                            pageParser->language(),
                            pageParser->items());
      }
      else {
        QStringList tokens = path.split("/", QString::SkipEmptyParts);
        QString lastToken = tokens.last();
        if (path.startsWith("exec:")){
          QProcess *process = new QProcess (this);

          _lastPage = _stackedLayout->currentWidget();
          _stackedLayout->setCurrentWidget(_loadingPage);

          QStringList args = path.split(" ");

          qDebug () << args;
          QString program = args[0].mid(7);

          args.removeFirst();

          qDebug () << program << " " << args;

          process->start(program, args);

          QEventLoop loop (this);
          connect (process, SIGNAL(finished(int)),
                   &loop, SLOT(quit()));
          loop.exec();

          delete process;

          if (lastToken == "trydhcp"){
#if __linux__
            process = new QProcess(this);

            process->start("getip");
            connect (process, SIGNAL(finished(int)),
                     &loop, SLOT(quit()));
            loop.exec();

            QString ip = QString (
                  process->readAllStandardOutput()).trimmed();

            qDebug () << ip;

            delete process;

            if (_ipPage)
              ((IPPage *)_ipPage)->setupIp (ip);
            else{
              _ipPage = new IPPage ((Page *)_lastPage, ip);
              _pages.insert(path, _ipPage);
              _stackedLayout->addWidget(_ipPage);

              connect(_ipPage, SIGNAL(parentPageRequested(Page*)),
                      this, SLOT(changePage(Page*)));
            }
            _stackedLayout->setCurrentWidget(_ipPage);
            _lastPage = 0;
#endif
          }
        }
        else{
          if (tokens.size() > 0){
            tokens = lastToken.split("#");

            if (tokens.size() > 1 && tokens.at(0) == "dyncontent.xml"){
              QString pageRequired = tokens.at(1);
              if (pageRequired == "usr-acct"){ // User account management
                newPage = new UserAccountPage ((Page*) _stackedLayout->currentWidget());
              }
              else if (pageRequired == "usr-prefs"){
                newPage = new UserPreferences ((Page *) _stackedLayout->currentWidget());
              }
              else if (pageRequired == "mnt-usb"){
                delete pageParser;
                pageParser = new PageXmlParser (USB_XML_FILE);

                newPage = new Page ((Page *)_stackedLayout->currentWidget(),
                                    _gingaPage, _execPath, pageParser->title(),
                                    pageParser->description(),
                                    pageParser->language(),
                                    pageParser->items());
                _usbPage = newPage;
              }
            }
          }
        }
      }
      if (newPage){
        setUpPage(path, newPage);
        _stackedLayout->setCurrentWidget(newPage);
      }

      delete pageParser;
    }
  }
}

void MainWindow::setUpPage (QString path, Page *page)
{
  if (!page) return;

  connect (page, SIGNAL(configurePlay()), this, SLOT(showGingaView()));
  connect (page, SIGNAL(menuItemSelected(MenuItem*)), this, SLOT(changePage(MenuItem*)));
  connect (page, SIGNAL(parentPageRequested(Page*)), this, SLOT(changePage(Page*)));

  _pages.insert(path, page);
  _stackedLayout->addWidget(page);
}

void MainWindow::showGingaView()
{
  qDebug () << "gui::gingaStarted";
  _lastPage = _stackedLayout->currentWidget();
  _stackedLayout->setCurrentWidget(_gingaPage);
  grabKeyboard();
}

void MainWindow::hideGingaView()
{
  qDebug () << "gui::gingaFinished";
  if (_lastPage){
    _stackedLayout->setCurrentWidget(_lastPage);
    _lastPage = 0;
  }
  releaseKeyboard();
}

void MainWindow::changePage(Page *page)
{
  if (page){
    _stackedLayout->setCurrentWidget(page);
  }
}

void MainWindow::keyPressEvent(QKeyEvent *keyEvent)
{
  qDebug () << keyEvent->key();
  if ((keyEvent->key() == Qt::Key_Left || keyEvent->key() == Qt::Key_Back)
      && _gingaProxy->state() != QProcess::Running){
    _stackedLayout->setCurrentWidget(_mainPage);
    focusNextChild();
    QMainWindow::keyPressEvent(keyEvent);
    return;
  }

  if (keyEvent->key() == Qt::Key_F11 || keyEvent->key() == Qt::Key_MediaStop)
  {
    _gingaProxy->killProcess();
  }
  else if (keyEvent->key() - Qt::Key_0 >= 0 && keyEvent->key() - Qt::Key_0 <= 9)
  {
    emit keyPressed(GINGA_KEY_PREFIX + QString::number(keyEvent->key() - Qt::Key_0));
  }
  else if (keyEvent->key() - Qt::Key_A >= 0 && keyEvent->key() - Qt::Key_A <= 26)
  {
    if (keyEvent->modifiers() == Qt::ShiftModifier)
      emit keyPressed(GINGA_KEY_PREFIX + QString(('A'+(keyEvent->key() - Qt::Key_A))));
    else
      emit keyPressed(GINGA_KEY_PREFIX + QString(('a'+(keyEvent->key() - Qt::Key_A))));
  }
  else if (keyEvent->key() == Qt::Key_PageDown)
  {
    emit keyPressed(GINGA_KEY_PREFIX + "PAGEDOWN");
  }
  else if (keyEvent->key() == Qt::Key_PageUp)
  {
    emit keyPressed(GINGA_KEY_PREFIX + "PAGEUP");
  }
  else if (keyEvent->key() - Qt::Key_F1 >= 0 && keyEvent->key() - Qt::Key_F1 < 11)
  {
    emit keyPressed(GINGA_KEY_PREFIX + "F"+QString::number(keyEvent->key() - Qt::Key_F1 + 1));
  }
  else if (keyEvent->key() == Qt::Key_Down)
  {
    emit keyPressed(GINGA_KEY_PREFIX + "DOWN");
  }
  else if (keyEvent->key() == Qt::Key_Left)
  {
    emit keyPressed(GINGA_KEY_PREFIX + "LEFT");
  }
  else if (keyEvent->key() == Qt::Key_Right)
  {
    emit keyPressed(GINGA_KEY_PREFIX + "RIGHT");
  }
  else if (keyEvent->key() == Qt::Key_Up)
  {
    emit keyPressed(GINGA_KEY_PREFIX + "UP");
  }
  else if (keyEvent->key() == Qt::Key_Tab)
  {
    emit keyPressed(GINGA_KEY_PREFIX + "TAB");
  }
  else if (keyEvent->key() == Qt::Key_Space)
  {
    emit keyPressed(GINGA_KEY_PREFIX + "SPACE");
  }
  else if (keyEvent->key() == Qt::Key_Backspace
           || keyEvent->key() == Qt::Key_Back)
  {
    emit keyPressed(GINGA_KEY_PREFIX + "BACKSPACE");
  }
  else if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
  {
    emit keyPressed(GINGA_KEY_PREFIX + "RETURN");
  }
  else if (keyEvent->key() == Qt::Key_Escape)
  {
    emit keyPressed(GINGA_KEY_PREFIX + "ESCAPE");
  }

  QMainWindow::keyPressEvent(keyEvent);
}

void MainWindow::analyzeDir(QString dir)
{
  QDir directory (dir);
  if (directory.exists("usb.xml.lock"))
  {
    _lastPage = _stackedLayout->currentWidget ();
    _stackedLayout->setCurrentWidget(_loadingPage);
  }
  else
  {
    PageXmlParser *parser = new PageXmlParser (USB_XML_FILE);
    if (_usbPage)
    {
      _usbPage->setUpItems(parser->items());
    }
    else
    {
      _usbPage = new Page (_mainPage, _gingaPage, _execPath,
                           parser->title(), parser->description(),
                           parser->language(), parser->items());

      setUpPage (USB_WILCARD, _usbPage);
    }

    if (!parser->hasError())
    {
      _stackedLayout->setCurrentWidget (_usbPage);      
    }
    focusNextChild();

    delete parser;
    _lastPage = 0;
  }
}

MainWindow::~MainWindow()
{
}
