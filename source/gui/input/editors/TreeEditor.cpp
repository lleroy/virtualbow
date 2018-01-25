#include "TreeEditor.hpp"
#include "TreeItem.hpp"
#include "gui/input/dialogs/CommentDialog.hpp"
#include "gui/input/dialogs/ProfileDialog.hpp"
#include "gui/input/dialogs/WidthDialog.hpp"
#include "gui/input/dialogs/LayerDialog.hpp"
#include "gui/input/dialogs/SettingsDialog.hpp"
#include "gui/input/dialogs/StringDialog.hpp"
#include "gui/input/dialogs/MassesDialog.hpp"
#include "gui/input/dialogs/OperationDialog.hpp"

TreeEditor::TreeEditor()
{
    new TreeItem<CommentDialog, std::string>(this, data.meta.comments, "Comments", QIcon(":/icons/model-tree/comments"));
    new TreeItem<SettingsDialog, Settings2>(this, data.settings, "Settings", QIcon(":/icons/model-tree/settings"));

    auto parameters = new QTreeWidgetItem(this, {"Parameters"});
    parameters->setIcon(0, QIcon(":/icons/model-tree/parameters"));

    new TreeItem<ProfileDialog, Profile2>(parameters, data.profile, "Profile", QIcon(":/icons/model-tree/profile"));
    new TreeItem<WidthDialog, Series>(parameters, data.width, "Width", QIcon(":/icons/model-tree/width"));
    new TreeItem<LayerDialog, std::vector<Layer2>>(parameters, data.layers, "Layers", QIcon(":/icons/model-tree/layers"));
    new TreeItem<StringDialog, String2>(parameters, data.string, "String", QIcon(":/icons/model-tree/string"));
    new TreeItem<MassesDialog, Masses2>(parameters, data.masses, "Masses", QIcon(":/icons/model-tree/masses"));
    new TreeItem<OperationDialog, Operation2>(parameters, data.operation, "Operation", QIcon(":/icons/model-tree/operation"));

    QObject::connect(this, &QTreeWidget::itemActivated, [](QTreeWidgetItem* item, int column)
    {
        auto action = dynamic_cast<Action*>(item);
        if(action != nullptr)
            action->performAction();
    });

    this->setHeaderLabel("Model Tree");
    this->expandAll();
    // this->setItemsExpandable(false);    // Todo: Why is the expansion symbol still visible? (on KDE Desktop at least)
}

const InputData& TreeEditor::getData() const
{
    return data;
}


void TreeEditor::setData(const InputData& data)
{
    this->data = data;
}
