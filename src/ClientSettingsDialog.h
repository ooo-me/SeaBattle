#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

class ClientSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ClientSettingsDialog(QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("Подключение к игре");
        setModal(true);
        setMinimumWidth(400);

        auto* layout = new QVBoxLayout(this);

        // Host input
        auto* hostLayout = new QHBoxLayout();
        hostLayout->addWidget(new QLabel("Адрес:"));
        m_hostLineEdit = new QLineEdit();
        m_hostLineEdit->setText("127.0.0.1");
        hostLayout->addWidget(m_hostLineEdit);
        layout->addLayout(hostLayout);

        // Port input
        auto* portLayout = new QHBoxLayout();
        portLayout->addWidget(new QLabel("Порт:"));
        m_portSpinBox = new QSpinBox();
        m_portSpinBox->setRange(1024, 65535);
        m_portSpinBox->setValue(12345);
        portLayout->addWidget(m_portSpinBox);
        layout->addLayout(portLayout);

        // Status label
        m_statusLabel = new QLabel();
        m_statusLabel->setAlignment(Qt::AlignCenter);
        m_statusLabel->setStyleSheet("color: gray; font-style: italic;");
        m_statusLabel->setVisible(false);
        layout->addWidget(m_statusLabel);

        // Buttons
        auto* buttonLayout = new QHBoxLayout();
        m_connectButton = new QPushButton("Подключиться");
        m_cancelButton = new QPushButton("Отмена");
        
        m_connectButton->setStyleSheet(
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

        buttonLayout->addWidget(m_connectButton);
        buttonLayout->addWidget(m_cancelButton);
        layout->addLayout(buttonLayout);

        connect(m_connectButton, &QPushButton::clicked, this, &ClientSettingsDialog::onConnectClicked);
        connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    }

    QString getHost() const
    {
        return m_hostLineEdit->text();
    }

    int getPort() const
    {
        return m_portSpinBox->value();
    }

    void setConnecting()
    {
        m_hostLineEdit->setEnabled(false);
        m_portSpinBox->setEnabled(false);
        m_connectButton->setEnabled(false);
        m_statusLabel->setText("Подключение к серверу...");
        m_statusLabel->setStyleSheet("color: gray; font-style: italic;");
        m_statusLabel->setVisible(true);
    }

    void setError(const QString& error)
    {
        m_statusLabel->setText("Ошибка: " + error);
        m_statusLabel->setStyleSheet("color: red; font-style: italic;");
        m_statusLabel->setVisible(true);
        m_hostLineEdit->setEnabled(true);
        m_portSpinBox->setEnabled(true);
        m_connectButton->setEnabled(true);
    }

signals:
    void connectRequested(const QString& host, int port);

private slots:
    void onConnectClicked()
    {
        emit connectRequested(getHost(), getPort());
    }

private:
    QLineEdit* m_hostLineEdit;
    QSpinBox* m_portSpinBox;
    QLabel* m_statusLabel;
    QPushButton* m_connectButton;
    QPushButton* m_cancelButton;
};
