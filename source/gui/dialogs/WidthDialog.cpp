#include "WidthDialog.hpp"

WidthDialog::WidthDialog(QWidget* parent)
    : PersistentDialog(parent, "WidthDialog", {800, 400}),    // Magic numbers
      edit(new TableEditor({"Position", "Width [m]"}, 100)),
      view(new SplineView("Position", "Width [m]"))
{
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QObject::connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto hbox = new QHBoxLayout();
    hbox->addWidget(edit, 0);
    hbox->addWidget(view, 1);

    auto vbox = new QVBoxLayout();
    vbox->addLayout(hbox, 1);
    vbox->addWidget(buttons, 0);

    this->setWindowTitle("Width");
    this->setLayout(vbox);

    // Event handling
    QObject::connect(edit, &TableEditor::rowSelectionChanged, view, &SplineView::setSelection);
    QObject::connect(edit, &TableEditor::modified, [&]{
        view->setData(edit->getData());
        emit modified();
    });
}

MatrixXd WidthDialog::getData() const
{
    return edit->getData();
}

void WidthDialog::setData(const MatrixXd& width)
{
    edit->setData(width);
    view->setData(width);
}
