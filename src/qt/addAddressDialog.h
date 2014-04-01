// Copyright (c) 2014 Ratcoin dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ADD_ADDRESS_DIALOG_H
#define ADD_ADDRESS_DIALOG_H

#include <QDialog>

class WalletModel;

namespace Ui {
    class AddAddressDialog;
}

/** Multifunctional dialog to add new address.
 */
class AddAddressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddAddressDialog( QWidget *parent);
    ~AddAddressDialog();

    void accept();

    void setModel(WalletModel *model);

private:
    Ui::AddAddressDialog *ui;
    WalletModel *model;
    bool fCapsLock;

private slots:
    void textChanged();

protected:
    bool event(QEvent *event);
    bool eventFilter(QObject *object, QEvent *event);
};

#endif // ADD_ADDRESS_DIALOG_H