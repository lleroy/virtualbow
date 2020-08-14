#include "TreeEditor.hpp"
#include "TreeItem.hpp"
#include "gui/dialogs/CommentDialog.hpp"
#include "gui/dialogs/ProfileDialog.hpp"
#include "gui/dialogs/WidthDialog.hpp"
#include "gui/dialogs/LayerDialog.hpp"
#include "gui/dialogs/SettingsDialog.hpp"
#include "gui/dialogs/StringDialog.hpp"
#include "gui/dialogs/MassesDialog.hpp"
#include "gui/dialogs/DampingDialog.hpp"
#include "gui/dialogs/DimensionsDialog.hpp"

TreeEditor::TreeEditor()
{
    new TreeItem<CommentDialog, std::string>(this, data.comment, "Comments", QIcon(":/icons/model-comments.png"));
    new TreeItem<SettingsDialog, Settings>(this, data.settings, "Settings", QIcon(":/icons/model-settings.png"));
    new TreeItem<DimensionsDialog, Dimensions>(this, data.dimensions, "Dimensions", QIcon(":/icons/model-dimensions.png"));
    new TreeItem<ProfileDialog, MatrixXd>(this, data.profile, "Profile", QIcon(":/icons/model-profile.png"));
    new TreeItem<WidthDialog, MatrixXd>(this, data.width, "Width", QIcon(":/icons/model-width.png"));
    new TreeItem<LayerDialog, Layers>(this, data.layers, "Layers", QIcon(":/icons/model-layers.png"));
    new TreeItem<StringDialog, String>(this, data.string, "String", QIcon(":/icons/model-string.png"));
    new TreeItem<MassesDialog, Masses>(this, data.masses, "Masses", QIcon(":/icons/model-masses.png"));
    new TreeItem<DampingDialog, Damping>(this, data.damping, "Damping", QIcon(":/icons/model-damping.png"));

    QObject::connect(this, &QTreeWidget::itemActivated, [](QTreeWidgetItem* item, int column) {
        auto action = dynamic_cast<Action*>(item);
        if(action != nullptr) {
            action->performAction();
        }
    });

    this->setHeaderLabel("Model");
    this->expandAll();
    this->setItemsExpandable(false);    // Todo: Why is the expansion symbol still visible? (on KDE Desktop at least)
}

const InputData& TreeEditor::getData() const
{
    return data;
}

void TreeEditor::setData(const InputData& data)
{
    this->data = data;
}
