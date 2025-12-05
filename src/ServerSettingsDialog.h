#pragma once

#include <QDialog>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

class ServerSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ServerSettingsDialog(QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("Настройки сервера");
        setModal(true);
        setMinimumWidth(400);

        auto* layout = new QVBoxLayout(this);

        // Port input
        auto* portLayout = new QHBoxLayout();
        portLayout->addWidget(new QLabel("Порт:"));
        m_portSpinBox = new QSpinBox();
        m_portSpinBox->setRange(1024, 65535);
        m_portSpinBox->setValue(12345);
        portLayout->addWidget(m_portSpinBox);
        layout->addLayout(portLayout);

        // Status label
        m_statusLabel = new QLabel("Ожидание подключения...");
        m_statusLabel->setAlignment(Qt::AlignCenter);
        m_statusLabel->setStyleSheet("color: gray; font-style: italic;");
        m_statusLabel->setVisible(false);
        layout->addWidget(m_statusLabel);

        // Buttons
        auto* buttonLayout = new QHBoxLayout();
        m_startButton = new QPushButton("Создать игру");
        m_cancelButton = new QPushButton("Отмена");
        
        m_startButton->setStyleSheet(
            "QPushButton {"
            "    font-size: 14px;"
            "    padding: 10px 20px;"
            "    background-color: #4CAF50;"
            "    color: white;"
            "    border: none;"
            "    border-radius: 5px;"
            "}"
            "QPushButton:hover {"
            "    background-color: #45a049;"
            "}"
            "QPushButton:disabled {"
            "    background-color: #cccccc;"
            "}");

        m_cancelButton->setStyleSheet(
            "QPushButton {"
            "    font-size: 14px;"
            "    padding: 10px 20px;"
            "    background-color: #f44336;"
            "    color: white;"
            "    border: none;"
            "    border-radius: 5px;"
            "}"
            "QPushButton:hover {"
            "    background-color: #da190b;"
            "}");

        buttonLayout->addWidget(m_startButton);
        buttonLayout->addWidget(m_cancelButton);
        layout->addLayout(buttonLayout);

        connect(m_startButton, &QPushButton::clicked, this, &ServerSettingsDialog::onStartClicked);
        connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    }

    int getPort() const
    {
        return m_portSpinBox->value();
    }

    void setWaitingForClient()
    {
        m_portSpinBox->setEnabled(false);
        m_startButton->setEnabled(false);
        m_statusLabel->setText("Ожидание подключения клиента...");
        m_statusLabel->setVisible(true);
    }

    void setError(const QString& error)
    {
        m_statusLabel->setText("Ошибка: " + error);
        m_statusLabel->setStyleSheet("color: red; font-style: italic;");
        m_statusLabel->setVisible(true);
        m_portSpinBox->setEnabled(true);
        m_startButton->setEnabled(true);
    }

signals:
    void startServerRequested(int port);

private slots:
    void onStartClicked()
    {
        emit startServerRequested(getPort());
    }

private:
    QSpinBox* m_portSpinBox;
    QLabel* m_statusLabel;
    QPushButton* m_startButton;
    QPushButton* m_cancelButton;
};
