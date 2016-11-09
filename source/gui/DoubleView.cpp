#include "DoubleView.hpp"
#include "../numerics/Domain.hpp"

DoubleView::DoubleView(DocItem<double>& doc_item)
    : doc_item(doc_item)
{
    connection = doc_item.connect([this](const double& value)
    {
        setValue(value);
    });

    QObject::connect(this, &QLineEdit::editingFinished, [this]()
    {
        try
        {
            double value = getValue();
            this->doc_item = value;
        }
        catch(const std::runtime_error&)
        {
            setValue(this->doc_item);
        }
    });
}

void DoubleView::setValue(double value)
{
    this->setText(QLocale::c().toString(value, 'g', 15));    // Todo: Magic number
}

double DoubleView::getValue() const
{
    bool ok;
    double value = QLocale::c().toDouble(this->text(), &ok);

    if(!ok)
    {
        throw std::runtime_error("Cannot convert inout to number");
    }

    return value;
}
