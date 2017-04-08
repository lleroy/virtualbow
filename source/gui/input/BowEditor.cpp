#include "BowEditor.hpp"
#include "SeriesView.hpp"
#include "NumberGroup.hpp"
#include "LimbView.hpp"
#include "ProfileView.hpp"
#include "SplineView.hpp"
#include "DoubleView.hpp"
#include "model/InputData.hpp"
#include "gui/HorizontalLine.hpp"

BowEditor::BowEditor(InputData& data)
    : QSplitter(Qt::Vertical)
{
    this->addWidget(new LimbView(data));

    auto tabs = new QTabWidget();
    tabs->addTab(new GeneralEditor(data), "General");
    tabs->addTab(new ProfileEditor(data), "Profile");
    tabs->addTab(new WidthEditor(data), "Width");
    tabs->addTab(new HeightEditor(data), "Height");
    this->addWidget(tabs);

    this->setStretchFactor(0, 1);
    this->setStretchFactor(1, 0);
}

GeneralEditor::GeneralEditor(InputData& data)
{
    auto hbox = new QHBoxLayout();
    this->setLayout(hbox);

    auto group_limb = new NumberGroup("Limb Material");
    group_limb->addRow("rho:", "kg/m³", data.sections_rho);      // Todo: Use unicode character (\u2374). Problem: Windows
    group_limb->addRow("E:", "N/m²", data.sections_E);
    hbox->addWidget(group_limb);

    auto group_string = new NumberGroup("String");
    group_string->addRow("Strand stiffness:", "N/100%", data.string_strand_stiffness);
    group_string->addRow("Strand density:", "kg/m", data.string_strand_density);
    group_string->addRow("Number of strands:", "", data.string_n_strands);
    hbox->addWidget(group_string);

    auto group_masses = new NumberGroup("Additional masses");
    group_masses->addRow("String center:", "kg", data.mass_string_center);
    group_masses->addRow("String tip:", "kg", data.mass_string_tip);
    group_masses->addRow("Limb tip:", "kg", data.mass_limb_tip);
    hbox->addWidget(group_masses);

    auto group_operation = new NumberGroup("Operation");
    group_operation->addRow("Brace height:", "m", data.operation_brace_height);
    group_operation->addRow("Draw length:", "m", data.operation_draw_length);
    group_operation->addRow("Arrow mass:", "kg", data.operation_mass_arrow);
    hbox->addWidget(group_operation);
}

ProfileEditor::ProfileEditor(InputData& data)
{
    auto hbox = new QHBoxLayout();
    this->setLayout(hbox);

    auto series_view = new SeriesView("Length [m]", "Curvature [m⁻¹]", data.profile_segments);
    hbox->addWidget(series_view);

    auto vbox = new QVBoxLayout();
    vbox->setSpacing(0);
    hbox->addLayout(vbox, 1);

    auto plot = new ProfileView(data);
    vbox->addWidget(plot, 1);

    // Todo: Modify NumberGroup so that it can also display horizontal groups. Use here.
    auto hbox2 = new QHBoxLayout();
    hbox2->addStretch();
    hbox2->addWidget(new QLabel("X-offset:")); hbox2->addSpacing(10); hbox2->addWidget(new DoubleView(data.profile_x0)); hbox2->addSpacing(10); hbox2->addWidget(new QLabel("[m]"));
    hbox2->addStretch();
    hbox2->addWidget(new QLabel("Y-offset:")); hbox2->addSpacing(10); hbox2->addWidget(new DoubleView(data.profile_y0)); hbox2->addSpacing(10); hbox2->addWidget(new QLabel("[m]"));
    hbox2->addStretch();
    hbox2->addWidget(new QLabel("Angle:")); hbox2->addSpacing(10); hbox2->addWidget(new DoubleView(data.profile_phi0)); hbox2->addSpacing(10); hbox2->addWidget(new QLabel("[rad]"));
    hbox2->addStretch();

    vbox->addSpacing(10);    // Magic number
    vbox->addWidget(new HorizontalLine());
    vbox->addSpacing(10);    // Magic number
    vbox->addLayout(hbox2);
    vbox->addSpacing(5);    // Magic number
}

WidthEditor::WidthEditor(InputData& data)
{
    auto hbox = new QHBoxLayout();
    this->setLayout(hbox);

    auto series_view = new SeriesView("Rel. position", "Width [m]", data.sections_width);
    hbox->addWidget(series_view);

    auto spline_view = new SplineView("Relative position", "Width [m]", data.sections_width);
    hbox->addWidget(spline_view, 1);

    QObject::connect(series_view, &SeriesView::selectionChanged, spline_view, &SplineView::setMarkedControlPoints);
}


HeightEditor::HeightEditor(InputData& data)
{
    auto hbox = new QHBoxLayout();
    this->setLayout(hbox);

    auto series_view = new SeriesView("Rel. position", "Height [m]", data.sections_height);
    hbox->addWidget(series_view);

    auto spline_view = new SplineView("Relative position", "Height [m]", data.sections_height);
    hbox->addWidget(spline_view, 1);
}


