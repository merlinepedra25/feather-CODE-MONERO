// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2020-2022 The Monero Project

#include "TxImportDialog.h"
#include "ui_TxImportDialog.h"

#include <QMessageBox>

#include "utils/NetworkManager.h"

TxImportDialog::TxImportDialog(QWidget *parent, QSharedPointer<AppContext> ctx)
        : WindowModalDialog(parent)
        , ui(new Ui::TxImportDialog)
        , m_ctx(std::move(ctx))
        , m_loadTimer(new QTimer(this))
{
    ui->setupUi(this);
    ui->resp->hide();
    ui->label_loading->hide();

    auto node = m_ctx->nodes->connection();
    m_rpc = new DaemonRpc(this, getNetworkTor(), node.toAddress());

    connect(ui->btn_load, &QPushButton::clicked, this, &TxImportDialog::loadTx);
    connect(ui->btn_import, &QPushButton::clicked, this, &TxImportDialog::onImport);

    connect(m_rpc, &DaemonRpc::ApiResponse, this, &TxImportDialog::onApiResponse);

    connect(m_loadTimer, &QTimer::timeout, [this]{
        ui->label_loading->setText(ui->label_loading->text() + ".");
    });

    this->adjustSize();
}

void TxImportDialog::loadTx() {
    QString txid = ui->line_txid->text();

    if (m_ctx->wallet->haveTransaction(txid)) {
        QMessageBox::warning(this, "Warning", "This transaction already exists in the wallet. "
                                              "If you can't find it in your history, "
                                              "check if it belongs to a different account (Wallet -> Account)");
        return;
    }

    FeatherNode node = m_ctx->nodes->connection();

    if (node.isLocal()) {
        m_rpc->setNetwork(getNetworkClearnet());
    } else {
        m_rpc->setNetwork(getNetworkTor());
    }

    qDebug() << node.toURL();
    m_rpc->setDaemonAddress(node.toURL());
    m_rpc->getTransactions(QStringList() << txid, false, true);

    ui->label_loading->setText("Loading transaction");
    ui->label_loading->setHidden(false);
    m_loadTimer->start(1000);
}

void TxImportDialog::onApiResponse(const DaemonRpc::DaemonResponse &resp) {
    m_loadTimer->stop();
    ui->label_loading->setHidden(true);
    if (!resp.ok) {
        QMessageBox::warning(this, "Import transaction", resp.status);
        return;
    }

    if (resp.endpoint == DaemonRpc::Endpoint::GET_TRANSACTIONS) {
        ui->resp->setVisible(true);
        ui->resp->setPlainText(QJsonDocument(resp.obj).toJson(QJsonDocument::Indented));
        this->adjustSize();

        if (resp.obj.contains("missed_tx")) {
            ui->btn_import->setEnabled(false);
            QMessageBox::warning(this, "Load transaction", "Transaction could not be found. Make sure the txid is correct, or try connecting to a different node.");
            return;
        }

        QMessageBox::information(this, "Load transaction", "Transaction loaded successfully.\n\nAfter closing this message box click the Import button to import the transaction into your wallet.");
        m_transaction = resp.obj;
        ui->btn_import->setEnabled(true);
   }
}

void TxImportDialog::onImport() {
    QJsonObject tx = m_transaction.value("txs").toArray().first().toObject();

    QString txid = tx.value("tx_hash").toString();

    QVector<quint64> output_indices;
    for (const auto &o: tx.value("output_indices").toArray()) {
        output_indices.push_back(o.toInt());
    }

    quint64 height = tx.value("block_height").toInt();
    quint64 timestamp = tx.value("block_timestamp").toInt();

    bool pool = tx.value("in_pool").toBool();
    bool double_spend_seen = tx.value("double_spend_seen").toBool();

    if (m_ctx->wallet->importTransaction(tx.value("tx_hash").toString(), output_indices, height, timestamp, false, pool, double_spend_seen)) {
        if (!m_ctx->wallet->haveTransaction(txid)) {
            QMessageBox::warning(this, "Import transaction", "This transaction does not belong to this wallet.");
            return;
        }
        QMessageBox::information(this, "Import transaction", "Transaction imported successfully.");
    } else {
        QMessageBox::warning(this, "Import transaction", "Transaction import failed.");
    }
    m_ctx->refreshModels();
}

TxImportDialog::~TxImportDialog() = default;
