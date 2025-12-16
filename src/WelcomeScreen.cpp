#include "WelcomeScreen.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>

WelcomeScreen::WelcomeScreen(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    QLabel* title = new QLabel("Морской Бой");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 36px; font-weight: bold; margin: 50px; color: white;");

    QPushButton* startButton = new QPushButton("Начать игру");
    QPushButton* exitButton = new QPushButton("Выход");

    startButton->setStyleSheet(
        "QPushButton {"
        "    font-size: 18px;"
        "    padding: 15px 30px;"
        "    background-color: #4CAF50;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #45a049;"
        "}");

    exitButton->setStyleSheet(
        "QPushButton {"
        "    font-size: 18px;"
        "    padding: 15px 30px;"
        "    background-color: #f44336;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #da190b;"
        "}");

    layout->addWidget(title);
    layout->addWidget(startButton);
    layout->addWidget(exitButton);
    layout->setAlignment(Qt::AlignCenter);

    connect(startButton, &QPushButton::clicked, this, &WelcomeScreen::onStartButtonClicked);
    connect(exitButton, &QPushButton::clicked, qApp, &QApplication::quit);
}

void WelcomeScreen::onStartButtonClicked()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Введите имя игрока");
    dialog.setModal(true);

    QVBoxLayout* dialogLayout = new QVBoxLayout(&dialog);

    QLabel* label = new QLabel("Имя игрока:");
    QLineEdit* nameEdit = new QLineEdit();
    nameEdit->setPlaceholderText("Введите ваше имя");

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QPushButton* okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(false);

    // Enable OK button only when name is not empty
    connect(nameEdit, &QLineEdit::textChanged, [okButton](const QString& text) {
        okButton->setEnabled(!text.trimmed().isEmpty());
    });

    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    dialogLayout->addWidget(label);
    dialogLayout->addWidget(nameEdit);
    dialogLayout->addWidget(buttonBox);

    if (dialog.exec() == QDialog::Accepted)
    {
        QString playerName = nameEdit->text().trimmed();
        if (!playerName.isEmpty())
        {
            emit startGameRequested(playerName);
        }
    }
}
