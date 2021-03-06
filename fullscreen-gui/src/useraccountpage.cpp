#include "useraccountpage.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QKeyEvent>
#include <QDebug>
#include <QSettings>

#include "defaultrichmenuitem.h"
#include "comborichmenuitem.h"

UserAccountPage::UserAccountPage(Page *parentPage, QString language, QWidget *parent)
  : Page (parentPage, "User Account Info:", "Edit user account", language, parent)
{
  QVBoxLayout *mainLayout = new QVBoxLayout;

  QStringList genres;
  genres << "m" << "f";

  _nameItem = new DefaultRichMenuItem ("Name:", DefaultRichMenuItem::NAME);
  _ageItem = new DefaultRichMenuItem ("Age:", DefaultRichMenuItem::NUMBER);
  _locationItem = new DefaultRichMenuItem ("Location (Zip):", DefaultRichMenuItem::LOCATION);
  _genreItem = new ComboRichMenuItem ("Genre:", genres);

  _nameItem->installEventFilter(this);
  _ageItem->installEventFilter(this);
  _locationItem->installEventFilter(this);
  _genreItem->installEventFilter(this);

  updateValues();

  QFont labelFont ("Tiresias", SCREEN_HEIGHT * 0.025, QFont::Bold);

  _changeValuesLabel = new FocusableLabel (QString ("<font color='white'>%1</font>").arg("> Save Values"));
  _changeValuesLabel->setFont(labelFont);
  _changeValuesLabel->installEventFilter(this);

  mainLayout->addSpacing(50);
  mainLayout->addWidget(_nameItem);
  mainLayout->addWidget(_ageItem);
  mainLayout->addWidget(_locationItem);
  mainLayout->addWidget(_genreItem);
  mainLayout->addSpacing(50);
  mainLayout->addWidget(_changeValuesLabel);

  QWidget *scrollWidget = new QWidget;
  scrollWidget->setLayout(mainLayout);

  QPalette palette = this->palette ();
  palette.setColor (QPalette::Window, QColor::fromRgb (0,0,0,0));

  scrollWidget->setBackgroundRole(QPalette::Window);
  scrollWidget->setPalette (palette);


  _itemsScrollArea->setWidget(scrollWidget);

  _imageLabel->setPixmap(QPixmap("../img/usermgmt.png"));
}

void UserAccountPage::updateValues()
{
  QSettings main_settings (QSettings::IniFormat, QSettings::UserScope,
                     TELEMIDIA_ID, APP_NAME, this);

  QString user_account_file =
      main_settings .value(USER_ACCOUNT_FILE, "").toString();
  if (user_account_file != "")
  {
    QSettings settings (user_account_file, QSettings::IniFormat, this);
    QString values = settings.value("||").toString();

    QStringList tokens = values.split(" ", QString::SkipEmptyParts);
    if (tokens.size() == 6){
      _nameItem->setValue(tokens[1]);
      _ageItem->setValue(tokens[3]);
      _locationItem->setValue(tokens[4]);
      _genreItem->setValue(tokens[5]);
    }
  }
}

void UserAccountPage::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Left)
    emit parentPageRequested(_parentPage);

  Page::keyPressEvent(event);
}

bool UserAccountPage::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::KeyPress){
    QKeyEvent *keyEvent = (QKeyEvent *) event;

    if (obj == _changeValuesLabel){
      if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return){
        persistValues();
        emit parentPageRequested(_parentPage);
        return true;
      }
    }
  }
  return Page::eventFilter(obj, event);
}

void UserAccountPage::persistValues()
{
  QFile userAccountFile (USER_ACCOUNT_FILE);
  if (userAccountFile.open(QIODevice::WriteOnly | QIODevice::Text)){
    userAccountFile.write(":: = 0\n");
    userAccountFile.write(" || = 0 " + _nameItem->value().toLatin1() + " 12345 " + _ageItem->value().toLatin1() + " "
                          + _locationItem->value().toLatin1() + " " + _genreItem->value().toLatin1() + "\n");
  }

  userAccountFile.close();
}
